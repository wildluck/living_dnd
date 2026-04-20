#include "state_machine.hpp"
#include "world_gen.hpp"
#include "display/display.hpp"
#include "common.hpp"

#include <cassert>
#include <print>

static WorldData make_test_world() {
    WorldConfig cfg;
    cfg.seed = WORLD_SEED;
    cfg.radius = WORLD_RADIUS;
    return WorldGenerator(cfg).generate();
}

/* ---------------- Settlement tests ---------------- */
void settlements_grow_over_time() {
    auto world = make_test_world();
    WeatherEngine weather(world, WORLD_SEED);
    StateMachine sm(world, weather, WORLD_SEED);

    /* Record initial populations */
    std::vector<int> initial_pops;
    for (const auto& s : world.settlements) {
        initial_pops.push_back(s.population);
    }

    /* Run 90 ticks (one season) of mild spring weather */
    for (int d = 1; d <= 90; ++d) {
        Tick tick = { d, 1, Season::Spring, d };
        weather.update(tick);
        sm.update(tick);
    }

    /* At least some settlements should have grown */
    int grew = 0;
    for (size_t i = 0; i < world.settlements.size(); ++i) {
        if (world.settlements[i].population > initial_pops[i]) grew++;
    }
    assert(grew > 0);
}

void settlements_never_below_minimum() {
    auto world = make_test_world();
    WeatherEngine weather(world, WORLD_SEED);
    StateMachine sm(world, weather, WORLD_SEED);

    /* Run a full year */
    for (int d = 1; d <= 360; ++d) {
        Season s = static_cast<Season>((d - 1) / 90);
        int dos = ((d - 1) % 90) + 1;
        Tick tick = { d, 1, s, dos };
        weather.update(tick);
        sm.update(tick);
    }

    for (const auto& s : world.settlements) {
        assert(s.population >= 10);
    }
}

void settlement_type_changes_with_population() {
    auto world = make_test_world();
    WeatherEngine weather(world, WORLD_SEED);
    StateMachine sm(world, weather, WORLD_SEED);

    /* Force a hamlet to have a huge population and run a tick */
    if (!world.settlements.empty()) {
        world.settlements[0].population = 6000;
        world.settlements[0].type = SettlementType::Hamlet;

        Tick tick = { 1, 1, Season::Spring, 1 };
        weather.update(tick);
        sm.update(tick);

        /* Should be promoted to City */
        assert(world.settlements[0].type == SettlementType::City);
    }
}

void garrison_scales_with_population() {
    auto world = make_test_world();
    WeatherEngine weather(world, WORLD_SEED);
    StateMachine sm(world, weather, WORLD_SEED);

    Tick tick = { 1, 1, Season::Spring, 1 };
    weather.update(tick);
    sm.update(tick);

    for (const auto& s : world.settlements) {
        assert(s.garrison == s.population / 10);
    }
}

/* ---------------- Kingdom tests ---------------- */
void kingdom_relations_change() {
    auto world = make_test_world();
    WeatherEngine weather(world, WORLD_SEED);
    StateMachine sm(world, weather, WORLD_SEED);

    if (world.kingdoms.size() < 2) return;

    int initial_score = world.kingdoms[0].relationships[1];

    /* Run several seasons (relations update on day_of_season == 1) */
    for (int d = 1; d <= 360; ++d) {
        Season s = static_cast<Season>((d - 1) / 90);
        int dos = ((d - 1) % 90) + 1;
        Tick tick = { d, 1, s, dos };
        weather.update(tick);
        sm.update(tick);
    }

    int final_score = world.kingdoms[0].relationships[1];
    /* Relations should have cahged (drift, events, border friction)
     * Could go either direction, but should not stay exaclty the same after a year
     * (statistically very unlikety with random event)
     */
    assert(final_score != initial_score || world.kingdoms.size() < 2);
}

void kingdom_relations_stay_clamped() {
    auto world = make_test_world();
    WeatherEngine weather(world, WORLD_SEED);
    StateMachine sm(world, weather, WORLD_SEED);

    /* Run for 3 years */
    for (int y = 1; y <= 3; ++y) {
        for (int d = 1; d <= 360; ++d) {
            Season s = static_cast<Season>((d - 1) / 90);
            int dos = ((d - 1) % 90) + 1;
            Tick tick = { d, y, s, dos };
            weather.update(tick);
            sm.update(tick);
        }
    }

    for (const auto& k : world.kingdoms) {
        for (const auto& [other, score] : k.relationships) {
            assert(score >= -100 && score <= 100);
        }
    }
}

/* ---------------- Road tests ---------------- */
void roads_can_decay() {
    auto world = make_test_world();
    WeatherEngine weather(world, WORLD_SEED);
    StateMachine sm(world, weather, WORLD_SEED);

    /* Count initial roads */
    int initial_roads = 0;
    world.grid.for_each([&](const lwe::hex::Coord&, const HexData& hex) {
        if (hex.terrain == Terrain::Road) initial_roads++;
    });

    /* Run 5 years — some roads should decay */
    for (int y = 1; y <= 5; ++y) {
        for (int d = 1; d <= 360; ++d) {
            Season s = static_cast<Season>((d - 1) / 90);
            int dos = ((d - 1) % 90) + 1;
            Tick tick = { d, y, s, dos };
            weather.update(tick);
            sm.update(tick);
        }
    }

    int final_roads = 0;
    world.grid.for_each([&](const lwe::hex::Coord&, const HexData& hex) {
        if (hex.terrain == Terrain::Road) final_roads++;
    });

    /* Trade routes maintain their roads, but unused roads may decay */
    assert(final_roads <= initial_roads);
}

