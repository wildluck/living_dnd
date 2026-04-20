#include "hex/coord.hpp"

#include <unordered_set>
#include <cassert>
#include <print>

#define RUN(name) do { std::print("  " #name "... "); name(); std::print("OK\n"); } while (0)

using namespace lwe::hex;

void cube_invariant() {
    Coord c(3, -7);
    assert(c.q + c.r + c.s() == 0);
    Coord c2(-5, 12);
    assert(c2.q + c2.r + c2.s() == 0);
}

void arithmetic() {
    Coord a(1, 2), b(3, -1);
    assert((a + b) == Coord(4, 1));
    assert((a - b) == Coord(-2, 3));
    assert((a * 3) == Coord(3, 6));
    assert((-a) == Coord(-1, -2));
}

void distance_to_self() { assert(Coord(5,-3).distance_to(Coord(5,-3)) == 0); }

void distance_to_neighbor() {
    Coord a(0,0);
    for (auto& n : a.neighbors()) assert(a.distance_to(n) == 1);
}

void distance_symmetric() {
    Coord a(2,-5), b(-3,7);
    assert(a.distance_to(b) == b.distance_to(a));
}

void distance_known() {
    assert(Coord(0,0).distance_to(Coord(4,0)) == 4);
    assert(Coord(0,0).distance_to(Coord(3,-3)) == 3);
}

void length_test() {
    Coord c(3,-1);
    assert(c.length() == c.distance_to(Coord(0,0)));
}

void neighbors_count() { assert(Coord(0,0).neighbors().size() == 6); }

void neighbors_all_dist_1() {
    Coord c(5,-3);
    for (auto& n : c.neighbors()) assert(c.distance_to(n) == 1);
}

void neighbors_unique() {
    auto nbrs = Coord(0,0).neighbors();
    std::unordered_set<Coord> s(nbrs.begin(), nbrs.end());
    assert(s.size() == 6);
}

void direction_between_cardinal() {
    Coord o(0,0);
    assert(o.direction_between(Coord(1,0))  == Direction::East);
    assert(o.direction_between(Coord(-1,0)) == Direction::West);
    assert(o.direction_between(Coord(0,1))  == Direction::SouthEast);
    assert(o.direction_between(Coord(0,-1)) == Direction::NorthWest);
    assert(o.direction_between(Coord(1,-1)) == Direction::NorthEast);
    assert(o.direction_between(Coord(-1,1)) == Direction::SouthWest);
}

void direction_between_self() {
    assert(Coord(5,3).direction_between(Coord(5,3)) == Direction::DirectionCount);
}

void ring_0() {
    auto r = Coord(3,-2).ring(0);
    assert(r.size() == 1 && r[0] == Coord(3,-2));
}

void ring_1() {
    Coord c(0,0);
    auto r = c.ring(1);
    assert(r.size() == 6);
    for (auto& h : r) assert(c.distance_to(h) == 1);
}

void ring_n_size() {
    for (int rad = 0; rad <= 5; ++rad) {
        auto r = Coord(0,0).ring(rad);
        assert(static_cast<int>(r.size()) == (rad == 0 ? 1 : 6 * rad));
    }
}

void ring_correct_distance() {
    Coord c(2,-3);
    for (auto& h : c.ring(4)) assert(c.distance_to(h) == 4);
}

void ring_no_dupes() {
    auto r = Coord(0,0).ring(3);
    std::unordered_set<Coord> s(r.begin(), r.end());
    assert(s.size() == r.size());
}

void hex_count_test() {
    assert(Coord::hex_count(0) == 1);
    assert(Coord::hex_count(1) == 7);
    assert(Coord::hex_count(2) == 19);
    assert(Coord::hex_count(3) == 37);
    assert(Coord::hex_count(40) == 4921);
}

void within_radius_count() {
    for (int rad = 0; rad <= 5; ++rad) {
        auto all = Coord(0,0).within_radius(rad);
        assert(static_cast<int>(all.size()) == Coord::hex_count(rad));
    }
}

void within_radius_distances() {
    for (auto& h : Coord(0,0).within_radius(4))
        assert(Coord(0,0).distance_to(h) <= 4);
}

void within_radius_no_dupes() {
    auto all = Coord(0,0).within_radius(4);
    std::unordered_set<Coord> s(all.begin(), all.end());
    assert(s.size() == all.size());
}

void within_radius_off_center() {
    Coord c(5,-3);
    auto all = c.within_radius(3);
    assert(static_cast<int>(all.size()) == Coord::hex_count(3));
    for (auto& h : all) assert(c.distance_to(h) <= 3);
}

void rotate_cw_6() {
    Coord c(2,-1), r = c;
    for (int i = 0; i < 6; ++i) r = r.rotate_cw();
    assert(r == c);
}

void rotate_ccw_6() {
    Coord c(3,-5), r = c;
    for (int i = 0; i < 6; ++i) r = r.rotate_ccw();
    assert(r == c);
}

void rotate_inverse() {
    Coord c(4,-2);
    assert(c.rotate_cw().rotate_ccw() == c);
    assert(c.rotate_ccw().rotate_cw() == c);
}

