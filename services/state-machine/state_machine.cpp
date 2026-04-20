#include "state_machine.hpp"
#include "common/types.hpp"
#include "common/world_data.hpp"
#include "weather_engine.hpp"

#include <algorithm>
#include <format>

StateMachine::StateMachine(WorldData& world, const WeatherEngine& weather, uint64_t seed)
    : world_(world), weather_(weather), rng_(seed) {}

void StateMachine::emit(const Tick& tick, const std::string& msg) {
    events_.push_back({ tick.day, tick.year, msg });
}

void StateMachine::update(const Tick& tick) {
    update_settlements(tick);
    update_resources(tick);
    update_roads(tick);
    update_pois(tick);

    /* Kingdoms update once per season */
    if (tick.day_of_season == 1) {
        update_kingdoms(tick);
    }
}

/* ---------------- Settlements growth/decline ---------------- */
void StateMachine::update_settlements(const Tick& tick) {
    for (auto& settlement : world_.settlements) {
        const auto& w = weather_.get(settlement.coord);
        double growth_rate = 0.001; /* base 0.1% per day */

        /* Food surplus bonus */
        if (settlement.supply.contains(Resource::Food)) {
            growth_rate += 0.0005;
        }

        /* Trade route bonus */
        int route_count = 0;
        for (const auto& tr : world_.trade_routes) {
            if (tr.from_settlement == settlement.id || tr.to_settlement == settlement.id)
                route_count++;
        }
        growth_rate += route_count * 0.0002;

        /* River bonus */
        const auto& hex = world_.grid.at(settlement.coord);
        if (hex.has_river) growth_rate += 0.0001;

        /* Storm damage */
        if (w.is_stormy) {
            growth_rate -= 0.001;
            if (rng_.rand_bool(0.1)) {
                int casualties = rng_.rand_int(1, settlement.population / 100 + 1);
                settlement.population = std::max(10, settlement.population - casualties);
                emit(tick, std::format("{} suffered storm damage, {} casualties",
                    settlement.name, casualties));
            }
        }

        /* Extreme weather */
        if (w.temperature < -10) {
            growth_rate -= 0.001;
        }
        if (w.temperature > 35) {
            growth_rate -= 0.001;
        }

        /* Nearby dungeon thread */
        for (const auto& poi : world_.pois) {
            int dist = settlement.coord.distance_to(poi.coord);
            if (dist <= 8) growth_rate -= 0.0003;
        }

        /* War penalty - check if kingdom is at war */
        if (hex.kingdom_id.has_value()) {
            int kid = hex.kingdom_id.value();
            if (kid < static_cast<int>(world_.kingdoms.size())) {
                for (const auto& [other_id, score] : world_.kingdoms[kid].relationships) {
                    if (score < -80) {
                        growth_rate -= 0.01;
                        break;
                    }
                }
            }
        }

        /* Apply growth */
        int delta = static_cast<int>(settlement.population * growth_rate);
        settlement.population = std::max(10, settlement.population + delta);

        /* Settlement type promotion/demotion based on population */
        SettlementType old_type = settlement.type;
        if (settlement.population >= 5000) settlement.type = SettlementType::City;
        else if (settlement.population >= 1000) settlement.type = SettlementType::Town;
        else if (settlement.population >= 200) settlement.type = SettlementType::Village;
        else settlement.type = SettlementType::Hamlet;

        if (settlement.type != old_type) {
            emit(tick, std::format("{} has grown into a {}!",
                settlement.name,
                settlement.type == SettlementType::City ? "City" :
                settlement.type == SettlementType::Town ? "Town" :
                settlement.type == SettlementType::Village ? "Village" : "Hamlet"));
        }

        /* Garrison scale with population */
        settlement.garrison = settlement.population / 10;
    }
}

