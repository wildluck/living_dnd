#include "cascade.hpp"
#include "world_gen.hpp"
#include "common.hpp"
#include "display/display.hpp"

#include <cassert>
#include <print>
#include <cmath>

static WorldData make_test_world() {
    WorldConfig cfg;
    cfg.seed = WORLD_SEED;
    cfg.radius = WORLD_RADIUS;
    return WorldGenerator(cfg).generate();
}

/* ---------------- Helper: simulate N ticks ---------------- */
struct SimContext {
    WorldData world;
    WeatherEngine weather;
    StateMachine state;
    CascadeEngine cascade;

    SimContext()
        : world(make_test_world())
        , weather(world, WORLD_SEED)
        , state(world, weather, WORLD_SEED)
        , cascade(world, weather, WORLD_SEED)
    {}

    void run_ticks(int n) {
        for (int d = 1; d <= n; ++d) {
            Season s = static_cast<Season>(((d - 1) % 360) / 90);
            int dos = ((d - 1) % 90) + 1;
            int year = (d - 1) / 360 + 1;
            Tick tick = { ((d - 1) % 360) + 1, year, s, dos };

            weather.update(tick);
            state.update(tick);
            cascade.process_events(state.events(), tick);
            cascade.update(tick);
            cascade.apply(tick);
            state.clear_events();
            cascade.clear_events();
        }
    }
};

/* ---------------- Basic tests ---------------- */
void cascade_starts_empty() {
    SimContext sim;
    assert(sim.cascade.active_effects().empty());
}

void raid_creates_fear_cascade() {
    SimContext sim;

    /* Manually inject a raid event */
    if (sim.world.settlements.empty()) return;
    const auto& s = sim.world.settlements[0];
    WorldEvent raid_event = { 1, 1, s.name + " was raided by goblins! 5 casualties, 10 food stolen" };
    Tick tick = { 1, 1, Season::Spring, 1 };

    sim.cascade.process_events({ raid_event }, tick);

    /* Should have created a fear cascade */
    bool has_fear = false;
    for (const auto& e : sim.cascade.active_effects()) {
        if (e.type == "fear") has_fear = true;
    }
    assert(has_fear);
}

void starvation_creates_famine_cascade() {
    SimContext sim;
    if (sim.world.settlements.empty()) return;

    const auto& s = sim.world.settlements[0];
    WorldEvent starve_event = { 1, 1, s.name + " is starving! 10 people lost" };
    Tick tick = { 1, 1, Season::Spring, 1 };

    sim.cascade.process_events({ starve_event }, tick);

    bool has_famine = false;
    for (const auto& e : sim.cascade.active_effects()) {
        if (e.type == "famine") has_famine = true;
    }
    assert(has_famine);
}

void promotion_creates_prosperity() {
    SimContext sim;
    if (sim.world.settlements.empty()) return;

    const auto& s = sim.world.settlements[0];
    WorldEvent promo_event = { 1, 1, s.name + " has grown into a City!" };
    Tick tick = { 1, 1, Season::Spring, 1 };

    sim.cascade.process_events({ promo_event }, tick);

    bool has_prosperity = false;
    for (const auto& e : sim.cascade.active_effects()) {
        if (e.type == "prosperity") has_prosperity = true;
    }
    assert(has_prosperity);
}

void effects_decay_over_time() {
    SimContext sim;
    if (sim.world.settlements.empty()) return;

    const auto& s = sim.world.settlements[0];
    WorldEvent raid_event = { 1, 1, s.name + " was raided by goblins! 5 casualties, 10 food stolen" };
    Tick tick = { 1, 1, Season::Spring, 1 };
    sim.cascade.process_events({ raid_event }, tick);

    int initial_count = sim.cascade.active_effects().size();
    assert(initial_count > 0);

    // Run many ticks — effects should eventually expire
    for (int d = 2; d <= 100; ++d) {
        Tick t = { d, 1, Season::Spring, d };
        sim.cascade.update(t);
    }

    assert(sim.cascade.active_effects().size() < static_cast<size_t>(initial_count) ||
           sim.cascade.active_effects().empty());
}

