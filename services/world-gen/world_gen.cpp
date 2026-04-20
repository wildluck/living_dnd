#include "world_gen.hpp"
#include "noise/fbm.hpp"
#include "hex/algorithms.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

WorldGenerator::WorldGenerator(const WorldConfig cfg) : cfg_(cfg), rng_(cfg.seed) {}

WorldData WorldGenerator::generate() {
    world_ = WorldData{};
    world_.seed = cfg_.seed;
    world_.radius = cfg_.radius;
    world_.grid = lwe::hex::HexGrid<HexData>::generate(cfg_.radius, [](const lwe::hex::Coord& c) {
        return HexData{.coord = c};
    });

    generate_elevation();
    generate_moisture();
    assign_biomes();
    generate_rivers();
    place_settlements();
    build_roads();
    create_trade_routes();
    place_pois();
    assign_kingdoms();
    assign_climate_zones();

    return std::move(world_);
}

void WorldGenerator::generate_elevation() {
    lwe::noise::SimplexNoise noise(cfg_.seed);
    lwe::noise::FBM fbm(noise, 6, 2.0, 0.5);

    double max_dist = static_cast<double>(cfg_.radius);
    world_.grid.for_each([&](const lwe::hex::Coord& c, HexData& hex) {
        auto [px, py] = c.to_pixel(1.0);

        double sx = px * cfg_.elevation_frequency;
        double sy = py * cfg_.elevation_frequency;

        double e = fbm.sample(sx, sy);
        e = (e + 1.0) / 2.0;

        double dist = static_cast<double>(c.length()) / max_dist;
        double falloff = 1.0;
        if (dist > 0.6) {
            double t = (dist - 0.6) / 0.4;
            falloff = 1.0 - t * t;
        }
        if (dist > 1.0) falloff = 0.0;

        hex.elevation = static_cast<float>(e * falloff);
    });
}

void WorldGenerator::generate_moisture() {
    lwe::noise::SimplexNoise noise(cfg_.seed + 1);
    lwe::noise::FBM fbm(noise, 6, 2.0, 0.5);

    world_.grid.for_each([&](const lwe::hex::Coord& c, HexData& hex) {
        auto [px, py] = c.to_pixel(1.0);
        double m = fbm.sample(px * cfg_.moisture_frequency, py * cfg_.moisture_frequency);
        hex.moisture = static_cast<float>((m + 1.0) / 2.0); /* remap to [0, 1] */
    });
}

/**
 * [--------]   Dry (0.0-0.3)   Moderate (0.3-0.6)   Wet (0.6-1.0)
 * Below sea    Ocean           Ocean                Ocean
 * Lowland      Desert          Grassland            Swamp
 * Midland      Hills           Forest               Jungle
 * Highland     Mountain        AlpineForest         CloudForest
 * Peak         Peak            Peak                 Peak
 */
void WorldGenerator::assign_biomes() {
    const double sea = cfg_.sea_level;
    const double peak = cfg_.mountain_level;
    const double band = (peak - sea) / 3.0;

    auto elevation_band = [&](float e) -> int {
        if (e < sea)         return 0; /* Ocean */
        if (e < sea + band)  return 1; /* Lowland */
        if (e < peak - band) return 2; /* Midland */
        if (e < peak)        return 3; /* Highland */
        return 4;                      /* Peak */
    };

    auto moisture_band = [](float m) -> int {
        if (m < 0.3) return 0; /* Dry */
        if (m < 0.6) return 1; /* Moderate */
        return 2;              /* Wet */
    };

    static constexpr Biome table[5][3] = {
        { Biome::Ocean,    Biome::Ocean,        Biome::Ocean       },
        { Biome::Desert,   Biome::Grassland,    Biome::Swamp       },
        { Biome::Hills,    Biome::Forest,       Biome::Jungle      },
        { Biome::Mountain, Biome::AlpineForest, Biome::CloudForest },
        { Biome::Peak,     Biome::Peak,         Biome::Peak        },
    };

    world_.grid.for_each([&](const lwe::hex::Coord& c, HexData& hex) {
        hex.biome = table[elevation_band(hex.elevation)][moisture_band(hex.moisture)];
        hex.terrain = (hex.biome == Biome::Ocean) ? Terrain::Water : Terrain::Land;
    });
}

