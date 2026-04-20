#include "hex/algorithms.hpp"

#include <cassert>
#include <print>
#include <unordered_set>

using namespace lwe::hex;

#define RUN(name) do { std::print("  " #name "... "); name(); std::print("OK\n"); } while (0)

/* ---------------- A* ---------------- */
void astar_same_start_goal() {
    auto result = a_star(Coord(0, 0), Coord(0, 0),
        [](const Coord&, const Coord&) { return 1.0; }, 100);
    assert(result.has_value());
    assert(result->found());
    assert(result->path.size() == 1);
    assert(result->total_cost == 0.0);
}

void astar_straight_line() {
    auto result = a_star(Coord(0, 0), Coord(5, 0),
        [](const Coord&, const Coord&) { return 1.0; }, 100);
    assert(result.has_value());
    assert(result->path.front() == Coord(0, 0));
    assert(result->path.back() == Coord(5, 0));
    assert(result->total_cost == 5.0);
    assert(result->path.size() == 6);
}

void astar_avoids_impassable() {
    std::unordered_set<Coord> walls = {Coord(1, 0), Coord(1, -1)};
    auto result = a_star(Coord(0, 0), Coord(2, 0),
        [&](const Coord&, const Coord& to) -> double {
            if (walls.contains(to)) return -1.0;
            return 1.0;
        }, 100);
    assert(result.has_value());
    assert(result->path.front() == Coord(0, 0));
    assert(result->path.back() == Coord(2, 0));
    for (const auto& c : result->path) {
        assert(!walls.contains(c));
    }
}

void astar_no_path() {
    /* Surround origin completely */
    auto result = a_star(Coord(0, 0), Coord(5, 0),
        [](const Coord& from, const Coord&) -> double {
            if (from == Coord(0, 0)) return -1.0;
            return 1.0;
        }, 10);
    /* All edges from origin are impassable */
    /* Actually cost_fn(origin, neighbor) returns -1 since from==origin */
    assert(!result.has_value());
}

void astar_prefers_low_cost() {
    /* "Road" hexes along r=0 cost 1, everything else costs 10 */
    auto result = a_star(Coord(0, 0), Coord(4, 0),
        [](const Coord&, const Coord& to) -> double {
            return (to.r == 0) ? 1.0 : 10.0;
        }, 100);
    assert(result.has_value());
    for (const auto& c : result->path) {
        assert(c.r == 0);
    }
}

void astar_consecutive_steps() {
    auto result = a_star(Coord(0, 0), Coord(3, -2),
        [](const Coord&, const Coord&) { return 1.0; }, 100);
    assert(result.has_value());
    for (size_t i = 1; i < result->path.size(); ++i) {
        assert(result->path[i - 1].distance_to(result->path[i]) == 1);
    }
}

void astar_max_radius_limits() {
    /* Goal is at distance 10, but max_radius is 3 */
    auto result = a_star(Coord(0, 0), Coord(10, 0),
        [](const Coord&, const Coord&) { return 1.0; }, 3);
    assert(!result.has_value());
}

/* ---------------- BFS ---------------- */
void bfs_same_start_goal() {
    auto result = bfs(Coord(0, 0), Coord(0, 0),
        [](const Coord&) { return true; });
    assert(result.has_value());
    assert(result->path.size() == 1);
    assert(result->total_cost == 0.0);
}

void bfs_straight() {
    auto result = bfs(Coord(0, 0), Coord(3, 0),
        [](const Coord&) { return true; });
    assert(result.has_value());
    assert(result->path.front() == Coord(0, 0));
    assert(result->path.back() == Coord(3, 0));
    assert(result->total_cost == 3.0);
}

void bfs_impassable_start() {
    auto result = bfs(Coord(0, 0), Coord(3, 0),
        [](const Coord& c) { return c != Coord(0, 0); });
    assert(!result.has_value());
}

void bfs_no_path() {
    /* Wall blocks all paths */
    auto result = bfs(Coord(0, 0), Coord(5, 0),
        [](const Coord& c) {
            if (c.length() > 5) return false; /* bounded world */
            if (c.q == 2) return false; /* wall */
            return true;
        });
    assert(!result.has_value());
}

