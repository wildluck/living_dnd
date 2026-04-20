#include "noise/fbm.hpp"
#include "common.hpp"

#include <cassert>
#include <cmath>
#include <print>
#include <numeric>
#include <vector>

using namespace lwe::noise;

/* ---------------- SimplexNoise ---------------- */
void simplex_deterministic() {
    SimplexNoise n1(42), n2(42);
    for (double x = -5.0; x <= 5.0; x += 0.7)
        for (double y = -5.0; y <= 5.0; y += 0.7)
            assert(n1.sample(x, y) == n2.sample(x, y));
}

void simplex_different_seeds() {
    SimplexNoise n1(1), n2(999);
    int different = 0;
    for (double x = 0; x < 5.0; x += 0.5)
        for (double y = 0; y < 5.0; y += 0.5)
            if (std::abs(n1.sample(x, y) - n2.sample(x, y)) > 0.01)
                ++different;
    assert(different > 50);
}

void simplex_range() {
    SimplexNoise noise(123);
    double lo = 1.0, hi = -1.0;
    for (double x = -50.0; x <= 50.0; x += 0.3)
        for (double y = -50.0; y <= 50.0; y += 0.3) {
            double v = noise.sample(x, y);
            lo = std::min(lo, v);
            hi = std::max(hi, v);
        }
    assert(lo >= -1.0);
    assert(hi <= 1.0);
    assert(lo < -0.3);  /* should reach near extremes */
    assert (hi > 0.3);
}

void simplex_coherent() {
    SimplexNoise noise(42);
    double v1 = noise.sample(5.0, 5.0);
    double v2 = noise.sample(5.01, 5.01);
    assert(std::abs(v1 - v2) < 0.1);
}

void simplex_not_constant() {
    SimplexNoise noise(77);
    std::vector<double> values;
    for (double x = 0; x < 10; x += 0.5)
        values.push_back(noise.sample(x, 0.0));
    double avg = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    double variance = 0.0;
    for (double v : values) variance += (v - avg) * (v - avg);
    variance /= values.size();
    assert(variance > 0.01);
}

void simplex_seed_accessor() {
    SimplexNoise noise(42);
    assert(noise.seed() == 42);
}

/* ---------------- FBM ---------------- */
void fbm_deterministic() {
    SimplexNoise source(42);
    FBM fbm1(source, 6, 2.0, 0.5);
    FBM fbm2(source, 6, 2.0, 0.5);
    assert(fbm1.sample(1.5, 2.5) == fbm2.sample(1.5, 2.5));
}

void fbm_range() {
    SimplexNoise source(99);
    FBM fbm(source, 6, 2.0, 0.5);
    double lo = 1.0, hi = -1.0;
    for (double x = -20; x < 20; x += 0.3)
        for (double y = -20; y < 20; y += 0.3) {
            double v = fbm.sample(x, y);
            lo = std::min(lo, v);
            hi = std::max(hi, v);
        }
    assert(lo >= -1.01);
    assert(hi <= 1.01);
}

void fbm_more_octaves_differs() {
    SimplexNoise source(42);
    FBM low(source, 1, 2.0, 0.5);
    FBM high(source, 8, 2.0, 0.5);
    /* With 1 octave vs 8, results should differ at most points */
    int different = 0;
    for (double x = 0; x < 5; x += 0.5)
        for (double y = 0; y < 5; y += 0.5)
            if (std::abs(low.sample(x, y) - high.sample(x, y)) > 0.01)
                ++different;
    assert(different > 20);
}

void fbm_accessors() {
    SimplexNoise source(42);
    FBM fbm(source, 4, 3.0, 0.6);
    assert(fbm.octaves() == 4);
    assert(fbm.lacunarity() == 3.0);
    assert(fbm.persistence() == 0.6);
}

void fbm_single_octave_matches_simplex() {
    SimplexNoise source(42);
    FBM fbm(source, 1, 2.0, 0.5);
    /* With 1 octave, FBM should equal raw simplex */
    for (double x = -3; x < 3; x += 0.7)
        for (double y = -3; y < 3; y += 0.7)
            assert(std::abs(fbm.sample(x, y) - source.sample(x, y)) < 1e-10);
}

/* ---------------- DomainWarp ---------------- */
void warp_zero_strength_equals_base() {
    SimplexNoise base(1), wx(2), wy(3);
    DomainWarp warp(base, wx, wy, 0.0);
    for (double x = -3; x < 3; x += 0.5)
        for (double y = -3; y < 3; y += 0.5)
            assert(warp.sample(x, y) == base.sample(x, y));
}

void warp_nonzero_differs_from_base() {
    SimplexNoise base(1), wx(2), wy(3);
    DomainWarp warp(base, wx, wy, 1.0);
    int different = 0;
    for (double x = -3; x < 3; x += 0.5)
        for (double y = -3; y < 3; y += 0.5)
            if (std::abs(warp.sample(x, y) - base.sample(x, y)) > 0.01)
                ++different;
    assert(different > 50);
}

void warp_deterministic() {
    SimplexNoise base(1), wx(2), wy(3);
    DomainWarp warp1(base, wx, wy, 0.5);
    DomainWarp warp2(base, wx, wy, 0.5);
    for (double x = -2; x < 2; x += 0.5)
        for (double y = -2; y < 2; y += 0.5)
            assert(warp1.sample(x, y) == warp2.sample(x, y));
}

void warp_strength_accessor() {
    SimplexNoise base(1), wx(2), wy(3);
    DomainWarp warp(base, wx, wy, 0.75);
    assert(warp.strength() == 0.75);
}

void warp_range() {
    SimplexNoise base(1), wx(2), wy(3);
    DomainWarp warp(base, wx, wy, 0.5);
    for (double x = -10; x < 10; x += 0.5)
        for (double y = -10; y < 10; y += 0.5) {
            double v = warp.sample(x, y);
            assert(v >= -1.0 && v <= 1.0);
        }
}

/* ---------------- Main ---------------- */
int main() {
    std::print("\n ---------------- Noise Tests ---------------- \n\n");

    /* SimplexNoise */
    RUN(simplex_deterministic);
    RUN(simplex_different_seeds);
    RUN(simplex_range);
    RUN(simplex_coherent);
    RUN(simplex_not_constant);
    RUN(simplex_seed_accessor);

    /* FBM */
    RUN(fbm_deterministic);
    RUN(fbm_range);
    RUN(fbm_more_octaves_differs);
    RUN(fbm_accessors);
    RUN(fbm_single_octave_matches_simplex);

    /* DomainWarp */
    RUN(warp_zero_strength_equals_base);
    RUN(warp_nonzero_differs_from_base);
    RUN(warp_deterministic);
    RUN(warp_strength_accessor);
    RUN(warp_range);

    std::print("\n ---------------- All Tests passed! ---------------- \n");
    return 0;
}
