#pragma once

#include <functional>

enum class Biome {
    Ocean,
    Coast,
    Desert,
    Grassland,
    Forest,
    Jungle,
    Swamp,
    Hills,
    Mountain,
    Peak,
    AlpineForest,
    CloudForest,
    Beach,
    Cliff,
    /* More biomes to be added soon */
};

enum class Terrain {
    Water,
    Land,
    Road,
    Bridge,
    River,
    /* More terrains to be added soon */
};

enum class Resource {
    Food,
    Iron,
    Wood,
    Gold,
    Stone,
    Fish,
    /* More Resources to be added soon */
};

template<>
struct std::hash<Resource> {
    std::size_t operator()(Resource r) const noexcept {
        return std::hash<int>{}(static_cast<int>(r));
    }
};

enum class Season {
    Spring,
    Summer,
    Autumn,
    Winter
};

enum class ClimateZone {
    Arctic,
    Subarctic,
    Temperate,
    Subtropical,
    Tropical,
    Arid,
    Alpine,
    Maritime,
    /* More ClimateZones to be added soon */
};

enum class SettlementType {
    Hamlet,
    Village,
    Town,
    City,
    Fortress,
};

enum class POIType {
    Cave,
    Ruins,
    DragonLair,
    BanditCamp,
    Crypt,
    WizardTower,
    GoblinWarren,
    Shrine,
};