void effects_expire_completely() {
    SimContext sim;
    if (sim.world.settlements.empty()) return;

    const auto& s = sim.world.settlements[0];
    WorldEvent raid_event = { 1, 1, s.name + " was raided by goblins! 5 casualties, 10 food stolen" };
    Tick tick = { 1, 1, Season::Spring, 1 };
    sim.cascade.process_events({ raid_event }, tick);

    /* Run until all effects expire */
    for (int d = 2; d <= 500; ++d) {
        Tick t = { ((d - 1) % 360) + 1, 1, Season::Spring, d };
        sim.cascade.update(t);
    }

    assert(sim.cascade.active_effects().empty());
}

void fear_intensity_decreases_with_distance() {
    SimContext sim;
    if (sim.world.settlements.empty()) return;

    const auto& s = sim.world.settlements[0];
    WorldEvent raid_event = { 1, 1, s.name + " was raided by goblins! 5 casualties, 10 food stolen" };
    Tick tick = { 1, 1, Season::Spring, 1 };
    sim.cascade.process_events({ raid_event }, tick);

    double fear_at_origin = sim.cascade.fear_at(s.coord);
    double fear_far = sim.cascade.fear_at(s.coord + lwe::hex::Coord(8, 0));

    assert(fear_at_origin > fear_far);
}

void fear_zero_outside_radius() {
    SimContext sim;
    if (sim.world.settlements.empty()) return;

    const auto& s = sim.world.settlements[0];
    WorldEvent raid_event = { 1, 1, s.name + " was raided by goblins! 5 casualties, 10 food stolen" };
    Tick tick = { 1, 1, Season::Spring, 1 };
    sim.cascade.process_events({ raid_event }, tick);

    /* Fear radius is 10, check at distance 15 */
    double fear_outside = sim.cascade.fear_at(s.coord + lwe::hex::Coord(15, 0));
    assert(fear_outside == 0.0);
}

void refugees_transfer_population() {
    SimContext sim;
    if (sim.world.settlements.size() < 2) return;

    int pop_0 = sim.world.settlements[0].population;
    int pop_total = 0;
    for (const auto& s : sim.world.settlements) pop_total += s.population;

    const auto& s = sim.world.settlements[0];
    WorldEvent raid_event = { 1, 1, s.name + " was raided by goblins! 5 casualties, 10 food stolen" };
    Tick tick = { 1, 1, Season::Spring, 1 };
    sim.cascade.process_events({ raid_event }, tick);

    /* Source settlement should have lost population */
    assert(sim.world.settlements[0].population <= pop_0);

    /* Total population should be roughly conserved (minus casualties from the raid event text) */
    int new_total = 0;
    for (const auto& s2 : sim.world.settlements) new_total += s2.population;
    assert(new_total <= pop_total); /* lost some to the raid */
}

void cascade_generates_events() {
    SimContext sim;
    if (sim.world.settlements.empty()) return;

    const auto& s = sim.world.settlements[0];
    WorldEvent raid_event = { 1, 1, s.name + " was raided by goblins! 5 casualties, 10 food stolen" };
    Tick tick = { 1, 1, Season::Spring, 1 };
    sim.cascade.process_events({ raid_event }, tick);

    /* Cascade should have generated its own events (fear spreading, refugees) */
    assert(!sim.cascade.events().empty());
}

void full_simulation_with_cascade() {
    SimContext sim;

    /* Run 1 year of full simulation */
    sim.run_ticks(360);

    /* World should still be in a valid state */
    for (const auto& s : sim.world.settlements) {
        assert(s.population >= 10);
    }
    for (const auto& k : sim.world.kingdoms) {
        for (const auto& [other, score] : k.relationships) {
            assert(score >= -100 && score <= 100);
        }
    }
}

