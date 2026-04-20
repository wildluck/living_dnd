#include "hex/grid.hpp"
#include "hex/coord.hpp"

#include <cassert>
#include <stdexcept>
#include <type_traits>
#include <unordered_set>
#include <cassert>
#include <print>

#define RUN(name) do { std::print("  " #name "... "); name(); std::print("OK\n"); } while (0)

using namespace lwe::hex;

/* ---------------- Construction ---------------- */
void construct_default() {
    HexGrid<int> grid;
    assert(grid.size() == 0);
    assert(grid.empty());
}

void construct_sized() {
    HexGrid<int> grid(100);
    assert(grid.size() == 0);
    assert(grid.empty());
}

/* ---------------- Access: get ---------------- */
void get_missing() {
    HexGrid<int> grid;
    auto result = grid.get(Coord(0, 0));
    assert(!result.has_value());
}

void get_existing() {
    HexGrid<int> grid;
    grid.set(Coord(1, 2), 42);
    auto result = grid.get(Coord(1, 2));
    assert(result.has_value());
    assert(result->get() == 42);
}

void get_mutable_ref() {
    HexGrid<int> grid;
    grid.set(Coord(0, 0), 10);
    auto result = grid.get(Coord(0, 0));
    result->get() = 99;
    assert(grid.at(Coord(0, 0)) == 99);
}

void get_const_missing() {
    const HexGrid<int> grid;
    auto result = grid.get(Coord(0, 0));
    assert(!result.has_value());
}

void get_const_existing() {
    HexGrid<int> grid;
    grid.set(Coord(3, -1), 77);
    const auto& cgrid = grid;
    auto result = cgrid.get(Coord(3, -1));
    assert(result.has_value());
    assert(result->get() == 77);
}

void get_const_is_immutable() {
    HexGrid<int> grid;
    grid.set(Coord(0, 0), 10);
    const auto& cgrid = grid;
    auto result = cgrid.get(Coord(0, 0));
    static_assert(std::is_const_v<std::remove_reference_t<decltype(result->get())>>);
}

/* ---------------- Access: at ---------------- */
void at_existing() {
    HexGrid<int> grid;
    grid.set(Coord(0, 0), 42);
    assert(grid.at(Coord(0, 0)) == 42);
}

void at_throws_missing() {
    HexGrid<int> grid;
    bool threw = false;
    try { (void)grid.at(Coord(0, 0)); }
    catch (const std::out_of_range&) { threw = true; }
    assert(threw);
}

void at_const() {
    HexGrid<int> grid;
    grid.set(Coord(0, 0), 42);
    const auto& cgrid = grid;
    assert(cgrid.at(Coord(0, 0)) == 42);
}

/* ---------------- Access: operator[] ---------------- */
void bracket_inserts_default() {
    HexGrid<int> grid;
    int& val = grid[Coord(5, -3)];
    assert(val == 0);
    assert(grid.contains(Coord(5, -3)));
    assert(grid.size() == 1);
}

void bracket_returns_existing() {
    HexGrid<int> grid;
    grid.set(Coord(0, 0), 42);
    assert(grid[Coord(0, 0)] == 42);
    (void)grid[Coord(0, 0)];
    assert(grid.at(Coord(0, 0)) == 42);
}

void bracket_mutable() {
    HexGrid<int> grid;
    grid[Coord(0, 0)] = 99;
    assert(grid.at(Coord(0, 0)) == 99);
}

/* ---------------- Modification ---------------- */
void set_new() {
    HexGrid<int> grid;
    grid.set(Coord(1, 1), 10);
    assert(grid.contains(Coord(1, 1)));
    assert(grid.at(Coord(1, 1)) == 10);
}

void set_overwrite() {
    HexGrid<int> grid;
    grid.set(Coord(0, 0), 10);
    grid.set(Coord(0, 0), 20);
    assert(grid.at(Coord(0, 0)) == 20);
    assert(grid.size() == 1);
}