void WorldGenerator::generate_rivers() {
    std::unordered_map<lwe::hex::Coord, int> flow;
    std::unordered_map<lwe::hex::Coord, lwe::hex::Direction> flow_dirs;

    world_.grid.for_each([&](const lwe::hex::Coord&c, const HexData& hex) {
        if (hex.biome != Biome::Peak) return;

        lwe::hex::Coord current = c;
        std::unordered_set<lwe::hex::Coord> visited;

        for (int step = 0; step < 200; ++step) {
            if (visited.contains(current)) break;
            visited.insert(current);

            const auto& cur_hex = world_.grid.at(current);
            if (cur_hex.biome == Biome::Ocean) break;

            lwe::hex::Coord lowest = current;
            float lowest_elev = cur_hex.elevation;
            for (const auto& n : current.neighbors()) {
                auto opt = world_.grid.get(n);
                if (!opt) continue;
                if (opt->get().elevation < lowest_elev) {
                    lowest = n;
                    lowest_elev = opt->get().elevation;
                }
            }

            if (lowest == current) break;
            flow[lowest]++;
            auto dir = current.direction_between(lowest);
            flow_dirs[lowest] = current.direction_between(lowest);
            current = lowest;
        }
    });

    const int river_threshold = 3;
    for (const auto& [coord, count] : flow) {
        if (count <= river_threshold) {
            auto opt = world_.grid.get(coord);
            if (opt && opt->get().biome != Biome::Ocean) {
                opt->get().has_river = true;
                opt->get().terrain = Terrain::River;
                opt->get().river_flor_dir = flow_dirs[coord];
                world_.rivers.push_back({coord, coord, static_cast<float>(count)});
            }
        }
    }
}

void WorldGenerator::place_settlements() {
    struct Candidate {
        lwe::hex::Coord coord;
        double score;
    };

    std::vector<Candidate> candidates;
    world_.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        double score = 0.0;

        switch (hex.biome) {
            case Biome::Grassland: score = 10; break;
            case Biome::Forest:    score =  7; break;
            case Biome::Hills:     score =  6; break;
            case Biome::Jungle:    score =  3; break;
            case Biome::Desert:    score =  2; break;
            case Biome::Swamp:     score =  2; break;
            default: return; /* ocean, mountain, peak */
        }

        /* River bonus */
        for (const auto& n : c.neighbors()) {
            auto opt = world_.grid.get(n);
            if (opt && opt->get().has_river) { score += 5; break; }
        }
        if (hex.has_river) score += 5;

        /* Coast bonus */
        for (const auto& n : c.neighbors()) {
            auto opt = world_.grid.get(n);
            if (opt && opt->get().biome == Biome::Ocean) { score += 3; break; }
        }

        /* Connectivity bonus */
        int land_neighbors = 0;
        for (const auto& n : c.neighbors()) {
            auto opt = world_.grid.get(n);
            if (opt && opt->get().terrain != Terrain::Water) land_neighbors++;
        }
        if (land_neighbors >= 4) score += 2;

        candidates.push_back({c, score});
    });

    /* Sort descending by score */
    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate& a, const Candidate& b) { return a.score  > b.score; });

    /* Count land hexes for target */
    int land_count = 0;
    world_.grid.for_each([&](const lwe::hex::Coord&, const HexData& hex) {
        if (hex.terrain != Terrain::Water) land_count++;
    });

    int target = std::max(4, static_cast<int>(land_count * cfg_.settlement_density * 0.01));
    int min_spacing = std::max(3, cfg_.radius / 4);

    /* Place settlements with spacing constraint */
    for (const auto& [coord, score] : candidates) {
        if (static_cast<int>(world_.settlements.size()) >= target) break;

        /* Check distance to existing settlements */
        bool too_close = false;
        for (const auto& s : world_.settlements) {
            if (coord.distance_to(s.coord) < min_spacing) {
                too_close = true;
                break;
            }
        }
        if (too_close) continue;

        /* Place it */
        int id = static_cast<int>(world_.settlements.size());
        Settlement s;
        s.id = id;
        s.name = "Settlement_" + std::to_string(id);
        s.coord = coord;
        s.population = 0;
        s.garrison = 0;

        world_.settlements.push_back(s);
        world_.grid.at(coord).settlement_id = id;
    }

    /* Assign types based on rank */
    int n = static_cast<int>(world_.settlements.size());
    for (int i = 0; i < n; ++i) {
        double rank = static_cast<double>(i) / n;
        if (rank < 0.2) {
            world_.settlements[i].type = SettlementType::City;
            world_.settlements[i].population = 5000 + rng_.rand_int(0, 5000);
        } else if (rank < 0.5) {
            world_.settlements[i].type = SettlementType::Town;
            world_.settlements[i].population = 1000 + rng_.rand_int(0, 2000);
        } else if (rank < 0.8) {
            world_.settlements[i].type = SettlementType::Village;
            world_.settlements[i].population = 200 + rng_.rand_int(0, 500);
        } else {
            world_.settlements[i].type = SettlementType::Hamlet;
            world_.settlements[i].population = 50 + rng_.rand_int(0, 150);
        }
        world_.settlements[i].garrison = world_.settlements[i].population / 10;
    }
}