void rotate_preserves_distance() {
    Coord c(3,-1), o(0,0);
    int d = c.distance_to(o);
    assert(c.rotate_cw().distance_to(o) == d);
    assert(c.rotate_ccw().distance_to(o) == d);
}

void line_to_self() {
    auto l = Coord(3,-2).line_to(Coord(3,-2));
    assert(l.size() == 1 && l[0] == Coord(3,-2));
}

void line_length() {
    Coord a(0,0), b(5,0);
    assert(static_cast<int>(a.line_to(b).size()) == a.distance_to(b) + 1);
}

void line_endpoints() {
    Coord a(1,-3), b(-2,4);
    auto l = a.line_to(b);
    assert(l.front() == a && l.back() == b);
}

void line_consecutive() {
    auto l = Coord(0,0).line_to(Coord(4,-2));
    for (size_t i = 1; i < l.size(); ++i)
        assert(l[i-1].distance_to(l[i]) == 1);
}

void wedge_basic() {
    auto w = Coord(0,0).wedge(Direction::East, 2);
    assert(static_cast<int>(w.size()) == 5); // dist1:2 + dist2:3
    for (auto& h : w) {
        assert(Coord(0,0).distance_to(h) >= 1);
        assert(Coord(0,0).distance_to(h) <= 2);
    }
}

void wedge_within_radius() {
    for (auto& h : Coord(0,0).wedge(Direction::NorthEast, 4))
        assert(Coord(0,0).distance_to(h) <= 4);
}

void cone_contains_wedge() {
    auto w = Coord(0,0).wedge(Direction::East, 3);
    auto c = Coord(0,0).cone(Direction::East, 3, 1);
    std::unordered_set<Coord> cs(c.begin(), c.end());
    for (auto& h : w) assert(cs.count(h) > 0);
}

void cone_spread_0_eq_wedge() {
    auto w = Coord(0,0).wedge(Direction::East, 3);
    auto c = Coord(0,0).cone(Direction::East, 3, 0);
    std::sort(w.begin(), w.end());
    assert(c.size() == w.size());
    for (size_t i = 0; i < c.size(); ++i) assert(c[i] == w[i]);
}

void cone_no_dupes() {
    auto c = Coord(0,0).cone(Direction::East, 3, 2);
    std::unordered_set<Coord> s(c.begin(), c.end());
    assert(s.size() == c.size());
}

void pixel_roundtrip() {
    for (auto& c : Coord(0,0).within_radius(5)) {
        auto px = c.to_pixel(16.0);
        assert(Coord::from_pixel(px, 16.0) == c);
    }
}

void pixel_origin() {
    auto px = Coord(0,0).to_pixel(10.0);
    assert(std::abs(px.x) < 0.001 && std::abs(px.y) < 0.001);
}

void cube_round_exact() {
    auto c = Coord::cube_round(3.0, -1.0);
    assert(c.q == 3 && c.r == -1);
}

void cube_round_invariant() {
    for (double fq = -3.0; fq <= 3.0; fq += 0.3)
        for (double fr = -3.0; fr <= 3.0; fr += 0.3)
            assert(Coord::cube_round(fq, fr).s() == -Coord::cube_round(fq, fr).q - Coord::cube_round(fq, fr).r);
}

void hash_test() {
    std::unordered_set<Coord> set;
    for (auto& c : Coord(0,0).within_radius(5)) set.insert(c);
    assert(set.size() == static_cast<size_t>(Coord::hex_count(5)));
}

int main() {
    std::print("\n ---------------- Coord Tests ---------------- \n\n");

    RUN(cube_invariant);
    RUN(arithmetic);
    RUN(distance_to_self);
    RUN(distance_to_neighbor);
    RUN(distance_symmetric);
    RUN(distance_known);
    RUN(length_test);
    RUN(neighbors_count);
    RUN(neighbors_all_dist_1);
    RUN(neighbors_unique);
    RUN(direction_between_cardinal);
    RUN(direction_between_self);
    RUN(ring_0);
    RUN(ring_1);
    RUN(ring_n_size);
    RUN(ring_correct_distance);
    RUN(ring_no_dupes);
    RUN(hex_count_test);
    RUN(within_radius_count);
    RUN(within_radius_distances);
    RUN(within_radius_no_dupes);
    RUN(within_radius_off_center);
    RUN(rotate_cw_6);
    RUN(rotate_ccw_6);
    RUN(rotate_inverse);
    RUN(rotate_preserves_distance);
    RUN(line_to_self);
    RUN(line_length);
    RUN(line_endpoints);
    RUN(line_consecutive);
    RUN(wedge_basic);
    RUN(wedge_within_radius);
    RUN(cone_contains_wedge);
    RUN(cone_spread_0_eq_wedge);
    RUN(cone_no_dupes);
    RUN(pixel_roundtrip);
    RUN(pixel_origin);
    RUN(cube_round_exact);
    RUN(cube_round_invariant);
    RUN(hash_test);

    std::print("\n ---------------- All Tests passed! ---------------- \n");
    return 0;
}