struct Cell {
    int elevation;
    bool water;
    Cell() : elevation(0), water(false) {}
    Cell(int e, bool w) : elevation(e), water(w) {}
};

void emplace_test() {
    HexGrid<Cell> grid;
    Cell& c = grid.emplace(Coord(0, 0), 100, true);
    assert(c.elevation == 100);
    assert(c.water == true);
    assert(grid.at(Coord(0, 0)).elevation == 100);
}

void erase_missing() {
    HexGrid<int> grid;
    assert(grid.erase(Coord(0, 0)) == false);
}

void erase_existing() {
    HexGrid<int> grid;
    grid.set(Coord(0, 0), 42);
    assert(grid.erase(Coord(0, 0)) == true);
    assert(!grid.contains(Coord(0, 0)));
    assert(grid.size() == 0);
}

void clear_test() {
    HexGrid<int> grid;
    grid.set(Coord(0, 0), 1);
    grid.set(Coord(1, 0), 2);
    grid.set(Coord(0, 1), 3);
    grid.clear();
    assert(grid.empty());
    assert(grid.size() == 0);
}

/* ---------------- Query ---------------- */
void contains_test() {
    HexGrid<int> grid;
    assert(!grid.contains(Coord(0, 0)));
    grid.set(Coord(0, 0), 1);
    assert(grid.contains(Coord(0, 0)));
    assert(!grid.contains(Coord(1, 1)));
}

void size_tracks() {
    HexGrid<int> grid;
    assert(grid.size() == 0);
    grid.set(Coord(0, 0), 1);
    assert(grid.size() == 1);
    grid.set(Coord(1, 0), 2);
    assert(grid.size() == 2);
    (void)grid.erase(Coord(0, 0));
    assert(grid.size() == 1);
}

void empty_test() {
    HexGrid<int> grid;
    assert(grid.empty());
    grid.set(Coord(0, 0), 1);
    assert(!grid.empty());
    grid.clear();
    assert(grid.empty());
}

/* ---------------- Neighbor access ---------------- */
void neighbors_of_partial() {
    HexGrid<int> grid;
    Coord center(0, 0);
    grid.set(center, 0);
    grid.set(center.neighbor(Direction::East), 1);
    grid.set(center.neighbor(Direction::West), 2);
    auto nbrs = grid.neighbors_of(center);
    assert(nbrs.size() == 2);
}

void neighbors_of_empty() {
    HexGrid<int> grid;
    grid.set(Coord(0, 0), 1);
    auto nbrs = grid.neighbors_of(Coord(100, 100));
    assert(nbrs.size() == 0);
}

void neighbors_of_full() {
    auto grid = HexGrid<int>::generate(2, [](const Coord& c) { return c.length(); });
    auto nbrs = grid.neighbors_of(Coord(0, 0));
    assert(nbrs.size() == 6);
}

void neighbors_of_const() {
    HexGrid<int> grid;
    grid.set(Coord(0, 0), 0);
    grid.set(Coord(1, 0), 1);
    const auto& cgrid = grid;
    auto nbrs = cgrid.neighbors_of(Coord(0, 0));
    assert(nbrs.size() == 1);
    assert(nbrs[0].second.get() == 1);
}

/* ---------------- Filtering and search ---------------- */
void count_if_some() {
    auto grid = HexGrid<int>::generate(3, [](const Coord& c) { return c.length(); });
    auto count = grid.count_if([](const Coord&, const int& v) { return v == 1; });
    assert(count == 6); /* ring at distance 1 has 6 hexes */
}

void count_if_none() {
    auto grid = HexGrid<int>::generate(3, [](const Coord&) { return 0; });
    auto count = grid.count_if([](const Coord&, const int& v) { return v == 999; });
    assert(count == 0);
}

void find_if_found() {
    auto grid = HexGrid<int>::generate(3, [](const Coord& c) { return c.q * 100 + c.r; });
    auto result = grid.find_if([](const Coord&, const int& v) { return v == 200; }); /* q=2, r=0 */
    assert(result.has_value());
    assert(result->first == Coord(2, 0));
}

