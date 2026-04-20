#include "effect_resolver.hpp"
#include "world_gen.hpp"

#include <cassert>
#include <print>
#include <algorithm>

#define RUN(name) do { std::print("  " #name "... "); name(); std::print("OK\n"); } while (0)

const uint64_t WORLD_SEED = 500;
const int WORLD_RADIUS = 40;

static WorldData make_test_world() {
    WorldConfig cfg;
    cfg.seed = WORLD_SEED;
    cfg.radius = WORLD_RADIUS;
    return WorldGenerator(cfg).generate();
}

static WorldData test_world = make_test_world();

/* ---------------- Helper: check if effects contain a named effect ---------------- */
bool has_effect(const std::vector<HexEffect>& effects, const std::string& name) {
    return std::any_of(effects.begin(), effects.end(),
        [&](const HexEffect& e) { return e.name == name; });
}

bool has_mod_type(const std::vector<HexEffect>& effects, ModType type) {
    for (const auto& e : effects)
        for (const auto& m : e.mods)
            if (m.type == type) return true;
    return false;
}

/* ---------------- Temperature tests ---------------- */
void extreme_cold_triggers() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = -15;
    w.precipitation = 0.1;
    w.wind_speed = 0.3;
    w.is_stormy = false;

    /* Find any land hex */
    lwe::hex::Coord land;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (hex.biome != Biome::Ocean) land = c;
    });

    Tick tick = { 315, 1, Season::Winter, 45 };
    auto effects = resolver.resolve(land, w, tick);
    assert(has_effect(effects, "Extreme Cold"));
    assert(has_mod_type(effects, ModType::SavingThrow));
    assert(has_mod_type(effects, ModType::Damage));
}

void freezing_triggers() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = -3;
    w.precipitation = 0.1;
    w.wind_speed = 0.3;
    w.is_stormy = false;

    lwe::hex::Coord land;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (hex.biome != Biome::Ocean) land = c;
    });

    Tick tick = { 315, 1, Season::Winter, 45 };
    auto effects = resolver.resolve(land, w, tick);
    assert(has_effect(effects, "Freezing"));
    assert(has_mod_type(effects, ModType::DifficultTerrain));
}

void extreme_heat_triggers() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = 40;
    w.precipitation = 0.05;
    w.wind_speed = 0.2;
    w.is_stormy = false;

    lwe::hex::Coord land;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (hex.biome != Biome::Ocean) land = c;
    });

    Tick tick = { 135, 1, Season::Summer, 45 };
    auto effects = resolver.resolve(land, w, tick);
    assert(has_effect(effects, "Extreme Heat"));
    assert(has_mod_type(effects, ModType::SavingThrow));
}

void mild_weather_no_temperature_effects() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = 20;
    w.precipitation = 0.1;
    w.wind_speed = 0.2;
    w.is_stormy = false;

    lwe::hex::Coord grassland;
    bool found = false;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (!found && hex.biome == Biome::Grassland) { grassland = c; found = true; }
    });
    assert(found);

    Tick tick = { 45, 1, Season::Spring, 45 };
    auto effects = resolver.resolve(grassland, w, tick);
    assert(!has_effect(effects, "Extreme Cold"));
    assert(!has_effect(effects, "Freezing"));
    assert(!has_effect(effects, "Extreme Heat"));
    assert(!has_effect(effects, "Oppressive Heat"));
}

/* ---------------- Precipitation tests ---------------- */
void downpour_triggers() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = 15;
    w.precipitation = 0.85;
    w.wind_speed = 0.3;
    w.is_stormy = false;

    lwe::hex::Coord land;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (hex.biome != Biome::Ocean) land = c;
    });

    Tick tick = { 45, 1, Season::Spring, 45 };
    auto effects = resolver.resolve(land, w, tick);
    assert(has_effect(effects, "Downpour"));
}

