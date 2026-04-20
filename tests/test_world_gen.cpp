#include "world_gen.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <optional>
#include <print>
#include <cassert>
#include <cstdlib>
#include <map>

#define RUN(name) do { std::print("  " #name "... "); name(); std::print("OK\n"); } while (0)

const char* biome_name(Biome b) {
    switch (b) {
    case Biome::Ocean:        return "Ocean";
    case Biome::Coast:        return "Coast";
    case Biome::Desert:       return "Desert";
    case Biome::Grassland:    return "Grassland";
    case Biome::Forest:       return "Forest";
    case Biome::Jungle:       return "Jungle";
    case Biome::Swamp:        return "Swamp";
    case Biome::Hills:        return "Hills";
    case Biome::Mountain:     return "Mountain";
    case Biome::Peak:         return "Peak";
    case Biome::AlpineForest: return "AlpineForest";
    case Biome::CloudForest:  return "CloudForest";
    case Biome::Beach:        return "Beach";
    case Biome::Cliff:        return "Cliff";
    default:                  return "?";
    }
}

char biome_char(Biome b, bool has_river) {
    if (has_river) return '~';
    switch (b) {
    case Biome::Ocean:        return '.';
    case Biome::Desert:       return 'd';
    case Biome::Grassland:    return ',';
    case Biome::Forest:       return 'F';
    case Biome::Jungle:       return 'J';
    case Biome::Swamp:        return 'w';
    case Biome::Hills:        return 'h';
    case Biome::Mountain:     return 'M';
    case Biome::Peak:         return '^';
    case Biome::AlpineForest: return 'A';
    case Biome::CloudForest:  return 'C';
    case Biome::Beach:        return '_';
    case Biome::Cliff:        return '-';
    default:                  return '?';
    }
}

void print_colored_biome_char(const HexData& hex) {
    if (hex.settlement_id.has_value()) { std::print("\033[33m@\033[0m "); return; }
    if (hex.poi_id.has_value()) { std::print("\033[31m!\033[0m "); return; }
    if (hex.has_river) { std::print("\033[36m~\033[0m "); return; }
    if (hex.terrain == Terrain::Road) { std::print("\033[33m#\033[0m "); return; }

    switch (hex.biome) {
    case Biome::Ocean:        std::print("\033[34m.\033[0m "); return;
    case Biome::Desert:       std::print("\033[38;5;208md\033[0m "); return;
    case Biome::Grassland:    std::print("\033[38;2;0;128;0m,\033[0m "); return;
    case Biome::Forest:       std::print("\033[32mF\033[0m "); return;
    case Biome::Jungle:       std::print("\033[38;5;22mJ\033[0m "); return;
    case Biome::Swamp:        std::print("\033[38;5;22mw\033[0m "); return;
    case Biome::Hills:        std::print("\033[32mh\033[0m "); return;
    case Biome::Mountain:     std::print("\033[90mM\033[0m "); return;
    case Biome::Peak:         std::print("\033[37m^\033[0m "); return;
    case Biome::AlpineForest: std::print("\033[38;2;34;139;34mA\033[0m "); return;
    case Biome::CloudForest:  std::print("\033[38;2;34;139;34mC\033[0m "); return;
    case Biome::Beach:        std::print("\033[38;2;238;214;175m_\033[0m "); return;
    case Biome::Cliff:        std::print("\033[38;2;194;178;128m-\033[0m "); return;
    default:                  std::print("? "); return;
    }
}

void print_kingdom_map(const WorldData& world) {
    std::print("\n ---------------- Kingdom Map ---------------- \n\n");
    int radius = world.radius;
    const char* colors[] = {
        "\033[31m", /* red */
        "\033[32m", /* green */
        "\033[33m", /* yellow */
        "\033[34m", /* blue */
        "\033[35m", /* magenta */
        "\033[36m", /* cyan */
    };

    for (int r = -radius; r <= radius; ++r) {
        int q_min = std::max(-radius, -radius - r);
        int q_max = std::min(radius, radius - r);
        for (int i = 0; i < std::abs(r); ++i) std::print(" ");

        for (int q = q_min; q <= q_max; ++q) {
            auto opt = world.grid.get(lwe::hex::Coord(q, r));
            if (!opt) { std::print("  "); continue; }
            const auto& hex = opt->get();

            if (hex.settlement_id.has_value()) {
                std::print("\033[33m@\033[0m ");
            } else if (!hex.kingdom_id.has_value()) {
                std::print("\033[34m.\033[0m "); // ocean/unowned
            } else {
                int kid = hex.kingdom_id.value();
                std::print("{}{}\033[0m ", colors[kid % 6], kid);
            }
        }
        std::print("\n");
    }
}

