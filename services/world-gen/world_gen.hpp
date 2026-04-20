#pragma once

#include "common/world_data.hpp"
#include "dice/dice.hpp"

#include <cstdint>

struct WorldConfig {
    uint64_t seed = 42;
    int radius = 40;                   /* 40 = ~4921 hexes */
    double sea_level = 0.35;           /* elevation threshold (default 0.35) */
    double mountain_level = 0.8;       /* elecation threashold (default 0.80) */
    double moisture_frequency = 0.03;  /* noise scale (default 0.03) */
    double elevation_frequency = 0.02; /* noise scale (default 0.02) */
    int num_kingdoms = 4;              /* target (default 4) */
    double settlement_density = 0.5;   /* 0.0-1.0 (default 0.5) */
    double dungeon_density = 0.05;     /* dungeons per 100 hexes (default 0.05) */
};

class WorldGenerator {
public:
    WorldGenerator(const WorldConfig cfg);
    WorldData generate();

private:
    void generate_elevation();   /* stage 1  */
    void generate_moisture();    /* stage 2  */
    void assign_biomes();        /* stage 3  */
    void generate_rivers();      /* stage 4  */
    void place_settlements();    /* stage 5  */
    void build_roads();          /* stage 6  */
    void create_trade_routes();  /* stage 7  */
    void place_pois();           /* stage 8  */
    void assign_kingdoms();      /* stage 9  */
    void assign_climate_zones(); /* stage 10 */

    WorldData world_;
    WorldConfig cfg_;
    RNG rng_;
};
