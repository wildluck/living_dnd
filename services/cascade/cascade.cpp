#include "cascade.hpp"

#include <vector>

CascadeEngine::CascadeEngine(WorldData& world, const WeatherEngine& weather, uint64_t seed)
    : world_(world), weather_(weather), rng_(seed) {}

void CascadeEngine::emit(const Tick& tick, const std::string& msg) {
    events_.push_back({ tick.day, tick.year, msg });
}

/* ---------------- Event processing ---------------- */
void CascadeEngine::process_events(const std::vector<WorldEvent>& events, const Tick& tick) {
    for (const auto& event : events) {
        if (event.message.find("raided") != std::string::npos ||
            event.message.find("raid") != std::string::npos) {
            on_raid(event, tick);
        }
        if (event.message.find("starving") != std::string::npos) {
            on_starvation(event, tick);
        }
        if (event.message.find("WAR") != std::string::npos) {
            on_war(event, tick);
        }
        if (event.message.find("grown into") != std::string::npos) {
            on_promotion(event, tick);
        }
        if (event.message.find("storm damage") != std::string::npos) {
            on_storm_damage(event, tick);
        }
    }
}

void CascadeEngine::on_raid(const WorldEvent& event, const Tick& tick) {
    /* Extract settlement name — message format: "X was raided by..." */
    auto pos = event.message.find(" was raided");
    if (pos == std::string::npos) {
        /* Try "repelled a raid" format */
        pos = event.message.find("'s garrison repelled");
        if (pos == std::string::npos) return;
    }
    std::string name = event.message.substr(0, pos);
    int sid = find_settlement_by_name(name);
    if (sid < 0) return;

    const auto& settlement = world_.settlements[sid];

    /* Fear spreads from raided settlement */
    effects_.push_back({
        settlement.coord,
        10,                /* radius */
        30,                /* lasts 30 ticks */
        "fear",
        0.8,               /* high intensity */
        sid
    });

    emit(tick, std::format("Fear spreads from {} after the raid", settlement.name));

    /* Refugees flee to nearest safe settlement */
    int nearest_id = -1;
    int nearest_dist = std::numeric_limits<int>::max();
    for (const auto& other : world_.settlements) {
        if (other.id == sid) continue;
        int d = settlement.coord.distance_to(other.coord);
        if (d < nearest_dist) {
            nearest_dist = d;
            nearest_id = other.id;
        }
    }

    if (nearest_id >= 0) {
        int refugees = rng_.rand_int(5, settlement.population / 20 + 1);
        world_.settlements[sid].population = std::max(10,
            world_.settlements[sid].population - refugees);
        world_.settlements[nearest_id].population += refugees;

        emit(tick, std::format("{} refugees flee from {} to {}",
            refugees, settlement.name, world_.settlements[nearest_id].name));

        /* Refugee arrival creates minor prosperity at destination */
        effects_.push_back({
            world_.settlements[nearest_id].coord,
            5,
            15,
            "refugees",
            0.3,
            nearest_id
        });
    }
}

void CascadeEngine::on_starvation(const WorldEvent& event, const Tick& tick) {
    /* "X is starving! Y people lost" */
    auto pos = event.message.find(" is starving");
    if (pos == std::string::npos) return;
    std::string name = event.message.substr(0, pos);
    int sid = find_settlement_by_name(name);
    if (sid < 0) return;

    const auto& settlement = world_.settlements[sid];

    /* Famine spreads — nearby settlements hoard food */
    effects_.push_back({
        settlement.coord,
        8,
        45,                /* famine lasts longer */
        "famine",
        0.6,
        sid
    });

    /* Population migration — people leave starving settlement */
    int nearest_id = -1;
    int nearest_dist = std::numeric_limits<int>::max();
    for (const auto& other : world_.settlements) {
        if (other.id == sid) continue;
        if (!other.supply.contains(Resource::Food)) continue;
        if (other.supply.at(Resource::Food) <= 0) continue;
        int d = settlement.coord.distance_to(other.coord);
        if (d < nearest_dist) {
            nearest_dist = d;
            nearest_id = other.id;
        }
    }

    if (nearest_id >= 0 && nearest_dist <= 15) {
        int migrants = rng_.rand_int(3, settlement.population / 50 + 1);
        world_.settlements[sid].population = std::max(10,
            world_.settlements[sid].population - migrants);
        world_.settlements[nearest_id].population += migrants;

        emit(tick, std::format("{} people migrate from starving {} to {}",
            migrants, settlement.name, world_.settlements[nearest_id].name));
    }
}

void CascadeEngine::on_war(const WorldEvent& event, const Tick& tick) {
    /* "WAR! Kingdom_X declares war on Kingdom_Y!" */
    /* Disrupts trade across both kingdoms */
    for (const auto& kingdom : world_.kingdoms) {
        if (event.message.find(kingdom.name) != std::string::npos) {
            effects_.push_back({
                kingdom.capital,
                20,
                90,            /* war effects last a full season */
                "trade_disruption",
                1.0,
                -1
            });

            /* Border settlements suffer */
            for (const auto& coord : kingdom.territory) {
                if (!world_.grid.at(coord).is_border) continue;
                if (!world_.grid.at(coord).settlement_id.has_value()) continue;

                int sid = world_.grid.at(coord).settlement_id.value();
                effects_.push_back({
                    coord,
                    3,
                    60,
                    "war_front",
                    0.9,
                    sid
                });

                emit(tick, std::format("{} is on the war front!",
                    world_.settlements[sid].name));
            }
        }
    }
}

