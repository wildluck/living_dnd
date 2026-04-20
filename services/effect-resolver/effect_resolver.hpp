#pragma once

#include "common/world_data.hpp"
#include "weather_engine.hpp"
#include "world_clock.hpp"


#include <vector>

enum class ModType {
    SkillCheck, /* Modifier to skill checks (perception, survival, etc.) */
    AttackRoll, /* Modifier to attack rolls (melle, ranged) */
    MovementSpeed, /* Movement speed modifier (halved = -50) */
    Damage, /* Damage per round (positive = taking damage) */
    SavingThrow, /* DC for saving throw */
    Visibility, /* 0=clear, 1=lightly obscured, 2=heavily obscured */
    DifficultTerrain, /* 1=difficult terrain active */
};

struct MechanicMod {
    ModType type;
    int modifier; /* -2, +5, DC 10, etc. */
    std::string detail; /* "Ranged attacks", "CON save vs exhaustion", etc/ */
};

struct HexEffect {
    std::string name; /* "Blizzard", "Scorching Heat" */
    std::string description; /* DM-facing flovar text */
    std::vector<MechanicMod> mods;
};

class EffectResolver {
public:
    EffectResolver(const WorldData& world);

    /* Resolve all active effects for a hex given its weather and current tick */
    [[nodiscard]] std::vector<HexEffect> resolve(
        const lwe::hex::Coord& coord,
        const HexWeather& weather,
        const Tick& tick) const;

private:
    const WorldData& world_;

    /* Individual effect check */
    void check_temperature(std::vector<HexEffect>& effects, const HexWeather& weather) const;
    void check_precipitation(std::vector<HexEffect>& effects, const HexWeather& weather) const;
    void check_wind(std::vector<HexEffect>& effects, const HexWeather& weather) const;
    void check_storm(std::vector<HexEffect>& effects, const HexWeather& weather) const;
    void check_terrain(std::vector<HexEffect>& effects, const HexData& hex) const;
    void check_river(std::vector<HexEffect>& effects, const HexData& hex, const HexWeather& weather) const;
    void check_altitude(std::vector<HexEffect>& effects, const HexData& hex) const;
    void check_season(std::vector<HexEffect>& effects, const HexData& hex, const Tick& tick) const;
};
