#include "world_gen.hpp"
#include "world_clock.hpp"
#include "weather_engine.hpp"

#include <print>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

static std::atomic<bool> running{true};

void signal_handler(int) { running = false; }

void clear_screen() { std::print("\033[2J\033[H"); }

const char* season_name(Season s) {
    switch (s) {
        case Season::Spring: return "Spring";
        case Season::Summer: return "Summer";
        case Season::Autumn: return "Autumn";
        case Season::Winter: return "Winter";
    }
    return "?";
}

void render(const WorldData& world, const WeatherEngine& engine, const Tick& tick) {
    clear_screen();

    int radius = world.radius;

    /* Header */
    std::print("\033[1m Living World Engine \033[0m");
    std::print("  Year {} | {} | Day {}/90 (total day {}) [Ctrl+C to quit]\n\n",
        tick.year, season_name(tick.season), tick.day_of_season, tick.day);


    /* Map */
    for (int r = -radius; r <= radius; ++r) {
        int q_min = std::max(-radius, -radius - r);
        int q_max = std::min(radius, radius - r);
        for (int i = 0; i < std::abs(r); ++i) std::print(" ");

        for (int q = q_min; q <= q_max; ++q) {
            auto opt = world.grid.get(lwe::hex::Coord(q, r));
            if (!opt) { std::print("  "); continue; }

            const auto& hex = opt->get();
            const auto& w = engine.get(lwe::hex::Coord(q, r));

            /* Ocean */
            if (hex.biome == Biome::Ocean) {
                if (w.is_stormy) {
                    std::print("\033[37;44m*\033[0m ");
                } else {
                    std::print("\033[34m.\033[0m ");
                }
                continue;
            }

            /* Settlement */
            if (hex.settlement_id.has_value()) {
                if (w.is_stormy) {
                    std::print("\033[33;41m@\033[0m ");
                } else {
                    std::print("\033[33m@\033[0m ");
                }
                continue;
            }

            /* Weather overlay on terrain */
            if (w.is_stormy) {
                std::print("\033[97;41m*\033[0m ");
            } else if (w.temperature < -10) {
                std::print("\033[97;46m#\033[0m "); /* bright white on cyan = blizzard */
            } else if (w.temperature < 0) {
                std::print("\033[96m#\033[0m "); /* cyan = freezing */
            } else if (w.temperature > 35) {
                std::print("\033[31m!\033[0m "); /* red = extreme heat */
            } else if (w.temperature > 28) {
                std::print("\033[38;5;208m+\033[0m "); /* orange = hot */
            } else if (w.precipitation > 0.6) {
                std::print("\033[34m~\033[0m "); /* blue = heavy rain */
            } else if (w.precipitation > 0.4) {
                std::print("\033[36m:\033[0m "); /* light cyan = light rain */
            } else {
                /* Normal biome display */
                switch (hex.biome) {
                    case Biome::Desert:       std::print("\033[38;5;208md\033[0m "); break;
                    case Biome::Grassland:    std::print("\033[38;2;0;128;0m,\033[0m "); break;
                    case Biome::Forest:       std::print("\033[32mF\033[0m "); break;
                    case Biome::Jungle:       std::print("\033[38;5;22mJ\033[0m "); break;
                    case Biome::Swamp:        std::print("\033[38;5;22mw\033[0m "); break;
                    case Biome::Hills:        std::print("\033[32mh\033[0m "); break;
                    case Biome::Mountain:     std::print("\033[90mM\033[0m "); break;
                    case Biome::Peak:         std::print("\033[37m^\033[0m "); break;
                    case Biome::AlpineForest: std::print("\033[38;2;34;139;34mA\033[0m "); break;
                    case Biome::CloudForest:  std::print("\033[38;2;34;139;34mC\033[0m "); break;
                    default:                  std::print("\033[32m.\033[0m "); break;
                }
            }
        }
        std::print("\n");
    }

    /* Stats bar */
    double min_temp = 100, max_temp = -100, avg_temp = 0;
    int storm_count = 0, total_land = 0;
    double avg_precip = 0, avg_wind = 0;

    world.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (hex.biome == Biome::Ocean) return;
        total_land++;
        const auto& w = engine.get(c);
        min_temp = std::min(min_temp, w.temperature);
        max_temp = std::max(max_temp, w.temperature);
        avg_temp += w.temperature;
        avg_precip += w.precipitation;
        avg_wind += w.wind_speed;
        if (w.is_stormy) storm_count++;
    });

    if (total_land > 0) {
        avg_temp /= total_land;
        avg_precip /= total_land;
        avg_wind /= total_land;
    }

    std::print("\n Temp: {:.1f}°C / {:.1f}°C / {:.1f}°C /   |  Rain: {:.2f}  |  Wind: {:.2f}  |  Storms:  {}/{}\n",
               min_temp, avg_temp, max_temp, avg_precip, avg_wind, storm_count, total_land);
}

int main(int argc, char* argv[]) {
    /* Parse optional args */
    int radius = 20;
    int tick_ms = 1000;
    uint64_t seed = static_cast<uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--radius" && i + 1 < argc) radius = std::stoi(argv[++i]);
        else if (arg == "--speed" && i + 1 < argc) tick_ms = std::stoi(argv[++i]);
        else if (arg == "--seed" && i + 1 < argc) seed = std::stoull(argv[++i]);
    }

    /* Generate world */
    std::print("Generating world (radius={}, seed={})...\n", radius, seed);
    WorldConfig cfg;
    cfg.seed = seed;
    cfg.radius = radius;
    auto world = WorldGenerator(cfg).generate();
    std::print("World generated. Starting simulation...\n");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    /* Setup */
    WeatherEngine weather(world, seed + 1000);
    WorldClock clock;

    /* Handle Ctrl+C */
    std::signal(SIGINT, signal_handler);

    /* Main loop */
    while (running) {
        auto tick = clock.tick();
        weather.update(tick);
        render(world, weather, tick);
        std::this_thread::sleep_for(std::chrono::milliseconds(tick_ms));
    }

    std::print("\n\nSimulation stopped at Year {}, Day {}.\n", clock.year(), clock.day());
    return 0;
}