void print_climate_map(const WorldData& world) {
    std::print("\n ---------------- Climate Map ---------------- \n\n");
    int radius = world.radius;

    for (int r = -radius; r <= radius; ++r) {
        int q_min = std::max(-radius, -radius - r);
        int q_max = std::min(radius, radius - r);
        for (int i = 0; i < std::abs(r); ++i) std::print(" ");

        for (int q = q_min; q <= q_max; ++q) {
            auto opt = world.grid.get(lwe::hex::Coord(q, r));
            if (!opt) { std::print("  "); continue; }
            const auto& hex = opt->get();

            if (hex.biome == Biome::Ocean) {
                std::print("\033[34m.\033[0m ");
                continue;
            }

            switch (hex.climate) {
                case ClimateZone::Arctic:       std::print("\033[97mA\033[0m "); break;
                case ClimateZone::Subarctic:    std::print("\033[96mS\033[0m "); break;
                case ClimateZone::Temperate:    std::print("\033[32mT\033[0m "); break;
                case ClimateZone::Subtropical:  std::print("\033[33ms\033[0m "); break;
                case ClimateZone::Tropical:     std::print("\033[31mt\033[0m "); break;
                case ClimateZone::Arid:         std::print("\033[38;5;208ma\033[0m "); break;
                case ClimateZone::Alpine:       std::print("\033[37m^\033[0m "); break;
                case ClimateZone::Maritime:     std::print("\033[36mM\033[0m "); break;
            }
        }
        std::print("\n");
    }

    std::print("\nLegend: A=Arctic S=Subarctic T=Temperate s=Subtropical\n");
    std::print("        t=Tropical a=Arid ^=Alpine M=Maritime .=Ocean\n");

    // Stats
    std::map<ClimateZone, int> counts;
    world.grid.for_each([&](const lwe::hex::Coord&, const HexData& hex) {
        if (hex.biome != Biome::Ocean) counts[hex.climate]++;
    });

    auto name = [](ClimateZone z) -> const char* {
        switch (z) {
            case ClimateZone::Arctic:     return "Arctic";
            case ClimateZone::Subarctic:  return "Subarctic";
            case ClimateZone::Temperate:  return "Temperate";
            case ClimateZone::Subtropical: return "Subtropical";
            case ClimateZone::Tropical:   return "Tropical";
            case ClimateZone::Arid:       return "Arid";
            case ClimateZone::Alpine:     return "Alpine";
            case ClimateZone::Maritime:   return "Maritime";
        }
        return "?";
    };

    int total_land = 0;
    for (const auto& [z, c] : counts) total_land += c;
    std::print("\nClimate distribution ({} land hexes):\n", total_land);
    for (const auto& [zone, count] : counts) {
        double pct = 100.0 * count / total_land;
        std::print("  {:>12}: {:>5} ({:.1f}%)\n", name(zone), count, pct);
    }
}

/* ---------------- Tests ---------------- */
void world_has_correct_size() {
    WorldConfig cfg;
    cfg.radius = 10;
    WorldGenerator gen(cfg);
    auto world = gen.generate();
    assert(static_cast<int>(world.grid.size()) == lwe::hex::Coord::hex_count(10));
    assert(world.seed == cfg.seed);
    assert(world.radius == 10);
}

