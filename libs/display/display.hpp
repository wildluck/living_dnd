#pragma once

#include "common/types.hpp"
#include "common/world_data.hpp"
#include "hex/coord.hpp"
#include "weather_engine.hpp"
#include "effect_resolver.hpp"

#include <map>
#include <print>

namespace lwe::display {

/* ---------------- Biome rendering ---------------- */
inline void print_biome_char(const HexData& hex) {
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

/* ---------------- World map ---------------- */
inline void print_world_map(const WorldData& world) {
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
            print_biome_char(hex);
        }
        std::print("\n");
    }

    std::print("\nLegend: .=ocean d=desert ,=grass w=swamp h=hills F=forest\n");
    std::print(  "        J=jungle M=mountain A=alpine C=cloud ^=peak ~=rivers\n");
    std::print(  "        #=road @=settlement !=POI\n");
}

/* ---------------- Biome stats ---------------- */
inline const char* biome_name(Biome b) {
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

inline void print_biome_stats(const WorldData& world) {
    std::map<Biome, int> counts;
    world.grid.for_each([&](const hex::Coord&, const HexData& hex) {
        counts[hex.biome]++;
    });

    int total = world.grid.size();
    std::print("\nBiome distribution ({} hexes):\n", total);
    for (const auto& [biome, count] : counts) {
        double pct = 100.0 * count / total;
        std::print("  {:>14}: {:>5} ({:.1f}%)\n", biome_name(biome), count, pct);
    }
}

/* ---------------- Settlement stats ---------------- */
inline const char* settlement_type_name(SettlementType t) {
    switch (t) {
    case SettlementType::City:     return "City";
    case SettlementType::Town:     return "Town";
    case SettlementType::Village:  return "Village";
    case SettlementType::Hamlet:   return "Hamlet";
    case SettlementType::Fortress: return "Fortress";
    default:                       return "?";
    }
}

inline void print_settlement_stats(const WorldData& world) {
    std::print("\nSettlements: {}\n", world.settlements.size());
    for (const auto& s : world.settlements) {
        std::print("  {} ({}, pop {}) at ({},{})\n",
            s.name, settlement_type_name(s.type), s.population, s.coord.q, s.coord.r);
    }
}

/* ---------------- Trade route stats ---------------- */
inline const char* resource_name(Resource r) {
    switch (r) {
    case Resource::Food:  return "Food";
    case Resource::Wood:  return "Wood";
    case Resource::Iron:  return "Iron";
    case Resource::Gold:  return "Gold";
    case Resource::Stone: return "Stone";
    case Resource::Fish:  return "Fish";
    default:              return "?";
    }
}

inline void print_trade_stats( const WorldData& world) {
    std::print("\nTrade Routes: {}\n", world.trade_routes.size());
    for (const auto& tr : world.trade_routes) {
        std::print("  {} -> {} ({})\n",
                   world.settlements[tr.from_settlement].name,
                   world.settlements[tr.to_settlement].name,
                   resource_name(tr.resource));
    }
}

/* ---------------- POI stats ---------------- */
inline void print_poi_stats(const WorldData& world) {
    std::print("\nPoints of Interest: {}\n", world.pois.size());
    for (const auto& poi : world.pois) {
        std::print("  {} (CR {}) at ({},{})\n",
                   poi.name, poi.difficulty, poi.coord.q, poi.coord.r);
    }
}

/* ---------------- Kingdom Map ---------------- */
inline void print_kingdom_map(const WorldData& world) {
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
                std::print("\033[34m.\033[0m "); /* ocean/unowned */
            } else {
                int kid = hex.kingdom_id.value();
                std::print("{}{}\033[0m ", colors[kid % 6], kid);
            }
        }
        std::print("\n");
    }
}

inline void print_kingdom_stats(const WorldData& world) {
    std::print("\nKingdoms: {}\n", world.kingdoms.size());
    for (const auto& k : world.kingdoms) {
        std::print("\n  {} (capital: {},{}, territory: {} hexes)\n",
                   k.name, k.capital.q, k.capital.r, k.territory.size());
        for (const auto& [other, score] : k.relationships) {
            std::print("    -> Kingdom_{}: {}", other, score);
        }
    }
}

/* ---------------- Climate Map ---------------- */
inline const char* climate_name(ClimateZone z) {
    switch (z) {
        case ClimateZone::Arctic:      return "Arctic";
        case ClimateZone::Subarctic:   return "Subarctic";
        case ClimateZone::Temperate:   return "Temperate";
        case ClimateZone::Subtropical: return "Subtropical";
        case ClimateZone::Tropical:    return "Tropical";
        case ClimateZone::Arid:        return "Arid";
        case ClimateZone::Alpine:      return "Alpine";
        case ClimateZone::Maritime:    return "Maritime";
        default:                       return "?";
    }
}

inline void print_climate_map(const WorldData& world) {
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
    std::print(  "        t=Tropical a=Arid ^=Alpine M=Maritime .=Ocean\n");

    std::map<ClimateZone, int> counts;
    world.grid.for_each([&](const lwe::hex::Coord&, const HexData& hex) {
        if (hex.biome != Biome::Ocean) counts[hex.climate]++;
    });

    int total_land = 0;
    for (const auto& [z, c] : counts) total_land += c;
    std::print("\nClimate distribution ({} land hexes):\n", total_land);
    for (const auto& [zone, count] : counts) {
        double pct = 100.0 * count / total_land;
        std::print("  {:>12}: {:>5} ({:.1f}%)\n", climate_name(zone), count, pct);
    }
}

