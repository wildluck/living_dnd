#pragma once

#include "common/world_data.hpp"
#include "dice/dice.hpp"
#include "world_clock.hpp"

#include <cstdint>

struct HexWeather {
    double temperature   = 15.0; /* Celsius */
    double precipitation =  0.0; /* 0.0-1.0 (chance of rain/snow) */
    double wind_speed    =  0.0; /* 0.0-1.0 */
    lwe::hex::Direction wind_dir = lwe::hex::Direction::East;
    bool is_stormy = false;
};

class WeatherEngine {
public:
    WeatherEngine(const WorldData& world, uint64_t seed);

    void update(const Tick& tick);
    [[nodiscard]] const HexWeather& get(const lwe::hex::Coord& coord) const;
    [[nodiscard]] const lwe::hex::HexGrid<HexWeather>& grid() const { return weather_; }

private:
    const WorldData& world_;
    lwe::hex::HexGrid<HexWeather> weather_;
    RNG rng_;

    void compute_base_temperature(const Tick& tick);
    void apply_elevation_modifier();
    void propagate_wind(const Tick& tick);
    void compute_precipitation();
    void generate_extreme_events(const Tick& tick);

    static double base_temp(ClimateZone zone, Season season);
    static double season_progress(const Tick& tick);
};