void find_if_not_found() {
    auto grid = HexGrid<int>::generate(2, [](const Coord&) { return 0; });
    auto result = grid.find_if([](const Coord&, const int& v) { return v == 999; });
    assert(!result.has_value());
}

void filter_some() {
    auto grid = HexGrid<int>::generate(3, [](const Coord& c) { return c.length(); });
    auto result = grid.filter([](const Coord&, const int& v) { return v == 2; });
    assert(result.size() == 12); /* ring at distance 2 has 12 hexes */
}

void filter_none() {
    auto grid = HexGrid<int>::generate(2, [](const Coord&) { return 0; });
    auto result = grid.filter([](const Coord&, const int& v) { return v == 999; });
    assert(result.size() == 0);
}

void nearest_to_self() {
    auto grid = HexGrid<int>::generate(3, [](const Coord& c) { return c.length(); });
    auto result = grid.nearest_to(Coord(0, 0), [](const Coord&, const int& v) { return v == 0;} );
    assert(result.has_value());
    assert(*result == Coord(0, 0));
}

void nearest_to_neighbor() {
    auto grid = HexGrid<int>::generate(5, [](const Coord& c) { return c.length(); });
    auto result = grid.nearest_to(Coord(0, 0), [](const Coord&, const int& v) { return v == 3; });
    assert(result.has_value());
    assert(Coord(0, 0).distance_to(*result) == 3);
}

void nearest_to_not_found() {
    auto grid = HexGrid<int>::generate(2, [](const Coord&) { return 0; });
    auto result = grid.nearest_to(Coord(0, 0), [](const Coord&, const int& v) { return v == 999; });
    assert(!result.has_value());
}

/* ---------------- Spatial ---------------- */
void bounding_box_test() {
    HexGrid<int> grid;
    grid.set(Coord(-3, 2), 1);
    grid.set(Coord(5, -4), 2);
    grid.set(Coord(1, 1), 3);
    auto [min, max] = grid.bounding_box();
    assert(min.q == -3 && min.r == -4);
    assert(max.q == 5 && max.r == 2);
}

void bounding_box_single() {
    HexGrid<int> grid;
    grid.set(Coord(3, -1), 42);
    auto [min, max] = grid.bounding_box();
    assert(min == Coord(3, -1));
    assert(max == Coord(3, -1));
}

void adjacent_region_test() {
    auto grid = HexGrid<int>::generate(3, [](const Coord&) { return 0; });
    /* Region is just the origin */
    std::unordered_set<Coord> region = {Coord(0, 0)};
    auto adj = grid.adjacent_region(region);
    assert(adj.size() == 6); /* 6 neighbors of origin, all in grid, none in region */
}

void adjacent_region_full_grid() {
    auto grid = HexGrid<int>::generate(2, [](const Coord&) { return 0; });
    /* Region = all hexes in grid -> no adjacent hexel left */
    std::unordered_set<Coord> region;
    for (const auto& c: Coord(0, 0).within_radius(2)) region.insert(c);
    auto adj = grid.adjacent_region(region);
    assert(adj.size() == 0);
}

/* ---------------- Transformation ---------------- */
void transform_same_type() {
    auto grid = HexGrid<int>::generate(2, [](const Coord& c) { return c.length(); });
    auto doubled = grid.transform([](const Coord&, const int& v) { return v * 2; });
    assert(doubled.size() == grid.size());
    assert(doubled.at(Coord(0, 0)) == 0);
    assert(doubled.at(Coord(1, 0)) == 2);
}

void transform_change_type() {
    auto grid = HexGrid<int>::generate(2, [](const Coord& c) { return c.length(); });
    auto as_double = grid.transform([](const Coord&, const int& v) { return static_cast<double>(v) + 0.5; });
    assert(as_double.size() == grid.size());
    assert(as_double.at(Coord(0, 0)) == 0.5);
    assert(as_double.at(Coord(1, 0)) == 1.5);
}

