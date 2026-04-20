#include "world_gen.hpp"
#include "display/display.hpp"
#include "common.hpp"

#include <chrono>
#include <optional>
#include <print>
#include <cassert>
#include <cstdlib>
#include <map>

using namespace lwe::hex;
using namespace lwe::display;

/* ---------------- Tests ---------------- */
void world_has_correct_size() {
    WorldConfig cfg;
    cfg.radius = 10;
    WorldGenerator gen(cfg);
    auto world = gen.generate();
    assert(static_cast<int>(world.grid.size()) == Coord::hex_count(10));
    assert(world.seed == cfg.seed);
    assert(world.radius == 10);
}

void elevation_in_range() {
    WorldConfig cfg;
    cfg.radius = 15;
    WorldGenerator gen(cfg);
    auto world = gen.generate();
    world.grid.for_each([](const Coord&, const HexData& hex) {
        assert(hex.elevation >= 0.0f);
        assert(hex.elevation <= 1.0f);
    });
}

void edges_are_ocean() {
    WorldConfig cfg;
    cfg.radius = 15;
    WorldGenerator gen(cfg);
    auto world = gen.generate();
    int ocean = 0, total = 0;
    for (const auto& c : Coord(0, 0).ring(15)) {
        auto opt = world.grid.get(c);
        if (opt) {
            ++total;
            if (opt->get().biome == Biome::Ocean) ++ocean;
        }
    }
    assert(total > 0);
    assert(ocean == total);
}

void deterministic() {
    WorldConfig cfg;
    cfg.radius = 10;
    auto w1 = WorldGenerator(cfg).generate();
    auto w2 = WorldGenerator(cfg).generate();
    w1.grid.for_each([&](const Coord& c, const HexData& hex1) {
        const auto& hex2 = w2.grid.at(c);
        assert(hex1.elevation == hex2.elevation);
        assert(hex1.moisture == hex2.moisture);
        assert(hex1.biome == hex2.biome);
    });
}

void different_seeds_differ() {
    WorldConfig cfg1;
    cfg1.seed = 1;
    cfg1.radius = 10;
    WorldConfig cfg2;
    cfg2.seed = 999;
    cfg2.radius = 10;
    auto w1 = WorldGenerator(cfg1).generate();
    auto w2 = WorldGenerator(cfg2).generate();
    int different = 0;
    w1.grid.for_each([&](const Coord& c, const HexData& hex1) {
        const auto& hex2 = w2.grid.at(c);
        if (std::abs(hex1.elevation - hex2.elevation) > 0.01) ++different;
    });
    assert(different > 50);
}

void has_biomes() {
    WorldConfig cfg;
    cfg.radius = 20;
    WorldGenerator gen(cfg);
    auto world = gen.generate();
    std::map<Biome, int> counts;
    world.grid.for_each([&](const Coord&, const HexData& hex) {
        counts[hex.biome]++;
    });
    assert(counts[Biome::Ocean] > 0);
    int land = 0;
    for (const auto& [b, c] : counts) {
        if (b != Biome::Ocean) land += c;
    }
    assert(land > 0);
}

void terrain_matches_biome() {
    WorldConfig cfg;
    cfg.radius = 15;
    WorldGenerator gen(cfg);
    auto world = gen.generate();
    world.grid.for_each([](const Coord&, const HexData& hex) {
        if (hex.biome == Biome::Ocean) {
            assert(hex.terrain == Terrain::Water);
        } else if (!hex.has_river) {
            assert(hex.terrain == Terrain::Land);
        }
    });
}

void rivers_only_on_land() {
    WorldConfig cfg;
    cfg.radius = 20;
    WorldGenerator gen(cfg);
    auto world = gen.generate();
    world.grid.for_each([](const Coord&, const HexData& hex) {
        if (hex.has_river) {
            assert(hex.biome != Biome::Ocean);
            assert(hex.terrain == Terrain::River);
        }
    });
}

int main() {
    std::print("\n ---------------- World Generation Tests ---------------- \n\n");

    RUN(world_has_correct_size);
    RUN(elevation_in_range);
    RUN(edges_are_ocean);
    RUN(deterministic);
    RUN(different_seeds_differ);
    RUN(has_biomes);
    RUN(terrain_matches_biome);
    RUN(rivers_only_on_land);

    std::print("\n ---------------- All Tests passed! ---------------- \n");

    uint64_t seed = static_cast<uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    std::print("\n ---------------- The seed is {}! ---------------- \n", seed);

    WorldConfig cfg;
    cfg.seed = seed;
    cfg.radius = 40;
    cfg.dungeon_density = 0.005;
    WorldGenerator gen(cfg);
    auto world = gen.generate();
    print_world_map(world);
    print_kingdom_stats(world);
    print_climate_map(world);

    return 0;
}