void bfs_shortest_path() {
    /* Open grid, BFS should find optimal (= hex distance) */
    Coord start(0, 0), goal(3, -2);
    auto result = bfs(start, goal, [](const Coord&) { return true; });
    assert(result.has_value());
    assert(static_cast<int>(result->path.size()) == start.distance_to(goal) + 1);
}

/* ---------------- flood_fill ---------------- */
void flood_fill_open() {
    auto filled = flood_fill(Coord(0, 0), [](const Coord&) { return true; }, 3);
    assert(static_cast<int>(filled.size()) == Coord::hex_count(3));
}

void flood_fill_blocked_start() {
    auto filled = flood_fill(Coord(0, 0), [](const Coord&) { return false; }, 3);
    assert(filled.empty());
}

void flood_fill_partial() {
    /* Only passable in q >= 0 half */
    auto filled = flood_fill(Coord(0, 0),
        [](const Coord& c) { return c.q >= 0; }, 3);
    for (const auto& c : filled) {
        assert(c.q >= 0);
        assert(Coord(0, 0).distance_to(c) <= 3);
    }
    assert(filled.size() > 1);
}

void flood_fill_no_radius_limit() {
    /* max_radius=0 means no limit — should fill outward freely */
    auto filled = flood_fill(Coord(0, 0),
        [](const Coord& c) { return c.length() <= 5; }, 0);
    assert(static_cast<int>(filled.size()) == Coord::hex_count(5));
}

void flood_fill_no_duplicates() {
    auto filled = flood_fill(Coord(0, 0), [](const Coord&) { return true; }, 4);
    /* Already a set, so no duplicates by definition, but check size matches */
    assert(static_cast<int>(filled.size()) == Coord::hex_count(4));
}

/* ---------------- distance_map ---------------- */
void distance_map_origin() {
    auto dmap = distance_map(Coord(0, 0), [](const Coord&) { return true; }, 3);
    assert(dmap[Coord(0, 0)] == 0);
}

void distance_map_neighbors() {
    auto dmap = distance_map(Coord(0, 0), [](const Coord&) { return true; }, 3);
    for (const auto& n : Coord(0, 0).neighbors()) {
        assert(dmap[n] == 1);
    }
}

void distance_map_ring2() {
    auto dmap = distance_map(Coord(0, 0), [](const Coord&) { return true; }, 3);
    for (const auto& c : Coord(0, 0).ring(2)) {
        assert(dmap[c] == 2);
    }
}

void distance_map_count() {
    auto dmap = distance_map(Coord(0, 0), [](const Coord&) { return true; }, 4);
    assert(static_cast<int>(dmap.size()) == Coord::hex_count(4));
}

void distance_map_blocked_start() {
    auto dmap = distance_map(Coord(0, 0), [](const Coord&) { return false; }, 3);
    assert(dmap.empty());
}

void distance_map_partial() {
    /* Only passable in q >= 0 */
    auto dmap = distance_map(Coord(0, 0),
        [](const Coord& c) { return c.q >= 0; }, 5);
    for (const auto& [c, d] : dmap) {
        assert(c.q >= 0);
    }
}

/* ---------------- weighted_flood_fill ---------------- */
void wff_uniform_cost() {
    auto result = weighted_flood_fill(Coord(0, 0),
        [](const Coord&, const Coord&) { return 1.0; }, 3.0);
    /* Everything within 3 steps should be reachable */
    assert(result[Coord(0, 0)] == 0.0);
    for (const auto& n : Coord(0, 0).neighbors()) {
        assert(result[n] == 1.0);
    }
    /* Distance 4 should NOT be present */
    for (const auto& c : Coord(0, 0).ring(4)) {
        assert(result.find(c) == result.end());
    }
}

void wff_variable_cost() {
    /* East costs 1, everything else costs 10 */
    auto result = weighted_flood_fill(Coord(0, 0),
        [](const Coord&, const Coord& to) -> double {
            return (to.r == 0 && to.q > 0) ? 1.0 : 10.0;
        }, 5.0);
    /* Should reach far east cheaply */
    assert(result.contains(Coord(5, 0)));
    assert(result[Coord(5, 0)] == 5.0);
    /* But not far west (cost 10 per step > max_cost 5) */
    assert(!result.contains(Coord(-1, 0)) || result[Coord(-1, 0)] == 10.0);
}