void CascadeEngine::on_promotion(const WorldEvent& event, const Tick& tick) {
    /* "X has grown into a City!" */
    auto pos = event.message.find(" has grown");
    if (pos == std::string::npos) return;
    std::string name = event.message.substr(0, pos);
    int sid = find_settlement_by_name(name);
    if (sid < 0) return;

    const auto& settlement = world_.settlements[sid];

    /* Prosperity radiates outward — nearby settlements benefit */
    effects_.push_back({
        settlement.coord,
        12,
        60,
        "prosperity",
        0.7,
        sid
    });

    emit(tick, std::format("Prosperity radiates from the growing {}", settlement.name));
}

void CascadeEngine::on_storm_damage(const WorldEvent& event, const Tick& tick) {
    auto pos = event.message.find(" suffered storm damage");
    if (pos == std::string::npos) return;
    std::string name = event.message.substr(0, pos);
    int sid = find_settlement_by_name(name);
    if (sid < 0) return;

    const auto& settlement = world_.settlements[sid];

    /* Storm damage accelerates road decay nearby */
    effects_.push_back({
        settlement.coord,
        6,
        10,
        "storm_aftermath",
        0.5,
        sid
    });
}

/* ---------------- Update active cascades ---------------- */
void CascadeEngine::update(const Tick& tick) {
    /* Decay existing effects */
    for (auto& effect : effects_) {
        effect.remaining_ticks--;
        /* Intensity decays over time */
        double time_ratio = static_cast<double>(effect.remaining_ticks) /
            static_cast<double>(effect.remaining_ticks + 1);
        effect.intensity *= (0.98 + 0.02 * time_ratio);
    }

    /* Remove expired effects */
    effects_.erase(
        std::remove_if(effects_.begin(), effects_.end(),
            [](const CascadeEffect& e) { return e.remaining_ticks <= 0 || e.intensity < 0.01; }),
        effects_.end());
}

/* ---------------- Apply cascade effects to world ---------------- */
void CascadeEngine::apply(const Tick& tick) {
    for (auto& settlement : world_.settlements) {
        double total_fear = fear_at(settlement.coord);
        double total_prosperity = prosperity_at(settlement.coord);
        double total_disruption = disruption_at(settlement.coord);

        /* Fear reduces population growth */
        if (total_fear > 0.3) {
            int fearful = static_cast<int>(settlement.population * total_fear * 0.001);
            settlement.population = std::max(10, settlement.population - fearful);
        }

        /* Prosperity boosts population */
        if (total_prosperity > 0.2) {
            int boost = static_cast<int>(settlement.population * total_prosperity * 0.001);
            settlement.population += boost;
        }

        /* Trade disruption reduces food */
        if (total_disruption > 0.3 && settlement.supply.contains(Resource::Food)) {
            int loss = static_cast<int>(total_disruption * 5);
            settlement.supply[Resource::Food] = std::max(0,
                settlement.supply[Resource::Food] - loss);
        }

        /* War front — garrison takes losses */
        for (const auto& effect : effects_) {
            if (effect.type != "war_front") continue;
            if (effect.source_settlement != settlement.id) continue;
            if (rng_.rand_bool(0.05)) {
                int losses = rng_.rand_int(1, 5);
                settlement.garrison = std::max(0, settlement.garrison - losses);
                settlement.population = std::max(10, settlement.population - losses);
                emit(tick, std::format("{} loses {} soldiers on the war front",
                    settlement.name, losses));
            }
        }
    }
}

/* ---------------- Query helpers ---------------- */
double CascadeEngine::intensity_at(const lwe::hex::Coord& origin,
                                    const lwe::hex::Coord& target,
                                    int radius, double base_intensity) const {
    int dist = origin.distance_to(target);
    if (dist > radius) return 0.0;
    /* Linear falloff with distance */
    double distance_factor = 1.0 - static_cast<double>(dist) / static_cast<double>(radius + 1);
    return base_intensity * distance_factor;
}

double CascadeEngine::fear_at(const lwe::hex::Coord& c) const {
    double total = 0.0;
    for (const auto& effect : effects_) {
        if (effect.type == "fear" || effect.type == "war_front") {
            total += intensity_at(effect.origin, c, effect.radius, effect.intensity);
        }
    }
    return std::min(total, 1.0);
}

double CascadeEngine::prosperity_at(const lwe::hex::Coord& c) const {
    double total = 0.0;
    for (const auto& effect : effects_) {
        if (effect.type == "prosperity") {
            total += intensity_at(effect.origin, c, effect.radius, effect.intensity);
        }
    }
    return std::min(total, 1.0);
}

double CascadeEngine::disruption_at(const lwe::hex::Coord& c) const {
    double total = 0.0;
    for (const auto& effect : effects_) {
        if (effect.type == "trade_disruption" || effect.type == "famine") {
            total += intensity_at(effect.origin, c, effect.radius, effect.intensity);
        }
    }
    return std::min(total, 1.0);
}

/* ---------------- Settlement lookup ---------------- */
int CascadeEngine::find_settlement_by_name(const std::string& name_fragment) const {
    for (const auto& s : world_.settlements) {
        if (s.name == name_fragment) return s.id;
    }
    // Partial match
    for (const auto& s : world_.settlements) {
        if (name_fragment.find(s.name) != std::string::npos ||
            s.name.find(name_fragment) != std::string::npos) {
            return s.id;
        }
    }
    return -1;
}