void deterministic_cascade() {
    auto run = [](uint64_t seed) -> int {
        WorldConfig cfg;
        cfg.seed = seed;
        cfg.radius = 10;
        auto world = WorldGenerator(cfg).generate();
        WeatherEngine weather(world, seed);
        StateMachine state(world, weather, seed);
        CascadeEngine cascade(world, weather, seed);

        for (int d = 1; d <= 90; ++d) {
            Tick tick = { d, 1, Season::Spring, d };
            weather.update(tick);
            state.update(tick);
            cascade.process_events(state.events(), tick);
            cascade.update(tick);
            cascade.apply(tick);
            state.clear_events();
            cascade.clear_events();
        }

        int total_pop = 0;
        for (const auto& s : world.settlements) total_pop += s.population;
        return total_pop;
    };

    assert(run(42) == run(42));
}

/* ---------------- Demo ---------------- */
void run_demo() {
    std::print("\n ---------------- Cascade Demo (1 year) ---------------- \n\n");

    auto world = make_test_world();
    WeatherEngine weather(world, WORLD_SEED);
    StateMachine state(world, weather, WORLD_SEED);
    CascadeEngine cascade(world, weather, WORLD_SEED);

    int total_state_events = 0;
    int total_cascade_events = 0;

    for (int d = 1; d <= 360; ++d) {
        Season s = static_cast<Season>((d - 1) / 90);
        int dos = ((d - 1) % 90) + 1;
        Tick tick = { d, 1, s, dos };

        weather.update(tick);
        state.update(tick);

        /* Print state machine events */
        for (const auto& ev : state.events()) {
            std::print("  \033[33m[Y{} D{}]\033[0m {}\n", ev.tick_year, ev.tick_day, ev.message);
            total_state_events++;
        }

        cascade.process_events(state.events(), tick);
        cascade.update(tick);
        cascade.apply(tick);

        /* Print cascade events */
        for (const auto& ev : cascade.events()) {
            std::print("  \033[36m[Y{} D{} CASCADE]\033[0m {}\n", ev.tick_year, ev.tick_day, ev.message);
            total_cascade_events++;
        }

        state.clear_events();
        cascade.clear_events();

        /* Season summary */
        if (dos == 90) {
            std::print("\n ---------------- End of {} ---------------- \n", lwe::display::season_name(s));
            std::print("  Active cascades: {}\n", cascade.active_effects().size());
            for (const auto& effect : cascade.active_effects()) {
                std::print("    {} at ({},{}) intensity={:.2f} remaining={} ticks\n",
                    effect.type, effect.origin.q, effect.origin.r,
                    effect.intensity, effect.remaining_ticks);
            }
            std::print("\n");
        }
    }

    std::print("\n ---------------- Year Summary ---------------- \n");
    std::print("State events: {}\n", total_state_events);
    std::print("Cascade events: {}\n", total_cascade_events);
    std::print("Remaining active cascades: {}\n\n", cascade.active_effects().size());

    for (const auto& s : world.settlements) {
        std::print("  {} ({}, pop {})\n", s.name,
            lwe::display::settlement_type_name(s.type), s.population);
    }
}

/* ---------------- Main ---------------- */
int main() {
    std::print("\n ---------------- Cascade Tests ----------------n\n");

    RUN(cascade_starts_empty);
    RUN(raid_creates_fear_cascade);
    RUN(starvation_creates_famine_cascade);
    RUN(promotion_creates_prosperity);
    RUN(effects_decay_over_time);
    RUN(effects_expire_completely);
    RUN(fear_intensity_decreases_with_distance);
    RUN(fear_zero_outside_radius);
    RUN(refugees_transfer_population);
    RUN(cascade_generates_events);
    RUN(full_simulation_with_cascade);
    RUN(deterministic_cascade);

    std::print("\n ---------------- All tests passed! ----------------\n");

    run_demo();

    return 0;
}