void heavy_rain_triggers() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = 15;
    w.precipitation = 0.65;
    w.wind_speed = 0.3;
    w.is_stormy = false;

    lwe::hex::Coord land;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (hex.biome != Biome::Ocean) land = c;
    });

    Tick tick = { 45, 1, Season::Spring, 45 };
    auto effects = resolver.resolve(land, w, tick);
    assert(has_effect(effects, "Heavy Rain"));
}

/* ---------------- Wind tests ---------------- */
void gale_triggers() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = 10;
    w.precipitation = 0.1;
    w.wind_speed = 0.95;
    w.is_stormy = false;

    lwe::hex::Coord land;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (hex.biome != Biome::Ocean) land = c;
    });

    Tick tick = { 45, 1, Season::Spring, 45 };
    auto effects = resolver.resolve(land, w, tick);
    assert(has_effect(effects, "Gale Force Wind"));
    assert(has_mod_type(effects, ModType::AttackRoll));
}

void strong_wind_triggers() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = 10;
    w.precipitation = 0.1;
    w.wind_speed = 0.75;
    w.is_stormy = false;

    lwe::hex::Coord land;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (hex.biome != Biome::Ocean) land = c;
    });

    Tick tick = { 45, 1, Season::Spring, 45 };
    auto effects = resolver.resolve(land, w, tick);
    assert(has_effect(effects, "Strong Wind"));
}

/* ---------------- Storm tests ---------------- */
void thunderstorm_triggers() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = 20;
    w.precipitation = 0.8;
    w.wind_speed = 0.8;
    w.is_stormy = true;

    lwe::hex::Coord land;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (hex.biome != Biome::Ocean) land = c;
    });

    Tick tick = { 135, 1, Season::Summer, 45 };
    auto effects = resolver.resolve(land, w, tick);
    assert(has_effect(effects, "Thunderstorm"));
}

void blizzard_triggers() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = -5;
    w.precipitation = 0.7;
    w.wind_speed = 0.8;
    w.is_stormy = true;

    lwe::hex::Coord land;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (hex.biome != Biome::Ocean) land = c;
    });

    Tick tick = { 315, 1, Season::Winter, 45 };
    auto effects = resolver.resolve(land, w, tick);
    assert(has_effect(effects, "Blizzard"));
}

void sandstorm_triggers() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = 38;
    w.precipitation = 0.1;
    w.wind_speed = 0.9;
    w.is_stormy = true;

    lwe::hex::Coord desert;
    bool found = false;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (!found && hex.biome == Biome::Desert) { desert = c; found = true; }
    });

    if (!found) return; // skip if no desert

    Tick tick = { 135, 1, Season::Summer, 45 };
    auto effects = resolver.resolve(desert, w, tick);
    assert(has_effect(effects, "Sandstorm"));
}

void storm_skips_rain_and_wind_checks() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = 20;
    w.precipitation = 0.9;
    w.wind_speed = 0.95;
    w.is_stormy = true;

    lwe::hex::Coord land;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (hex.biome != Biome::Ocean) land = c;
    });

    Tick tick = { 135, 1, Season::Summer, 45 };
    auto effects = resolver.resolve(land, w, tick);
    /* Storm should trigger but NOT separate rain/wind effects */
    assert(has_effect(effects, "Thunderstorm"));
    assert(!has_effect(effects, "Downpour"));
    assert(!has_effect(effects, "Gale Force Wind"));
}

/* ---------------- Terrain tests ---------------- */
void swamp_has_difficult_terrain() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = 20;
    w.precipitation = 0.3;
    w.wind_speed = 0.1;
    w.is_stormy = false;

    lwe::hex::Coord swamp;
    bool found = false;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (!found && hex.biome == Biome::Swamp) { swamp = c; found = true; }
    });

    if (!found) return;

    Tick tick = { 45, 1, Season::Spring, 45 };
    auto effects = resolver.resolve(swamp, w, tick);
    assert(has_effect(effects, "Swampland"));
    assert(has_mod_type(effects, ModType::DifficultTerrain));
}