/* ---------------- Iteration ---------------- */
void for_each_visits_all() {
    auto grid = HexGrid<int>::generate(2, [](const Coord&) { return 1; });
    int sum = 0;
    grid.for_each([&sum](const Coord&, const int& v) { sum += v; });
    assert(sum == Coord::hex_count(2));
}

void for_each_mutate() {
    auto grid = HexGrid<int>::generate(2, [](const Coord&) { return 1; });
    grid.for_each([](const Coord&, int& v) { v *= 10; });
    assert(grid.at(Coord(0, 0)) == 10);
}

void coords_test() {
    auto grid = HexGrid<int>::generate(2, [](const Coord&) { return 0; });
    auto all = grid.coords();
    assert(static_cast<int>(all.size()) == Coord::hex_count(2));
    std::unordered_set<Coord> unique(all.begin(), all.end());
    assert(unique.size() == all.size());
}

void range_for_loop() {
    auto grid = HexGrid<int>::generate(1, [](const Coord&) { return 1; });
    int count = 0;
    for (auto& [coord, val] : grid) {
        count++;
        (void)coord; (void)val;
    }
    assert(count == Coord::hex_count(1));
}

/* ---------------- Bulk generation ---------------- */
void generate_count() {
    auto grid = HexGrid<int>::generate(4, [](const Coord&) { return 0; });
    assert(static_cast<int>(grid.size()) == Coord::hex_count(4));
}

void generate_values() {
    auto grid = HexGrid<int>::generate(3, [](const Coord& c) { return c.length(); });
    assert(grid.at(Coord(0, 0)) == 0);
    assert(grid.at(Coord(1, 0)) == 1);
    assert(grid.at(Coord(3, 0)) == 3);
}

void generate_within_radius() {
    auto grid = HexGrid<int>::generate(3, [](const Coord&) { return 0; });
    for (auto& [coord, _] : grid) {
        assert(Coord(0, 0).distance_to(coord) <= 3);
    }
}

int main() {
    std::print("\n ---------------- Grid Tests ---------------- \n\n");

    /* Construction */
    RUN(construct_default);
    RUN(construct_sized);

    /* Access: get */
    RUN(get_missing);
    RUN(get_existing);
    RUN(get_mutable_ref);
    RUN(get_const_missing);
    RUN(get_const_existing);
    RUN(get_const_is_immutable);

    /* Access: at */
    RUN(at_existing);
    RUN(at_throws_missing);
    RUN(at_const);

    /* Access: operator[] */
    RUN(bracket_inserts_default);
    RUN(bracket_returns_existing);
    RUN(bracket_mutable);

    /* Modification */
    RUN(set_new);
    RUN(set_overwrite);
    RUN(emplace_test);
    RUN(erase_missing);
    RUN(erase_existing);
    RUN(clear_test);

    /* Query */
    RUN(contains_test);
    RUN(size_tracks);
    RUN(empty_test);

    /* Neighbor access */
    RUN(neighbors_of_partial);
    RUN(neighbors_of_empty);
    RUN(neighbors_of_full);
    RUN(neighbors_of_const);

    /* Filtering and search */
    RUN(count_if_some);
    RUN(count_if_none);
    RUN(find_if_found);
    RUN(find_if_not_found);
    RUN(filter_some);
    RUN(filter_none);
    RUN(nearest_to_self);
    RUN(nearest_to_neighbor);
    RUN(nearest_to_not_found);

    /* Spatial */
    RUN(bounding_box_test);
    RUN(bounding_box_single);
    RUN(adjacent_region_test);
    RUN(adjacent_region_full_grid);

    /* Transformation */
    RUN(transform_same_type);
    RUN(transform_change_type);

    /* Iteration */
    RUN(for_each_visits_all);
    RUN(for_each_mutate);
    RUN(coords_test);
    RUN(range_for_loop);

    /* Bulk generation */
    RUN(generate_count);
    RUN(generate_values);
    RUN(generate_within_radius);

    std::print("\n ---------------- All Tests passed! ---------------- \n");
    return 0;
}