/* ---------------- Weather Map ---------------- */
inline const char* season_name(Season s) {
    switch (s) {
        case Season::Spring: return "Spring";
        case Season::Summer: return "Summer";
        case Season::Autumn: return "Autumn";
        case Season::Winter: return "Winter";
        default:             return "?";
    }
}

inline void print_weather_map(const WorldData& world, const WeatherEngine& engine, const Tick& tick) {
    std::print("\n ---------------- Weather Map (Day {}, Year {}) ----------------\n\n", tick.day, tick.year);

    int radius = world.radius;
    for (int r = -radius; r <= radius; ++r) {
        int q_min = std::max(-radius, -radius - r);
        int q_max = std::min(radius, radius - r);
        for (int i = 0; i < std::abs(r); ++i) std::print(" ");

        for (int q = q_min; q <= q_max; ++q) {
            auto opt = world.grid.get(lwe::hex::Coord(q, r));
            if (!opt) { std::print(" "); continue; }

            const auto& hex = opt->get();
            const auto& w = engine.get(lwe::hex::Coord(q, r));

            if (hex.biome == Biome::Ocean) {
                std::print("\033[34m.\033[0m ");
                continue;
            }

            if (w.is_stormy) {
                std::print("\033[97;41m*\033[0m "); /* white on red */
            } else if (w.temperature < 0) {
                std::print("\033[96m#\033[0m "); /* cyan = cold */
            } else if (w.temperature > 30) {
                std::print("\033[31m+\033[0m "); /* red = hot */
            } else if (w.precipitation > 0.5) {
                std::print("\033[34m~\033[0m "); /* blue = rainy */
            } else {
                std::print("\033[32m-\033[0m "); /* green = mild */
            }
        }
        std::print("\n");
    }

    std::print("\nLegend: *=storm #=freezing +=hot ~=rainy -=mild .-ocean\n");

    double min_temp = 100, max_temp = -100, avg_temp = 0;
    int storm_count = 0, total_land = 0;
    double avg_precip = 0;

    world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (hex.biome == Biome::Ocean) return;
        total_land++;
        const auto& w = engine.get(c);
        min_temp = std::min(min_temp, w.temperature);
        max_temp = std::max(max_temp, w.temperature);
        avg_temp += w.temperature;
        avg_precip += w.precipitation;
        if (w.is_stormy) storm_count++;
    });

    if (total_land > 0) {
        avg_temp /= total_land;
        avg_precip /= total_land;
    }

    std::print("\nTemperature: {:.2f}°C to {:.2f}°C (avg {:.2f}°C\n", min_temp, max_temp, avg_temp);
    std::print("Avg precipitation: {:.2f}\n", avg_precip);
    std::print("Storms: {} / Hexes\n", storm_count, total_land);
}

/* ---------------- Effect Display ---------------- */
inline const char* mod_type_name(ModType t) {
    switch (t) {
    case ModType::SkillCheck:       return "[Skill]";
    case ModType::AttackRoll:       return "[Attack]";
    case ModType::MovementSpeed:    return "[Speed]";
    case ModType::Damage:           return "[Damage]";
    case ModType::SavingThrow:      return "[Save]";
    case ModType::Visibility:       return "[Vision]";
    case ModType::DifficultTerrain: return "[Terrain]";
    default:                        return "[?]";
    }
}

inline void print_hex_effects(const WorldData& world,
                              const EffectResolver& resolver,
                              const WeatherEngine& engine,
                              const hex::Coord& coord,
                              const Tick& tick) {
    auto opt = world.grid.get(coord);
    if (!opt) return;
    const auto& hex = opt->get();
    const auto& w = engine.get(coord);
    auto effects = resolver.resolve(coord, w, tick);

    std::print("\n  Hex ({},{}) {}\n", coord.q, coord.r, biome_name(hex.biome));
    std::print("  Weather: {:.2f}°C, precip={:.2f}, wind={:.2f}{}\n",
        w.temperature,
        w.precipitation,
        w.wind_speed,
        (w.is_stormy ? " [STORM]" : "")
    );

    if (effects.empty()) {
        std::print("  No active effects.\n");
        return;
    }

    std::print("  Active effects ({}):\n", effects.size());
    for (const auto& effect : effects) {
        std::print("    \033[33m{}\033[0m - {}\n", effect.name, effect.description);
        for (const auto& mod : effect.mods) {
            std::print("      {}  ", mod_type_name(mod.type));
            if (mod.modifier > 0) std::print("+");
            std::print("{} - {}\n", mod.modifier, mod.detail);
        }
    }
}

/* ---------------- Print everything ---------------- */
inline void print_full_world(const WorldData& world) {
    print_world_map(world);
    print_biome_stats(world);
    print_settlement_stats(world);
    print_trade_stats(world);
    print_poi_stats(world);
    print_kingdom_stats(world);
    std::print("\nRivers: {} hexes\n", world.rivers.size());
}

} /* namespace lwe::display */
