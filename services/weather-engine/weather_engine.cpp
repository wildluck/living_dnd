#include "weather_engine.hpp"

#include <algorithm>

WeatherEngine::WeatherEngine(const WorldData& world, uint64_t seed)
    : world_(world), rng_(seed) {
        world_.grid.for_each([&](const lwe::hex::Coord& c, const HexData&) {
            weather_.set(c, HexWeather{});
        });
}

void WeatherEngine::update(const Tick& tick) {
    compute_base_temperature(tick);
    apply_elevation_modifier();
    propagate_wind(tick);
    compute_precipitation();
    generate_extreme_events(tick);
}

const HexWeather& WeatherEngine::get(const lwe::hex::Coord& coord) const {
    return weather_.at(coord);
}

/* Stage 1: Base temperature from climate zones and seasons */
double WeatherEngine::base_temp(ClimateZone zone, Season season) {
    static constexpr double table[8][4] = {
        { -10,  5, -5, -25 }, /* Arctic */
        {   0, 12,  2, -15 }, /* Subarctic */
        {  10, 22, 12,   0 }, /* Temperate */
        {  18, 30, 20,  10 }, /* Subtropical */
        {  25, 32, 26,  22 }, /* Tropical */
        {  20, 40, 22,  10 }, /* Arid */
        {  -5,  8, -2, -20 }, /* Alpine */
        {  12, 20, 14,   5 }, /* Maritime */
    };

    int zone_idx = static_cast<int>(zone);
    int season_idx = static_cast<int>(season);
    return table[zone_idx][season_idx];
}

double WeatherEngine::season_progress(const Tick& tick) {
    return static_cast<double>(tick.day_of_season - 1) / static_cast<double>(WorldClock::DAYS_PER_SEASON - 1);
}

void WeatherEngine::compute_base_temperature(const Tick& tick) {
    Season current = tick.season;
    Season next = static_cast<Season>((static_cast<int>(current) + 1) % 4);
    double progress = season_progress(tick);

    weather_.for_each([&](const lwe::hex::Coord& c, HexWeather& w) {
        auto opt = world_.grid.get(c);
        if (!opt) return;
        const auto& hex = opt->get();

        if (hex.biome == Biome::Ocean) {
            double base = base_temp(ClimateZone::Maritime, current);
            double nxt = base_temp(ClimateZone::Maritime, next);
            w.temperature = base + (nxt - base) * progress;
            return;
        }

        double base = base_temp(hex.climate, current);
        double nxt = base_temp(hex.climate, next);
        w.temperature = base + (nxt - base) * progress;

        w.temperature += rng_.rand_float(-2.0, 2.0);
    });
}

/* Stage 2: Elevation modifier */
void WeatherEngine::apply_elevation_modifier() {
    /* Temperature drops ~6.5 degree C per 1000m of elevation
     * Our elevation os 0-1, treat 1.0 as ~4000m
     * So modifier = elevation * 4000 * 6.5 / 1000 = elevation * 26
     */
    weather_.for_each([&](const lwe::hex::Coord& c, HexWeather& w) {
        auto opt = world_.grid.get(c);
        if (!opt) return;
        const auto& hex = opt->get();

        if (hex.biome == Biome::Ocean) return;

        double above_sea = std::max(0.0, static_cast<double>(hex.elevation) - 0.35);
        w.temperature -= above_sea * 26.0;
    });
}

/* Stage 3: Wind propagation (simple cellular automata) */
void WeatherEngine::propagate_wind(const Tick& tick) {
    /* Prevailing wind direction rotates with seasons
     * Spring: East, Summer: SouthEast, Autumn: West, Winter: NorthWest
     */
    using Dir = lwe::hex::Direction;
    static const Dir prevailing[] = {
        Dir::East,      /* Spring */
        Dir::SouthEast, /* Summer */
        Dir::West,      /* Autumn */
        Dir::NorthEast, /* Winter */
    };

    Dir base_dir = prevailing[static_cast<int>(tick.season)];

    weather_.for_each([&](const lwe::hex::Coord& c, HexWeather& w) {
        auto opt = world_.grid.get(c);
        if (!opt) return;;
        const auto& hex = opt->get();

        /* Base wind direction with random variation */
        int dir_offset = rng_.rand_int(-1, 1);
        int dir_val = (static_cast<int>(base_dir) + dir_offset + 6) % 6;
        w.wind_dir = static_cast<Dir>(dir_val);

        /* Wind speed based on terrain
         * Open terrain (grassland, desert, ocean) = high_wind
         * Sheltered terrin (forest, jungle, mountain) = low wind
         */
        switch (hex.biome) {
        case Biome::Ocean:
        case Biome::Desert:
            w.wind_speed = 0.5 + rng_.rand_float(0.0, 0.3);
            break;
        case Biome::Grassland:
            w.wind_speed = 0.4 + rng_.rand_float(0.0, 0.3);
            break;
        case Biome::Hills:
            w.wind_speed = 0.3 + rng_.rand_float(0.0, 0.3);
            break;
        case Biome::Forest:
        case Biome::Jungle:
        case Biome::AlpineForest:
        case Biome::CloudForest:
            w.wind_speed = 0.1 + rng_.rand_float(0.0, 0.2);
            break;
        case Biome::Mountain:
        case Biome::Peak:
            w.wind_speed = 0.6 + rng_.rand_float(0.0, 0.4);
            break;
        case Biome::Swamp:
            w.wind_speed = 0.1 + rng_.rand_float(0.0, 0.15);
            break;
        default:
            w.wind_speed = 0.3 + rng_.rand_float(0.0, 0.2);
            break;
        }

        w.wind_speed = std::clamp(w.wind_speed, 0.0, 0.1);
    });
}