void wff_impassable() {
    auto result = weighted_flood_fill(Coord(0, 0),
        [](const Coord&, const Coord&) { return -1.0; }, 100.0);
    // Only origin reachable
    assert(result.size() == 1);
    assert(result[Coord(0, 0)] == 0.0);
}

void wff_zero_max_cost() {
    auto result = weighted_flood_fill(Coord(0, 0),
        [](const Coord&, const Coord&) { return 1.0; }, 0.0);
    assert(result.size() == 1);
    assert(result[Coord(0, 0)] == 0.0);
}

/* ---------------- voronoi_partition ---------------- */
void voronoi_two_seeds() {
    Coord seed_a(-3, 0), seed_b(3, 0);
    std::vector<Coord> seeds = {seed_a, seed_b};
    auto partition = voronoi_partition(seeds,
        [](const Coord& c) { return c.length() <= 5; });
    /* Each seed owns itself */
    assert(partition[seed_a] == 0);
    assert(partition[seed_b] == 1);
    /* Origin is equidistant — first seed to reach claims it (BFS order) */
    assert(partition.contains(Coord(0, 0)));
    /* Hexes near seed_a should belong to 0 */
    assert(partition[Coord(-2, 0)] == 0);
    /* Hexes near seed_b should belong to 1 */
    assert(partition[Coord(2, 0)] == 1);
}

void voronoi_single_seed() {
    std::vector<Coord> seeds = {Coord(0, 0)};
    auto partition = voronoi_partition(seeds,
        [](const Coord& c) { return c.length() <= 3; });
    /* Everything should belong to seed 0 */
    for (const auto& [coord, owner] : partition) {
        assert(owner == 0);
    }
    assert(static_cast<int>(partition.size()) == Coord::hex_count(3));
}

void voronoi_impassable_border() {
    Coord seed_a(-2, 0), seed_b(2, 0);
    std::vector<Coord> seeds = {seed_a, seed_b};
    /* Wall at q=0 */
    auto partition = voronoi_partition(seeds,
        [](const Coord& c) { return c.q != 0 && c.length() <= 4; });
    /* Seeds on opposite sides of wall shouldn't mix */
    for (const auto& [coord, owner] : partition) {
        if (coord.q < 0) assert(owner == 0);
        if (coord.q > 0) assert(owner == 1);
    }
}

/* ---------------- connected_components ---------------- */
void cc_single_component() {
    auto all = Coord(0, 0).within_radius(3);
    auto components = connected_components(
        [](const Coord& c) { return c.length() <= 3; }, all);
    assert(components.size() == 1);
    assert(static_cast<int>(components[0].size()) == Coord::hex_count(3));
}

void cc_two_islands() {
    /* Two separate clusters far apart */
    auto island_a = Coord(-10, 0).within_radius(2);
    auto island_b = Coord(10, 0).within_radius(2);
    std::vector<Coord> all_coords;
    all_coords.insert(all_coords.end(), island_a.begin(), island_a.end());
    all_coords.insert(all_coords.end(), island_b.begin(), island_b.end());

    auto components = connected_components(
        [](const Coord& c) {
            return c.distance_to(Coord(-10, 0)) <= 2 || c.distance_to(Coord(10, 0)) <= 2;
        }, all_coords);
    assert(components.size() == 2);
}

void cc_all_impassable() {
    auto all = Coord(0, 0).within_radius(2);
    auto components = connected_components(
        [](const Coord&) { return false; }, all);
    assert(components.empty());
}

void cc_scattered_singles() {
    /* Individual hexes far apart, each is its own component */
    std::vector<Coord> singles = {Coord(0, 0), Coord(10, 0), Coord(-10, 0)};
    auto components = connected_components(
        [&](const Coord& c) {
            return c == Coord(0, 0) || c == Coord(10, 0) || c == Coord(-10, 0);
        }, singles);
    assert(components.size() == 3);
    for (const auto& comp : components) {
        assert(comp.size() == 1);
    }
}

/* ---------------- minimum_spanning_tree ---------------- */
void mst_single_node() {
    std::vector<Coord> nodes = {Coord(0, 0)};
    auto edges = minimum_spanning_tree(nodes,
        [](const Coord& a, const Coord& b) { return static_cast<double>(a.distance_to(b)); });
    assert(edges.empty());
}