void jungle_has_effects() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = 28;
    w.precipitation = 0.3;
    w.wind_speed = 0.1;
    w.is_stormy = false;

    lwe::hex::Coord jungle;
    bool found = false;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (!found && hex.biome == Biome::Jungle) { jungle = c; found = true; }
    });

    if (!found) return;

    Tick tick = { 135, 1, Season::Summer, 45 };
    auto effects = resolver.resolve(jungle, w, tick);
    assert(has_effect(effects, "Dense Jungle"));
    assert(has_mod_type(effects, ModType::DifficultTerrain));
    assert(has_mod_type(effects, ModType::Visibility));
}

/* ---------------- River tests ---------------- */
void river_crossing_triggers() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = 15;
    w.precipitation = 0.2;
    w.wind_speed = 0.2;
    w.is_stormy = false;

    lwe::hex::Coord river;
    bool found = false;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (!found && hex.has_river) { river = c; found = true; }
    });

    if (!found) return;

    Tick tick = { 45, 1, Season::Spring, 45 };
    auto effects = resolver.resolve(river, w, tick);
    assert(has_effect(effects, "River Crossing"));
}

void raging_river_in_storm() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = 15;
    w.precipitation = 0.8;
    w.wind_speed = 0.7;
    w.is_stormy = true;

    lwe::hex::Coord river;
    bool found = false;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (!found && hex.has_river) { river = c; found = true; }
    });

    if (!found) return;

    Tick tick = { 270, 1, Season::Autumn, 90 };
    auto effects = resolver.resolve(river, w, tick);
    assert(has_effect(effects, "Raging River"));
}

/* ---------------- Altitude tests ---------------- */
void peak_altitude_sickness() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = -10;
    w.precipitation = 0.2;
    w.wind_speed = 0.5;
    w.is_stormy = false;

    lwe::hex::Coord peak;
    bool found = false;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (!found && hex.biome == Biome::Peak) { peak = c; found = true; }
    });

    if (!found) return;

    Tick tick = { 45, 1, Season::Spring, 45 };
    auto effects = resolver.resolve(peak, w, tick);
    assert(has_effect(effects, "Extreme Altitude"));
}

/* ---------------- Season tests ---------------- */
void spring_foraging_bonus() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = 15;
    w.precipitation = 0.3;
    w.wind_speed = 0.2;
    w.is_stormy = false;

    lwe::hex::Coord grass;
    bool found = false;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (!found && hex.biome == Biome::Grassland) { grass = c; found = true; }
    });

    if (!found) return;

    Tick tick = { 45, 1, Season::Spring, 45 };
    auto effects = resolver.resolve(grass, w, tick);
    assert(has_effect(effects, "Spring Growth"));
}

void summer_long_days() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = 22;
    w.precipitation = 0.2;
    w.wind_speed = 0.2;
    w.is_stormy = false;

    lwe::hex::Coord land;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (hex.biome != Biome::Ocean) land = c;
    });

    Tick tick = { 135, 1, Season::Summer, 45 };
    auto effects = resolver.resolve(land, w, tick);
    assert(has_effect(effects, "Long Days"));
}

void winter_short_days() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = 0;
    w.precipitation = 0.2;
    w.wind_speed = 0.3;
    w.is_stormy = false;

    lwe::hex::Coord temperate;
    bool found = false;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (!found && hex.climate == ClimateZone::Temperate) { temperate = c; found = true; }
    });

    if (!found) return;

    Tick tick = { 315, 1, Season::Winter, 45 };
    auto effects = resolver.resolve(temperate, w, tick);
    assert(has_effect(effects, "Short Days"));
}

/* ---------------- Ocean returns no effects ---------------- */
void ocean_no_effects() {
    EffectResolver resolver(test_world);
    HexWeather w;
    w.temperature = 15;
    w.precipitation = 0.5;
    w.wind_speed = 0.5;
    w.is_stormy = true;

    lwe::hex::Coord ocean;
    bool found = false;
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (!found && hex.biome == Biome::Ocean) { ocean = c; found = true; }
    });

    assert(found);
    Tick tick = { 45, 1, Season::Spring, 45 };
    auto effects = resolver.resolve(ocean, w, tick);
    assert(effects.empty());
}