/* ---------------- Resource production and consumption ---------------- */
void StateMachine::update_resources(const Tick& tick) {
    for (auto& settlement : world_.settlements) {
        /* Production based on type */
        int production_mult = 1;
        switch (settlement.type) {
        case SettlementType::City: production_mult = 50; break;
        case SettlementType::Town: production_mult = 20; break;
        case SettlementType::Village: production_mult = 8; break;
        case SettlementType::Hamlet: production_mult = 3; break;
        default: break;
        }

        for (auto& [resource, amount] : settlement.supply) {
            /* Produce */
            amount += production_mult;

            /* Consume based on papulation */
            int consumption = settlement.population / 500;
            amount = std::max(0, amount - consumption);
        }

        /* Food shortage check */
        if (settlement.supply.contains(Resource::Food)) {
            if (settlement.supply[Resource::Food] == 0) {
                int starved = settlement.population / 500;
                settlement.population = std::max(10, settlement.population - starved);
                if (starved > 0) {
                    emit(tick, std::format("{} is starving! {} people lost", settlement.name, starved));
                }
            }
        }

        /* Season affects food production */
        if (tick.season == Season::Winter) {
            if (settlement.supply.contains(Resource::Food)) {
                settlement.supply[Resource::Food] = std::max(0,
                    settlement.supply[Resource::Food] - production_mult);
            }
        } else if (tick.season == Season::Spring) {
            if (settlement.supply.contains(Resource::Food)) {
                settlement.supply[Resource::Food] += production_mult;
            }
        }
    }
}

/* ---------------- Kingdom relations ---------------- */
void StateMachine::update_kingdoms(const Tick& tick) {
    int n = static_cast<int>(world_.kingdoms.size());

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            int& score = world_.kingdoms[i].relationships[j];
            int& mirror = world_.kingdoms[j].relationships[i];

            /* Shared border friction */
            bool share_border = false;
            for (const auto& coord : world_.kingdoms[i].territory) {
                if (!world_.grid.at(coord).is_border) continue;
                for (const auto& nb : coord.neighbors()) {
                    auto opt = world_.grid.get(nb);
                    if (opt && opt->get().kingdom_id.has_value() &&
                        opt->get().kingdom_id.value() == j) {
                        share_border = true;
                        break;
                    }
                }
                if (share_border) break;
            }

            if (share_border) {
                score -= 1;
                mirror -= 1;
            }

            /* Trade bonus */
            bool has_trade = false;
            for (const auto& tr : world_.trade_routes) {
                auto from_kid = world_.grid.at(
                    world_.settlements[tr.from_settlement].coord).kingdom_id;
                auto to_kid = world_.grid.at(
                    world_.settlements[tr.to_settlement].coord).kingdom_id;
                if (from_kid && to_kid) {
                    if ((from_kid.value() == i && to_kid.value() == j) ||
                        (from_kid.value() == j && to_kid.value() == i)) {
                        has_trade = true;
                        break;
                    }
                }
            }

            if (has_trade) {
                score += 2;
                mirror += 2;
            }

            /* Natural drift toward 0 */
            if (score > 0) { score -= 1; mirror -= 1; }
            else if (score < 0) { score += 1; mirror += 1; }

            /* Random events */
            if (rng_.rand_bool(0.05)) {
                int event_val = rng_.rand_int(-10, 10);
                score += event_val;
                mirror += event_val;

                if (event_val > 5) {
                    emit(tick, std::format("{} and {} strengthen diplomatic ties",
                        world_.kingdoms[i].name, world_.kingdoms[j].name));
                } else if (event_val < -5) {
                    emit(tick, std::format("Tensions rise between {} and {}",
                        world_.kingdoms[i].name, world_.kingdoms[j].name));
                }
            }

            /* Clamp */
            score = std::clamp(score, -100, 100);
            mirror = std::clamp(mirror, -100, 100);

            /* War declaration */
            if (score < -80) {
                if (rng_.rand_bool(0.02)) {
                    emit(tick, std::format("WAR! {} declares war on {}!",
                        world_.kingdoms[i].name, world_.kingdoms[j].name));
                }
            }

            /* Alliance formation */
            if (score > 80) {
                if (rng_.rand_bool(0.02)) {
                    emit(tick, std::format("{} and {} form an alliance!",
                        world_.kingdoms[i].name, world_.kingdoms[j].name));
                }
            }
        }
    }
}