void elevation_in_range() {
    WorldConfig cfg;
    cfg.radius = 15;
    WorldGenerator gen(cfg);
    auto world = gen.generate();
    world.grid.for_each([](const lwe::hex::Coord&, const HexData& hex) {
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
    for (const auto& c : lwe::hex::Coord(0, 0).ring(15)) {
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
    w1.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex1) {
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
    w1.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex1) {
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
    world.grid.for_each([&](const lwe::hex::Coord&, const HexData& hex) {
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
    world.grid.for_each([](const lwe::hex::Coord&, const HexData& hex) {
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
    world.grid.for_each([](const lwe::hex::Coord&, const HexData& hex) {
        if (hex.has_river) {
            assert(hex.biome != Biome::Ocean);
            assert(hex.terrain == Terrain::River);
        }
    });
}

void print_world(const WorldData& world) {
    std::print("\n ---------------- World Map ---------------- \n");

    int radius = world.radius;

    for (int r = -radius; r <= radius; ++r) {
        int q_min = std::max(-radius, -radius - r);
        int q_max = std::min(radius, radius - r);

        int indent = std::abs(r);
        for (int i = 0; i < indent; ++i) std::print(" ");

        for (int q = q_min; q <= q_max; ++q) {
            auto opt = world.grid.get(lwe::hex::Coord(q, r));
            if (!opt) {
                std::print("  ");
                continue;
            }
            const auto& hex = opt->get();
            print_colored_biome_char(hex);
        }
        std::print("\n");
    }

    std::print("\nLegend: .=ocean d=desert ,=grass w=swamp h=hills F=forest\n");
    std::print(  "        J=jungle M=mountain A=alpine C=cloud ^=peak ~=rivers\n");

    std::map<Biome, int> counts;
    world.grid.for_each([&](const lwe::hex::Coord&, const HexData& hex) {
        counts[hex.biome]++;
    });

    int total = world.grid.size();
    std::print("\nBiome distribution ({} hexes):\n", total);
    for (const auto& [biome, count] : counts) {
        double pct = 100.0 * count / total;
        std::print("  {:>12}: {:>5} ({:.1f}%)\n", biome_name(biome), count, pct);
    }

    std::print("\nSettlements: {}\n", world.settlements.size());
    for (const auto& s : world.settlements) {
        const char* type_name = "";
        switch (s.type) {
            case SettlementType::City:     type_name = "City"; break;
            case SettlementType::Town:     type_name = "Town"; break;
            case SettlementType::Village:  type_name = "Village"; break;
            case SettlementType::Hamlet:   type_name = "Hamlet"; break;
            case SettlementType::Fortress: type_name = "Fortress"; break;
        }
        std::print("  {} ({}, pop {}) at ({},{})\n", s.name, type_name, s.population, s.coord.q, s.coord.r);
    }

    std::print("Trade Routes: {}\n", world.trade_routes.size());
    for (const auto& tr : world.trade_routes) {
        const char* res_name = "";
        switch (tr.resource) {
            case Resource::Food:  res_name = "Food";  break;
            case Resource::Wood:  res_name = "Wood";  break;
            case Resource::Iron:  res_name = "Iron";  break;
            case Resource::Gold:  res_name = "Gold";  break;
            case Resource::Stone: res_name = "Stone"; break;
            case Resource::Fish:  res_name = "Fish";  break;
        }
        std::print("  {} -> {} ({})\n",
                   world.settlements[tr.from_settlement].name,
                   world.settlements[tr.to_settlement].name, res_name);
    }

    std::print("Points of Interest: {}\n", world.pois.size());
    for (const auto& poi : world.pois) {
        std::print("  {} (CR {}) at({},{})\n",
                   poi.name, poi.difficulty, poi.coord.q, poi.coord.r);
    }

    std::print("Kingdoms: {}\n", world.kingdoms.size());
    for (const auto& k : world.kingdoms) {
        std::print("  {} (capital: {}, territory: {} hexes)\n",
                   k.name, k.capital.q, k.capital.r, k.territory.size());
        for (const auto& [other, score] : k.relationships) {
            std::print("    -> Kingdom_{}: {}\n", other, score);
        }
    }

    std::print("Rivers: {} hexes\n", world.rivers.size());
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
    print_world(world);
    print_kingdom_map(world);
    print_climate_map(world);

    return 0;
}
