#include "weather_engine.hpp"
#include "world_gen.hpp"
#include "display/display.hpp"
#include "common.hpp"

#include <cassert>
#include <cstdlib>
#include <print>

using namespace lwe::hex;
using namespace lwe::display;

/* Shared world for tests */
static WorldData make_test_world() {
    WorldConfig cfg;
    cfg.seed = WORLD_SEED;
    cfg.radius = WORLD_RADIUS;
    return WorldGenerator(cfg).generate();
}

static WorldData test_world = make_test_world();

/* ---------------- Basic tests ---------------- */
void weather_initialized() {
    WeatherEngine engine(test_world, WORLD_SEED);
    /* Every hex should have weather */
    test_world.grid.for_each([&](const Coord& c, const HexData&) {
        (void)engine.get(c);
    });
}

void temperature_varies_by_season() {
    WeatherEngine engine(test_world, WORLD_SEED);

    /* Find a temperate hex */
    Coord temperate_hex;
    bool found = false;
    test_world.grid.for_each([&](const Coord& c, const HexData& hex) {
        if (!found && hex.climate == ClimateZone::Temperate) {
            temperate_hex = c;
            found = true;
        }
    });
    assert(found);

    /* Summer should be warmer than winter */
    Tick summer_tick = { 135, 1, Season::Summer, 45 };
    engine.update(summer_tick);
    double summer_temp = engine.get(temperate_hex).temperature;

    Tick winter_tick = { 315, 1, Season::Winter, 45 };
    engine.update(winter_tick);
    double winter_temp = engine.get(temperate_hex).temperature;

    assert(summer_temp > winter_temp);
}

void elevation_cools_temperature() {
    WeatherEngine engine(test_world, WORLD_SEED);

    /* Find a lowland and a highland hex in same climate zone */
    Coord low_hex, high_hex;
    bool found_low = false, found_high = false;
    test_world.grid.for_each([&](const Coord& c, const HexData& hex) {
        if (hex.biome == Biome::Ocean) return;
        if (!found_low && hex.elevation < 0.4) { low_hex = c; found_low = true; }
        if (!found_high && hex.elevation > 0.7) { high_hex = c; found_high = true; }
    });

    if (!found_low || !found_high) return; /* skip if world does not have both */

    Tick tick = { 45, 1, Season::Spring, 45 };
    engine.update(tick);

    /* Higher elevation should be colder (on averagem accounting for random variance)
     * Run multiple times to account for randomness
     */
    double low_total = 0, high_total = 0;
    for (int i = 0; i < 10; ++i) {
        WeatherEngine e(test_world, i);
        e.update(tick);
        low_total += e.get(low_hex).temperature;
        high_total += e.get(high_hex).temperature;
    }
    assert(low_total / 10 > high_total / 10);
}

void precipitation_in_range() {
    WeatherEngine engine(test_world, WORLD_SEED);
    Tick tick = { 45, 1, Season::Spring, 45 };
    engine.update(tick);

    test_world.grid.for_each([&](const Coord& c, const HexData&) {
        double ws = engine.get(c).precipitation;
        assert(ws >= 0.0 && ws <= 1.0);
    });
}

void wind_speed_in_range() {
    WeatherEngine engine(test_world, WORLD_SEED);
    Tick tick = { 45, 1, Season::Spring, 45 };
    engine.update(tick);

    test_world.grid.for_each([&](const Coord& c, const HexData&) {
        double ws = engine.get(c).wind_speed;
        assert(ws >= 0.0 && ws <= 1.0);
    });
}

void desert_is_dry() {
    WeatherEngine engine(test_world, WORLD_SEED);
    Tick tick = { 45, 1, Season::Spring, 45 };
    engine.update(tick);

    double desert_precip = 0, forest_precip = 0;
    int desert_count = 0, forest_count = 0;

    test_world.grid.for_each([&](const Coord& c, const HexData& hex) {
        if (hex.biome == Biome::Desert) {
            desert_precip += engine.get(c).precipitation;
            desert_count++;
        }
        if (hex.biome == Biome::Forest) {
            forest_precip += engine.get(c).precipitation;
            forest_count++;
        }
    });

    if (desert_count > 0 && forest_count > 0) {
        assert(desert_precip / desert_count < forest_precip / desert_count);
    }
}

void storms_exist() {
    WeatherEngine engine(test_world, WORLD_SEED);
    /* Run many ticks to find at least one storm */
    bool found_storm = false;
    for (int day = 1; day <= 360; ++day) {
        Season s = static_cast<Season>((day - 1) / 90);
        int dos = ((day - 1) % 90) + 1;
        Tick tick { day, 1, s, dos };
        engine.update(tick);

        test_world.grid.for_each([&](const Coord& c, const HexData&) {
            if (engine.get(c).is_stormy) found_storm = true;
        });
        if (found_storm) break;
    }
    assert(found_storm);
}

void deterministic() {
    Tick tick = { 45, 1, Season::Spring, 45 };

    WeatherEngine e1(test_world, WORLD_SEED);
    e1.update(tick);

    WeatherEngine e2(test_world, WORLD_SEED);
    e2.update(tick);

    test_world.grid.for_each([&](const Coord& c, const HexData&) {
        assert(e1.get(c).temperature == e2.get(c).temperature);
        assert(e1.get(c).precipitation == e2.get(c).precipitation);
    });
}

void different_seeds_differ() {
    Tick tick = { 45, 1, Season::Spring, 45 };

    WeatherEngine e1(test_world, WORLD_SEED);
    e1.update(tick);

    WeatherEngine e2(test_world, WORLD_SEED + 100);
    e2.update(tick);

    int different = 0;
    test_world.grid.for_each([&](const Coord& c, const HexData&) {
        if (std::abs(e1.get(c).temperature - e2.get(c).temperature) > 0.01) different++;
    });
    assert(different > 10);
}

/* ---------------- Main ---------------- */
int main() {
    std::print("\n ---------------- WeatherEngine Tests ---------------- \n\n");

    RUN(weather_initialized);
    RUN(temperature_varies_by_season);
    RUN(elevation_cools_temperature);
    RUN(precipitation_in_range);
    RUN(wind_speed_in_range);
    RUN(desert_is_dry);
    RUN(storms_exist);
    RUN(deterministic);
    RUN(different_seeds_differ);

    std::print("\n ---------------- All tests passed! ---------------- \n");

    /* Visual demo: show weather at different seasons */
    WeatherEngine engine(test_world, WORLD_SEED);

    /* Spring */
    Tick spring = { 45, 1, Season::Spring, 45 };
    engine.update(spring);
    print_weather_map(test_world, engine, spring);

    /* Summer */
    Tick summer = { 135, 1, Season::Summer, 45 };
    engine.update(summer);
    print_weather_map(test_world, engine, summer);

    /* Autumn */
    Tick autumn = { 225, 1, Season::Autumn, 45 };
    engine.update(autumn);
    print_weather_map(test_world, engine, autumn);

    /* Winter */
    Tick winter = { 315, 1, Season::Winter, 45 };
    engine.update(winter);
    print_weather_map(test_world, engine, winter);

    return 0;
}