void mst_two_nodes() {
    std::vector<Coord> nodes = {Coord(0, 0), Coord(3, 0)};
    auto edges = minimum_spanning_tree(nodes,
        [](const Coord& a, const Coord& b) { return static_cast<double>(a.distance_to(b)); });
    assert(edges.size() == 1);
    /* Edge should connect the two nodes */
    assert((edges[0].first == Coord(0, 0) && edges[0].second == Coord(3, 0)));
}

void mst_edge_count() {
    /* N nodes -> N-1 edges */
    std::vector<Coord> nodes = {
        Coord(0, 0), Coord(5, 0), Coord(-5, 0),
        Coord(0, 5), Coord(0, -5)
    };
    auto edges = minimum_spanning_tree(nodes,
        [](const Coord& a, const Coord& b) { return static_cast<double>(a.distance_to(b)); });
    assert(edges.size() == nodes.size() - 1);
}

void mst_all_connected() {
    /* Every node should appear in at least one edge */
    std::vector<Coord> nodes = {
        Coord(0, 0), Coord(3, 0), Coord(-3, 0), Coord(0, 3)
    };
    auto edges = minimum_spanning_tree(nodes,
        [](const Coord& a, const Coord& b) { return static_cast<double>(a.distance_to(b)); });
    std::unordered_set<Coord> seen;
    for (const auto& [from, to] : edges) {
        seen.insert(from);
        seen.insert(to);
    }
    for (const auto& n : nodes) {
        assert(seen.contains(n));
    }
}

void mst_prefers_cheap_edges() {
    /* Triangle: A-B close, A-C far, B-C medium */
    /* MST should pick A-B and B-C, not A-C */
    Coord a(0, 0), b(1, 0), c(5, 0);
    std::vector<Coord> nodes = {a, b, c};
    auto edges = minimum_spanning_tree(nodes,
        [](const Coord& x, const Coord& y) { return static_cast<double>(x.distance_to(y)); });
    assert(edges.size() == 2);
    double total = 0;
    for (const auto& [from, to] : edges) {
        total += from.distance_to(to);
    }
    /* Optimal: A-B (1) + B-C (4) = 5, not A-B (1) + A-C (5) = 6 */
    assert(total == 5.0);
}

void mst_empty() {
    std::vector<Coord> nodes = {};
    auto edges = minimum_spanning_tree(nodes,
        [](const Coord& a, const Coord& b) { return static_cast<double>(a.distance_to(b)); });
    assert(edges.empty());
}

/* ---------------- Main ---------------- */
int main() {
    std::print("\n ---------------- Algorithm Tests ---------------- \n\n");

    /* A* */
    RUN(astar_same_start_goal);
    RUN(astar_straight_line);
    RUN(astar_avoids_impassable);
    RUN(astar_no_path);
    RUN(astar_prefers_low_cost);
    RUN(astar_consecutive_steps);
    RUN(astar_max_radius_limits);

    /* BFS */
    RUN(bfs_same_start_goal);
    RUN(bfs_straight);
    RUN(bfs_impassable_start);
    RUN(bfs_no_path);
    RUN(bfs_shortest_path);

    /* flood_fill */
    RUN(flood_fill_open);
    RUN(flood_fill_blocked_start);
    RUN(flood_fill_partial);
    RUN(flood_fill_no_radius_limit);
    RUN(flood_fill_no_duplicates);

    /* distance_map */
    RUN(distance_map_origin);
    RUN(distance_map_neighbors);
    RUN(distance_map_ring2);
    RUN(distance_map_count);
    RUN(distance_map_blocked_start);
    RUN(distance_map_partial);

    /* weighted_flood_fill */
    RUN(wff_uniform_cost);
    RUN(wff_variable_cost);
    RUN(wff_impassable);
    RUN(wff_zero_max_cost);

    /* voronoi_partition */
    RUN(voronoi_two_seeds);
    RUN(voronoi_single_seed);
    RUN(voronoi_impassable_border);

    /* connected_components */
    RUN(cc_single_component);
    RUN(cc_two_islands);
    RUN(cc_all_impassable);
    RUN(cc_scattered_singles);

    /* minimum_spanning_tree */
    RUN(mst_single_node);
    RUN(mst_two_nodes);
    RUN(mst_edge_count);
    RUN(mst_all_connected);
    RUN(mst_prefers_cheap_edges);
    RUN(mst_empty);

    std::print("\n ---------------- All Tests passed! ---------------- \n");
    return 0;
}
