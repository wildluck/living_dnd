#pragma once

#include "hex/coord.hpp"
#include "hex/grid.hpp"
#include "common/types.hpp"

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct HexData {
    lwe::hex::Coord coord;
    float elevation; /* [0, 1] */
    float moisture;  /* [0, 1] */
    Biome biome;
    Terrain terrain;
    ClimateZone climate;
    std::optional<int> settlement_id;
    std::optional<int> poi_id;
    std::optional<int> kingdom_id;
    bool is_border;
    bool has_river;
    std::optional<lwe::hex::Direction> river_flor_dir;
    std::vector<Resource> resources;
    /* More needed data to be added soon */
};

struct Settlement {
    int id;
    std::string name;
    SettlementType type;
    lwe::hex::Coord coord;
    int population;
    int garrison;
    std::unordered_map<Resource, int> supply;
    /* More needed data to be added soon */
};

struct TradeRoute {
    int id;
    int from_settlement;
    int to_settlement;
    Resource resource;
    std::vector<lwe::hex::Coord> path;
};

struct Kingdom {
    int id;
    std::string name;
    lwe::hex::Coord capital;
    std::unordered_set<lwe::hex::Coord> territory;
    std::unordered_map<int, int> relationships; /* kingdom_id -> score (-100 to 100) */
};

struct PointOfInterest {
    int id;
    std::string name;
    POIType type;
    lwe::hex::Coord coord;
    int difficulty; /* CR (Challenge Rating) 1-20 */
};

struct RiverSegment {
    lwe::hex::Coord from;
    lwe::hex::Coord to;
    float flow;
};

struct WorldData {
    uint64_t seed;
    int radius;
    lwe::hex::HexGrid<HexData> grid;
    std::vector<Settlement> settlements;
    std::vector<TradeRoute> trade_routes;
    std::vector<Kingdom> kingdoms;
    std::vector<PointOfInterest> pois;
    std::vector<RiverSegment> rivers;
};
