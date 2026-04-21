#pragma once

#include "common/world_data.hpp"
#include "weather_engine.hpp"
#include "state_machine.hpp"
#include "hex/coord.hpp"

#include <string>
#include <vector>

struct CascadeEffect {
    lwe::hex::Coord origin;
    int radius;
    int remaining_ticks;
    std::string type; /* "fear", "refugees", "trade_disruption" */

    double intensity; /* 1.0 at origin, decays with distance and time */
    int source_settlement; /* settlement id that triggered it, -1 if none */
};

class CascadeEngine {
public:
    CascadeEngine(WorldData& world, const WeatherEngine& weather, uint64_t seed);

    /* Process events from state machine and create cascade */
    void process_events(const std::vector<WorldEvent>& events, const Tick& tick);

    /* Update active cascades (decay, spread effects) */
    void update(const Tick& tick);

    /* Apply cascade effects to world state */
    void apply(const Tick& tick);

    [[nodiscard]] const std::vector<CascadeEffect>& active_effects() const { return effects_; }
    [[nodiscard]] const std::vector<WorldEvent>& events() const { return events_; }
    void clear_events() { events_.clear(); }

    /* Query: total cascade intensity at a coord */
    [[nodiscard]] double fear_at(const lwe::hex::Coord& c) const;
    [[nodiscard]] double prosperity_at(const lwe::hex::Coord& c) const;
    [[nodiscard]] double disruption_at(const lwe::hex::Coord& c) const;

private:
    WorldData& world_;
    const WeatherEngine& weather_;
    RNG rng_;
    std::vector<CascadeEffect> effects_;
    std::vector<WorldEvent> events_;

    void emit(const Tick& tick, const std::string& msg);

    /* Event handlers - parse event message and create appropriate cascade */
    void on_raid(const WorldEvent& event, const Tick& tick);
    void on_starvation(const WorldEvent& event, const Tick& tick);
    void on_war(const WorldEvent& event, const Tick& tick);
    void on_promotion(const WorldEvent& event, const Tick& tick);
    void on_storm_damage(const WorldEvent& event, const Tick& tick);

    /* Hlelpers */
    int find_settlement_by_name(const std::string& name_fragment) const;
    double intensity_at(const lwe::hex::Coord& origin, const lwe::hex::Coord& target,
                        int radius, double base_intensity) const;
};