void WorldGenerator::build_roads() {
    if (world_.settlements.size() < 2) return;

    /* Collect settlement coords */
    std::vector<lwe::hex::Coord> nodes;
    for (const auto& s : world_.settlements) {
        nodes.push_back(s.coord);
    }

    /* MST gives us which settlements to connect */
    auto edges = lwe::hex::minimum_spanning_tree(nodes,
        [](const lwe::hex::Coord& a, const lwe::hex::Coord& b) {
        return static_cast<double>(a.distance_to(b));
    });

    /* Terrain cost function for pathfinding */
    auto terrain_cost = [this](const lwe::hex::Coord& from, const lwe::hex::Coord& to) -> double {
        auto opt = world_.grid.get(to);
        if (!opt) return -1.0;
        const auto& hex = opt->get();

        /* Prefer existing roads */
        if (hex.terrain == Terrain::Road) return 0.5;

        if (hex.terrain == Terrain::River) {
            auto from_opt = world_.grid.get(from);
            if (from_opt && from_opt->get().terrain == Terrain::River) {
                auto flow = from_opt->get().river_flor_dir;
                if (flow.has_value()) {
                    auto flow_target = from.neighbor(*flow);
                    if (flow_target == to) return 0.5; /* Downstream */
                    return 3.0; /* Upstream */
                }
            }
            return 2.0; /* Entering river from land */
        }

        switch (hex.biome) {
        case Biome::Ocean:        return -1.0;
        case Biome::Peak:         return -1.0;
        case Biome::Mountain:     return  8.0;
        case Biome::Swamp:        return  5.0;
        case Biome::Jungle:       return  4.0;
        case Biome::Hills:        return  4.0;
        case Biome::Forest:       return  3.0;
        case Biome::Desert:       return  3.0;
        case Biome::AlpineForest: return  5.0;
        case Biome::CloudForest:  return  5.0;
        case Biome::Grassland:    return  1.0;
        default:                  return  2.0;
        }
    };

    for (const auto& [from, to] : edges) {
        auto result = lwe::hex::a_star(from, to, terrain_cost, cfg_.radius * 2);
        if (!result || !result->found()) continue;

        for (const auto& c : result->path) {
            auto& hex = world_.grid.at(c);
            if (hex.terrain == Terrain::River) continue;
            if (hex.settlement_id.has_value()) continue;
            hex.terrain = Terrain::Road;
        }
    }
}

