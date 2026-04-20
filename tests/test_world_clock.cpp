#include "world_clock.hpp"

#include <cassert>
#include <print>

#define RUN(name) do { std::print("  " #name "... "); name(); std::print("OK\n"); } while (0)

void initial_state() {
    WorldClock clock;
    auto t = clock.current();
    assert(t.day == 1);
    assert(t.year == 1);
    assert(t.season == Season::Spring);
    assert(t.day_of_season == 1);
}

void tick_advances_day() {
    WorldClock clock;
    auto t = clock.tick();
    assert(t.day == 2);
    assert(t.year == 1);
}

void season_changes() {
    WorldClock clock;
    for (int i = 0; i < 90; ++i) clock.tick();
    auto t = clock.current();
    assert(t.season == Season::Summer);
    assert(t.day_of_season == 1);
    assert(t.day == 91);
}

void year_wraps() {
    WorldClock clock;
    for (int i = 0; i < 360; ++i) clock.tick();
    auto t = clock.current();
    assert(t.day == 1);
    assert(t.year == 2);
    assert(t.season == Season::Spring);
}

void full_year_seasons() {
    WorldClock clock;
    /* Spring: days 1-90 */
    assert(clock.current().season == Season::Spring);
    for (int i = 0; i < 89; ++i) clock.tick();
    assert(clock.current().season == Season::Spring);
    assert(clock.current().day == 90);

    clock.tick(); /* day 91 */
    assert(clock.current().season == Season::Summer);

    for (int i = 0; i < 89; ++i) clock.tick(); // day 180
    assert(clock.current().season == Season::Summer);

    clock.tick(); /* day 181 */
    assert(clock.current().season == Season::Autumn);

    for (int i = 0; i < 89; ++i) clock.tick(); // day 270
    assert(clock.current().season == Season::Autumn);

    clock.tick(); /* day 271 */
    assert(clock.current().season == Season::Winter);
}

void on_tick_callback() {
    WorldClock clock;
    int count = 0;
    clock.on_tick([&](const Tick&) { ++count; });
    for (int i = 0; i < 10; ++i) clock.tick();
    assert(count == 10);
}

void on_season_change_callback() {
    WorldClock clock;
    int count = 0;
    clock.on_season_change([&](const Tick&) { ++count; });
    for (int i = 0; i < 360; ++i) clock.tick();
    assert(count == 4); /* Spring->Summer->Autumn->Winter->Spring */
}

void on_year_change_callback() {
    WorldClock clock;
    int count = 0;
    clock.on_year_change([&](const Tick&) { ++count; });
    for (int i = 0; i < 720; ++i) clock.tick();
    assert(count == 2);
}

void reset_test() {
    WorldClock clock;
    for (int i = 0; i < 100; ++i) clock.tick();
    clock.reset();
    auto t = clock.current();
    assert(t.day == 1);
    assert(t.year == 1);
}

void multi_year_consistency() {
    WorldClock clock;
    for (int i = 0; i < 3600; ++i) clock.tick(); // 10 years
    auto t = clock.current();
    assert(t.day == 1);
    assert(t.year == 11);
    assert(t.season == Season::Spring);
}

int main() {
    std::print("\n---------------- WorldClock Tests ----------------\n\n");
    RUN(initial_state);
    RUN(tick_advances_day);
    RUN(season_changes);
    RUN(year_wraps);
    RUN(full_year_seasons);
    RUN(on_tick_callback);
    RUN(on_season_change_callback);
    RUN(on_year_change_callback);
    RUN(reset_test);
    RUN(multi_year_consistency);
    std::print("\n---------------- All tests passed! ----------------\n\n");
    return 0;
}