/* ---------------- POI tests ---------------- */
void pois_can_raid_settlements() {
    auto world = make_test_world();
    WeatherEngine weather(world, WORLD_SEED);
    StateMachine sm(world, weather, WORLD_SEED);

    /* Run for 2 years — should see at least one raid event */
    bool found_raid = false;
    for (int y = 1; y <= 2; ++y) {
        for (int d = 1; d <= 360; ++d) {
            Season s = static_cast<Season>((d - 1) / 90);
            int dos = ((d - 1) % 90) + 1;
            Tick tick = { d, y, s, dos };
            weather.update(tick);
            sm.update(tick);

            for (const auto& ev : sm.events()) {
                if (ev.message.find("raid") != std::string::npos ||
                    ev.message.find("repelled") != std::string::npos) {
                    found_raid = true;
                }
            }
            sm.clear_events();
        }
    }
    assert(found_raid);
}

/* ---------------- Resource tests ---------------- */
void resources_change_over_time() {
    auto world = make_test_world();
    WeatherEngine weather(world, WORLD_SEED);
    StateMachine sm(world, weather, WORLD_SEED);

    if (world.settlements.empty()) return;

    /* Record initial food for first settlement that has it */
    int initial_food = -1;
    int target_idx = -1;
    for (size_t i = 0; i < world.settlements.size(); ++i) {
        if (world.settlements[i].supply.contains(Resource::Food)) {
            initial_food = world.settlements[i].supply[Resource::Food];
            target_idx = static_cast<int>(i);
            break;
        }
    }
    if (target_idx < 0) return;

    /* Run 30 days */
    for (int d = 1; d <= 30; ++d) {
        Tick tick = { d, 1, Season::Spring, d };
        weather.update(tick);
        sm.update(tick);
    }

    int final_food = world.settlements[target_idx].supply[Resource::Food];
    assert(final_food != initial_food); /* should have changed */
}

/* ---------------- Event generation ---------------- */
void events_are_generated() {
    auto world = make_test_world();
    WeatherEngine weather(world, WORLD_SEED);
    StateMachine sm(world, weather, WORLD_SEED);

    int total_events = 0;
    for (int y = 1; y <= 2; ++y) {
        for (int d = 1; d <= 360; ++d) {
            Season s = static_cast<Season>((d - 1) / 90);
            int dos = ((d - 1) % 90) + 1;
            Tick tick = { d, y, s, dos };
            weather.update(tick);
            sm.update(tick);
            total_events += sm.events().size();
            sm.clear_events();
        }
    }

    /* Over 2 years, there should be some events */
    assert(total_events > 0);
}

void deterministic_simulation() {
    auto run = [](uint64_t seed) -> int {
        WorldConfig cfg;
        cfg.seed = seed;
        cfg.radius = 10;
        auto world = WorldGenerator(cfg).generate();
        WeatherEngine weather(world, seed);
        StateMachine sm(world, weather, seed);

        for (int d = 1; d <= 90; ++d) {
            Tick tick = { d, 1, Season::Spring, d };
            weather.update(tick);
            sm.update(tick);
        }

        int total_pop = 0;
        for (const auto& s : world.settlements) total_pop += s.population;
        return total_pop;
    };

    int run1 = run(42);
    int run2 = run(42);
    assert(run1 == run2);
}

/* ---------------- Visual demo ---------------- */
void run_demo() {
    std::print("\n ---------------- State Machine Demo (2 years) ---------------- \n\n");

    auto world = make_test_world();
    WeatherEngine weather(world, WORLD_SEED);
    StateMachine sm(world, weather, WORLD_SEED);

    /* Print initial state */
    std::print("Initial state:\n");
    for (const auto& s : world.settlements) {
        std::print("  {} ({}, pop {})\n", s.name,
            lwe::display::settlement_type_name(s.type), s.population);
    }

    /* Simulate 2 years */
    for (int y = 1; y <= 2; ++y) {
        for (int d = 1; d <= 360; ++d) {
            Season s = static_cast<Season>((d - 1) / 90);
            int dos = ((d - 1) % 90) + 1;
            Tick tick = { d, y, s, dos };
            weather.update(tick);
            sm.update(tick);

            /* Print significant events */
            for (const auto& ev : sm.events()) {
                std::print("  [Y{} D{}] {}\n", ev.tick_year, ev.tick_day, ev.message);
            }
            sm.clear_events();
        }

        /* End of year summary */
        std::print("\n--- End of Year {} ---\n", y);
        for (const auto& s : world.settlements) {
            std::print("  {} ({}, pop {})\n", s.name,
                lwe::display::settlement_type_name(s.type), s.population);
        }

        if (!world.kingdoms.empty()) {
            std::print("\nKingdom relations:\n");
            for (size_t i = 0; i < world.kingdoms.size(); ++i) {
                for (const auto& [other, score] : world.kingdoms[i].relationships) {
                    if (other > static_cast<int>(i)) {
                        std::print("  {} <-> {}: {}\n",
                            world.kingdoms[i].name,
                            world.kingdoms[other].name, score);
                    }
                }
            }
        }
    }
}

/* ---------------- Main ---------------- */
int main() {
    std::print("\n ---------------- StateMachine Tests ----------------\n\n");

    RUN(settlements_grow_over_time);
    RUN(settlements_never_below_minimum);
    RUN(settlement_type_changes_with_population);
    RUN(garrison_scales_with_population);
    RUN(kingdom_relations_change);
    RUN(kingdom_relations_stay_clamped);
    RUN(roads_can_decay);
    RUN(pois_can_raid_settlements);
    RUN(resources_change_over_time);
    RUN(events_are_generated);
    RUN(deterministic_simulation);

    std::print("\n ---------------- All tests passed! ----------------\n");

    run_demo();

    return 0;
}