void WorldGenerator::create_trade_routes() {
    if (world_.settlements.size() < 2) return;
    /* TODO: assign resources to settlements based on nearby biomes */
    for (auto& settlement : world_.settlements) {
        std::unordered_set<Resource> produces;
        auto& hex = world_.grid.at(settlement.coord);

        auto check_biome = [&](const HexData& h) {
            switch (h.biome) {
            case Biome::Forest:
            case Biome::Jungle:    produces.insert(Resource::Wood); break;
            case Biome::Hills:     produces.insert(Resource::Stone); break;
            case Biome::Mountain:  produces.insert(Resource::Iron);
                                   produces.insert(Resource::Stone); break;
            case Biome::Grassland: produces.insert(Resource::Food); break;
            case Biome::Desert:    produces.insert(Resource::Gold); break;
            default: break;
            }
        };

        check_biome(hex);
        for (const auto& n : settlement.coord.neighbors()) {
            auto opt = world_.grid.get(n);
            if (opt) check_biome(opt->get());
        }

        /* Coast: Fish */
        for (const auto& n : settlement.coord.neighbors()) {
            auto opt = world_.grid.get(n);
            if (opt && opt->get().biome == Biome::Ocean) {
                produces.insert(Resource::Fish);
                break;
            }
        }

        for (const auto& r : produces) {
            int amount = 0;
            switch (settlement.type) {
            case SettlementType::City:    amount = 100; break;
            case SettlementType::Town:    amount =  50; break;
            case SettlementType::Village: amount =  20; break;
            case SettlementType::Hamlet:  amount =  10; break;
            default:                      amount =  10; break;
            }
            settlement.supply[r] = amount;
        }
    }

    std::vector<Resource> all_resources = {
        Resource::Food, Resource::Wood,  Resource::Iron,
        Resource::Gold, Resource::Stone, Resource::Fish,
    };

    auto route_cost = [this](const lwe::hex::Coord&, const lwe::hex::Coord& to) -> double {
        auto opt = world_.grid.get(to);
        if (!opt) return -1.0;
        const auto& hex = opt->get();
        if (hex.terrain == Terrain::Road) return 0.5;
        if (hex.biome == Biome::Ocean || hex.biome == Biome::Peak) return -1.0;
        return 3.0;
    };

    int max_routes_per_settlement = 3;
    std::unordered_map<int, int> route_count; /* settlement_id -> num routes */

    for (auto& consumer : world_.settlements) {
        if (route_count[consumer.id] >= max_routes_per_settlement) continue;

        for (const auto& resource : all_resources) {
            if (consumer.supply.contains(resource)) continue;
            if (route_count[consumer.id] >= max_routes_per_settlement) break;

            int best_id = -1;
            int best_dist = std::numeric_limits<int>::max();
            for (const auto& producer : world_.settlements) {
                if (producer.id == consumer.id) continue;
                if (!producer.supply.contains(resource)) continue;
                if (route_count[producer.id] >= max_routes_per_settlement) continue;

                int dist = consumer.coord.distance_to(producer.coord);
                if (dist < best_dist) {
                    best_dist = dist;
                    best_id = producer.id;
                }
            }

            if (best_id < 0) continue;

            auto result = lwe::hex::a_star(
                consumer.coord, world_.settlements[best_id].coord,
                route_cost, cfg_.radius * 2);

            if (!result || !result->found()) continue;

            TradeRoute route;
            route.id = static_cast<int>(world_.trade_routes.size());
            route.from_settlement = best_id;
            route.to_settlement = consumer.id;
            route.resource = resource;
            route.path = result->path;

            world_.trade_routes.push_back(route);
            route_count[consumer.id]++;
            route_count[best_id]++;
        }
    }
}