/* Stage 4: Precepitation */
void WeatherEngine::compute_precipitation() {
    weather_.for_each([&](const lwe::hex::Coord& c, HexWeather& w) {
        auto opt = world_.grid.get(c);
        if (!opt) return;
        const auto& hex = opt->get();

        double base_precip = 0.0;

        /* Moisture  from biome */
        switch (hex.biome) {
        case Biome::Ocean:        base_precip =  0.3; break;
        case Biome::Swamp:        base_precip =  0.6; break;
        case Biome::Jungle:       base_precip =  0.7; break;
        case Biome::Forest:       base_precip =  0.4; break;
        case Biome::CloudForest:  base_precip =  0.6; break;
        case Biome::Grassland:    base_precip =  0.3; break;
        case Biome::Desert:       base_precip = 0.05; break;
        case Biome::Hills:        base_precip =  0.3; break;
        case Biome::Mountain:     base_precip =  0.4; break;
        case Biome::Peak:         base_precip =  0.3; break;
        case Biome::AlpineForest: base_precip = 0.35; break;
        default:                  base_precip =  0.2; break;
        }

        /* Coast proximity encreases precipitation */
        bool near_coast = false;
        for (const auto& n : c.neighbors()) {
            auto nopt = world_.grid.get(n);
            if (nopt && nopt->get().biome == Biome::Ocean) {
                near_coast = true;
                break;
            }
        }
        if (near_coast) base_precip += 0.15;

        /* River proximity increases precipitation */
        if (hex.has_river) base_precip += 0.1;

        /* Temperature affects precipitation type and amount
         * Cold = less moisture in air but still precipitaes (snow)
         */
        if (w.temperature < 0) {
            base_precip *= 0.7; /* Less total precipitation when cold */
        }

        /* Wind brings moisture inland */
        base_precip += w.wind_speed * 0.1;

        /* Random daily vatiation */
        base_precip += rng_.rand_float(-0.1, 0.15);

        w.precipitation = std::clamp(base_precip, 0.0, 1.0);
    });
}

/* Stage 5: Extreme weather events (Markov-inspired) */
void WeatherEngine::generate_extreme_events(const Tick& tick) {
    weather_.for_each([&](const lwe::hex::Coord& c, HexWeather& w) {
        auto opt = world_.grid.get(c);
        if (!opt) return;
        const auto& hex = opt->get();

        w.is_stormy = false;

        /* Base storm probability depends on climate and season */
        double storm_chance = 0.02; /* 2% base chance */

        switch (hex.climate) {
        case ClimateZone::Tropical: /* Monsoon season in summer */
            storm_chance = (tick.season == Season::Summer) ? 0.15 : 0.05;
            break;
        case ClimateZone::Maritime: /* Coastal storms in autumn/winter */
            storm_chance = (tick.season == Season::Autumn || tick.season == Season::Winter) ? 0.1 : 0.03;
            break;
        case ClimateZone::Arctic: /* Blizzards in winter */
        case ClimateZone::Subarctic:
            storm_chance = (tick.season == Season::Winter) ? 0.12 : 0.02;
            break;
        case ClimateZone::Arid: /* Sandstorm in summer */
            storm_chance = (tick.season == Season::Summer) ? 0.08 : 0.02;
            break;
        case ClimateZone::Alpine: /* Mountain storms year-round */
            storm_chance = 0.06;
            break;
        default:
            storm_chance = 0.03;
            break;
        }

        /* High wind increases storm chance */
        storm_chance += w.wind_speed * 0.05;

        /* High precipitation increases storm chance */
        storm_chance += w.precipitation * 0.05;

        /* Roll for storm */
        if (rng_.rand_bool(storm_chance)) {
            w.is_stormy = true;
            /* Storm increases wind and precipitation */
            w.wind_speed = std::clamp(w.wind_speed + 0.3, 0.0, 1.0);
            w.precipitation = std::clamp(w.precipitation + 0.3, 0.0, 1.0);
            /* Temperature drops during storms */
            w.temperature -= rng_.rand_float(2.0, 8.0);
        }

        /* Neighboring storms can spread (simple contagion) */
        if (!w.is_stormy) {
            int stormy_neighbors = 0;
            for (const auto& n : c.neighbors()) {
                auto wopt = weather_.get(n);
                if (wopt && wopt->get().is_stormy) stormy_neighbors++;
            }
            /* 15% chance per stormy neighbor */
            if (rng_.rand_bool(stormy_neighbors * 0.15)) {
                w.is_stormy = true;
                w.wind_speed = std::clamp(w.wind_speed + 0.2, 0.0, 1.0);
                w.precipitation = std::clamp(w.precipitation + 0.2, 0.0, 1.0);
            }
        }
    });
}