/* ---------------- Road maintenance and decay ---------------- */
void StateMachine::update_roads(const Tick& tick) {
    /* Collect road hexes on trade routes (these are maintained) */
    std::unordered_set<lwe::hex::Coord> trade_road_hexes;
    for (const auto& tr : world_.trade_routes) {
        for (const auto& c : tr.path) {
            trade_road_hexes.insert(c);
        }
    }

    /* Track which roads decay */
    std::vector<lwe::hex::Coord> decayed_roads;

    world_.grid.for_each([&](const lwe::hex::Coord& c, HexData& hex) {
        if (hex.terrain != Terrain::Road) return;
        if (hex.settlement_id.has_value()) return; /* settlement roads never decay */

        /* Trade routes maintain roads */
        if (trade_road_hexes.contains(c)) return;

        /* Base decay chance per tick */
        double decay_chance = 0.0005;

        /* Storms accelerate decay */
        const auto& w = weather_.get(c);
        if (w.is_stormy) decay_chance *= 3.0;

        /* Freezing accelerates decay */
        if (w.temperature < 0) decay_chance *= 1.5;

        if (rng_.rand_bool(decay_chance)) {
            decayed_roads.push_back(c);
        }
    });

    for (const auto& c : decayed_roads) {
        auto& hex = world_.grid.at(c);
        hex.terrain = Terrain::Land;
        emit(tick, std::format("Road at ({},{}) has fallen into disrepair",
            c.q, c.r));
    }
}

/* ---------------- POI threat escalation ---------------- */
void StateMachine::update_pois(const Tick& tick) {
    for (auto& poi : world_.pois) {
        /* Threat grows over time (difficulty as proxy for thread)
         * Higher CR dungeons are more active
         */
        double raid_chance = 0.0;

        if (poi.difficulty >= 15) {
            raid_chance = 0.01;
        } else if (poi.difficulty >= 10) {
            raid_chance = 0.005;
        } else if (poi.difficulty >= 5) {
            raid_chance = 0.002;
        } else {
            raid_chance = 0.001;
        }

        /* Season modifiers */
        if (tick.season == Season::Winter) {
            raid_chance *= 0.5; /* monsters hunker down */
        } else if (tick.season == Season::Summer) {
            raid_chance *= 1.5; /* monsters are active */
        }

        if (!rng_.rand_bool(raid_chance)) continue;

        /* Find nearest settlement */
        int nearest_dist = std::numeric_limits<int>::max();
        Settlement* nearest = nullptr;
        for (auto& s : world_.settlements) {
            int d = poi.coord.distance_to(s.coord);
            if (d < nearest_dist) {
                nearest_dist = d;
                nearest = &s;
            }
        }

        if (!nearest || nearest_dist > 12) continue; /* too far to raid */

        /* Raid! */
        int raid_strength = poi.difficulty * rng_.rand_int(1, 3);

        if (raid_strength > nearest->garrison) {
            /* Successful raid */
            int casualties = rng_.rand_int(1, raid_strength / 2 + 1);
            int stolen_food = rng_.rand_int(0, 20);
            nearest->population = std::max(10, nearest->population - casualties);
            nearest->garrison = std::max(0, nearest->garrison - casualties / 2);

            if (nearest->supply.contains(Resource::Food)) {
                nearest->supply[Resource::Food] = std::max(0,
                    nearest->supply[Resource::Food] - stolen_food);
            }

            const char* poi_type = "";
            switch (poi.type) {
                case POIType::Cave:         poi_type = "creatures from the caves"; break;
                case POIType::Ruins:        poi_type = "raiders from the ruins"; break;
                case POIType::DragonLair:   poi_type = "a dragon"; break;
                case POIType::BanditCamp:   poi_type = "bandits"; break;
                case POIType::Crypt:        poi_type = "undead from the crypt"; break;
                case POIType::WizardTower:  poi_type = "arcane constructs"; break;
                case POIType::GoblinWarren: poi_type = "goblins"; break;
                case POIType::Shrine:       poi_type = "cultists from the shrine"; break;
            }

            emit(tick, std::format("{} was raided by {}! {} casualties, {} food stolen",
                nearest->name, poi_type, casualties, stolen_food));
        } else {
            /* Failed raid — garrison held */
            int garrison_loss = rng_.rand_int(0, raid_strength / 3 + 1);
            nearest->garrison = std::max(0, nearest->garrison - garrison_loss);

            emit(tick, std::format("{}'s garrison repelled a raid from {} (lost {} guards)",
                nearest->name, poi.name, garrison_loss));
        }
    }
}