/* ---------------- Print effects for a hex (visual demo) ---------------- */
void print_hex_effects(const WorldData& world, const EffectResolver& resolver,
                       const WeatherEngine& engine, const lwe::hex::Coord& coord, const Tick& tick)
{
    auto opt = world.grid.get(coord);
    if (!opt) return;
    const auto& hex = opt->get();
    const auto& w = engine.get(coord);
    auto effects = resolver.resolve(coord, w, tick);

    const char* biome_names[] = {
        "Ocean", "Coast", "Desert", "Grassland", "Forest", "Jungle", "Swamp",
        "Hills", "Mountain", "Peak", "AlpineForest", "CloudForest", "Beach", "Cliff"
    };

    std::print("\n  Hex ({},{}) {}\n", coord.q, coord.r, biome_names[static_cast<int>(hex.biome)]);
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
        std::print("    \033[33m{}\033[0m - {}\n", effect.name, effect.desctiption);
        for (const auto& mod : effect.mods) {
            std::print("      ");
            switch (mod.type) {
                case ModType::SkillCheck:       std::print("[Skill]"); break;
                case ModType::AttackRoll:       std::print("[Attack]"); break;
                case ModType::MovementSpeed:    std::print("[Speed]"); break;
                case ModType::Damage:           std::print("[Damage]"); break;
                case ModType::SavingThrow:      std::print("[Save]"); break;
                case ModType::Visibility:       std::print("[Vision]"); break;
                case ModType::DifficultTerrain: std::print("[Terrain]"); break;
            }
            std::print(" ");
            if (mod.modifier > 0) std::print("+");
            std::print("{} - {}\n", mod.modifier, mod.detail);
        }
    }
}

/* ---------------- Main ---------------- */
int main() {
    std::print("\n ---------------- EffectResolver Tests ---------------- \n\n");

    RUN(extreme_cold_triggers);
    RUN(freezing_triggers);
    RUN(extreme_heat_triggers);
    RUN(mild_weather_no_temperature_effects);
    RUN(downpour_triggers);
    RUN(heavy_rain_triggers);
    RUN(gale_triggers);
    RUN(strong_wind_triggers);
    RUN(thunderstorm_triggers);
    RUN(blizzard_triggers);
    RUN(sandstorm_triggers);
    RUN(storm_skips_rain_and_wind_checks);
    RUN(swamp_has_difficult_terrain);
    RUN(jungle_has_effects);
    RUN(river_crossing_triggers);
    RUN(raging_river_in_storm);
    RUN(peak_altitude_sickness);
    RUN(spring_foraging_bonus);
    RUN(summer_long_days);
    RUN(winter_short_days);
    RUN(ocean_no_effects);

    std::print("\n ---------------- All tests passed! ---------------- \n");

    /* Visual demo: show effects for a few interesting hexes */
    std::print("\n ---------------- Effect Demo ---------------- \n");

    WeatherEngine engine(test_world, 42);
    EffectResolver resolver(test_world);

    /* Winter storm */
    Tick winter = { 315, 1, Season::Winter, 45 };
    engine.update(winter);

    std::print("\nWinter (Day 315):\n");
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (hex.biome == Biome::Peak) {
            print_hex_effects(test_world, resolver, engine, c, winter);
        }
        if (hex.biome == Biome::Swamp) {
            static int swamp_count = 0;
            if (swamp_count++ == 0)
                print_hex_effects(test_world, resolver, engine, c, winter);
        }
    });

    /* Summer */
    Tick summer = { 135, 1, Season::Summer, 45 };
    engine.update(summer);

    std::print("/nSummer (Day 135):\n");
    test_world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (hex.biome == Biome::Desert) {
            static int desert_count = 0;
            if (desert_count++ == 0)
                print_hex_effects(test_world, resolver, engine, c, summer);
        }
        if (hex.has_river) {
            static int river_count = 0;
            if (river_count++ == 0)
                print_hex_effects(test_world, resolver, engine, c, summer);
        }
    });

    return 0;
}