void WorldGenerator::place_pois() {
    struct Candidate {
        lwe::hex::Coord coord;
        double score;
    };

    std::vector<Candidate> candidates;

    world_.grid.for_each([&](const lwe::hex::Coord& c, const HexData& hex) {
        if (hex.terrain == Terrain::Water) return;
        if (hex.settlement_id.has_value()) return;
        if (hex.biome == Biome::Peak) return;

        double score = 0.0;

        int min_settlement_dist = std::numeric_limits<int>::max();
        for (const auto& s : world_.settlements) {
            int d = c.distance_to(s.coord);
            min_settlement_dist = std::min(min_settlement_dist, d);
        }
        score += min_settlement_dist * 2.0;

        int min_road_dist = std::numeric_limits<int>::max();
        world_.grid.for_each([&](const lwe::hex::Coord& rc, const HexData& rh) {
            if (rh.terrain == Terrain::Road) {
                min_road_dist = std::min(min_road_dist, c.distance_to(rc));
            }
        });
        score += min_road_dist;

        switch (hex.biome) {
        case Biome::Mountain:  score += 3; break;
        case Biome::Hills:     score += 2; break;
        case Biome::Swamp:     score += 2; break;
        case Biome::Forest:    score += 1; break;
        case Biome::Jungle:    score += 1; break;
        case Biome::Grassland: score -= 2; break;
        default: break;
        }

        candidates.push_back({c, score});
    });

    std::sort(candidates.begin(), candidates.end(),
        [](const Candidate& a, const Candidate& b) { return a.score > b.score; });

    int land_count = 0;
    world_.grid.for_each([&](const lwe::hex::Coord&, const HexData& hex) {
        if (hex.terrain != Terrain::Water) land_count++;
    });

    int target = std::max(3, static_cast<int>(land_count * cfg_.dungeon_density));
    int min_spacing = 5;

    for (const auto& [coord, score] : candidates) {
        if (static_cast<int>(world_.pois.size()) >= target) break;

        bool too_close = false;
        for (const auto& poi : world_.pois) {
            if (coord.distance_to(poi.coord) < min_spacing) {
                too_close = true;
                break;
            }
        }
        if (too_close) continue;

        const auto& hex = world_.grid.at(coord);

        POIType type;
        switch (hex.biome) {
        case Biome::Mountain:
            type = rng_.rand_bool(0.3) ? POIType::DragonLair : POIType::Cave;
            break;
        case Biome::Hills:
            type = rng_.rand_bool(0.5) ? POIType::Cave : POIType::GoblinWarren;
            break;
        case Biome::Swamp:
            type = rng_.rand_bool(0.5) ? POIType::Crypt : POIType::Shrine;
            break;
        case Biome::Forest:
        case Biome::Jungle:
            type = rng_.rand_bool(0.5) ? POIType::Ruins : POIType::GoblinWarren;
            break;
        case Biome::Desert:
            type = rng_.rand_bool(0.5) ? POIType::Ruins : POIType::Shrine;
            break;
        default:
            type = POIType::WizardTower;
            break;
        }

        int min_dist = std::numeric_limits<int>::max();
        for (const auto& s : world_.settlements) {
            min_dist = std::min(min_dist, coord.distance_to(s.coord));
        }
        int difficulty;
        if (min_dist > 15) difficulty = rng_.rand_int(10, 20);
        else if (min_dist > 8) difficulty = rng_.rand_int(5, 10);
        else difficulty = rng_.rand_int(1, 5);

        int id = static_cast<int>(world_.pois.size());

        const char* type_prefix = "";
        switch (type) {
        case POIType::Cave:         type_prefix = "Cave";         break;
        case POIType::Ruins:        type_prefix = "Ruins";        break;
        case POIType::DragonLair:   type_prefix = "DragonLair";   break;
        case POIType::BanditCamp:   type_prefix = "BanditCamp";   break;
        case POIType::Crypt:        type_prefix = "Crypt";        break;
        case POIType::WizardTower:  type_prefix = "WizardTower";  break;
        case POIType::GoblinWarren: type_prefix = "GoblinWarren"; break;
        case POIType::Shrine:       type_prefix = "Shrine";       break;
        }

        PointOfInterest poi;
        poi.id = id;
        poi.name = std::string(type_prefix) + "_" + std::to_string(id);
        poi.type = type;
        poi.coord = coord;
        poi.difficulty = difficulty;

        world_.pois.push_back(poi);
        world_.grid.at(coord).poi_id = id;
    }
}

