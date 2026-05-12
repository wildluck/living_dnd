#pragma once

#include "world_data.hpp"
#include "event.hpp"
#include "dice.hpp"
#include "weather_engine.hpp"
#include "world_clock.hpp"

#include <vector>

class StateMachine {
public:
    StateMachine(WorldData& world, const WeatherEngine& weather, uint64_t seed);

    void update(const Tick& tick); /* Called every tick, mutates world */

    [[nodiscard]] const std::vector<WorldEvent>& events() const { return events_; }
    void clear_events() { events_.clear(); }

private:
    WorldData& world_;
    const WeatherEngine& weather_;
    RNG rng_;
    std::vector<WorldEvent> events_;

    void emit(const Tick& tick, const EventPayload& payload);

    void update_settlements(const Tick& tick);
    void update_kingdoms(const Tick& tick);
    void update_roads(const Tick& tick);
    void update_pois(const Tick& tick);
    void update_resources(const Tick& tick);
};