void WorldGenerator::assign_kingdoms() {
    if (world_.settlements.empty()) return;

    std::vector<int> sorted_ids;
    for (const auto& s : world_.settlements) sorted_ids.push_back(s.id);
    std::sort(sorted_ids.begin(), sorted_ids.end(), [this](int a, int b) {
        return world_.settlements[a].population > world_.settlements[b].population;
    });

    int num_kingdoms = std::min(cfg_.num_kingdoms, static_cast<int>(sorted_ids.size()));
    std::vector<lwe::hex::Coord> capitals;
    for (int i = 0; i < num_kingdoms; ++i) {
        capitals.push_back(world_.settlements[sorted_ids[i]].coord);
    }

    // for (int i = 0; i < num_kingdoms; ++i) {
    //     auto& hex = world_.grid.at(capitals[i]);
    //     std::print("Kingdom {} capital at ({},{}) biome={} terrain={}\n",
    //         i, capitals[i].q, capitals[i].r,
    //         static_cast<int>(hex.biome), static_cast<int>(hex.terrain));
    // }

    auto partition = lwe::hex::voronoi_partition(capitals,
        [this](const lwe::hex::Coord& c) {
            auto opt = world_.grid.get(c);
            if (!opt) return false;
            return opt->get().biome != Biome::Ocean;
        });

    for (int i = 0; i < num_kingdoms; ++i) {
        Kingdom k;
        k.id = i;
        k.name = "Kingdom_" + std::to_string(i);
        k.capital = capitals[i];
        world_.kingdoms.push_back(k);
    }

    for (const auto& [coord, owner] : partition) {
        world_.grid.at(coord).kingdom_id = owner;
        world_.kingdoms[owner].territory.insert(coord);
    }

    for (const auto& [coord, owner] : partition) {
        for (const auto& n : coord.neighbors()) {
            auto opt = world_.grid.get(n);
            if (!opt) continue;
            const auto& nhex = opt->get();

            if (!nhex.kingdom_id.has_value() || nhex.kingdom_id.value() != owner) {
                world_.grid.at(coord).is_border = true;
                break;
            }
        }
    }

    for (int i = 0; i < num_kingdoms; ++i) {
        for (int j = i + 1; j < num_kingdoms; ++j) {
            int score = rng_.rand_int(-100, 100);
            world_.kingdoms[i].relationships[j] = score;
            world_.kingdoms[j].relationships[i] = score;
        }
    }
}

void WorldGenerator::assign_climate_zones() {
    world_.grid.for_each([&](const lwe::hex::Coord& c, HexData& hex) {
        if (hex.biome == Biome::Ocean) return;

        double latitude = static_cast<double>(c.r) / static_cast<double>(cfg_.radius);
        double abs_lat = std::abs(latitude);

        bool near_coast = false;
        for (const auto& n : c.neighbors()) {
            auto opt = world_.grid.get(n);
            if (opt && opt->get().biome == Biome::Ocean) {
                near_coast = true;
                break;
            }
        }

        if (hex.biome == Biome::Peak || hex.biome == Biome::Mountain) {
            hex.climate = ClimateZone::Alpine;
        } else if (hex.biome == Biome::AlpineForest || hex.biome == Biome::CloudForest) {
            hex.climate = ClimateZone::Alpine;
        } else if (near_coast) {
            hex.climate = ClimateZone::Maritime;
        } else if (abs_lat > 0.8) {
            hex.climate = ClimateZone::Arctic;
        } else if (abs_lat > 0.6) {
            hex.climate = ClimateZone::Subarctic;
        } else if (abs_lat > 0.4) {
            hex.climate = ClimateZone::Temperate;
        } else if (abs_lat > 0.2) {
            hex.climate = hex.biome == Biome::Desert ? ClimateZone::Arid : ClimateZone::Subtropical;
        } else {
            hex.climate = hex.biome == Biome::Desert ? ClimateZone::Arid : ClimateZone::Tropical;
        }
    });
}
