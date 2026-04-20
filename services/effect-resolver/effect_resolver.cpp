#include "effect_resolver.hpp"

#include <vector>

EffectResolver::EffectResolver(const WorldData& world) : world_(world) {}

std::vector<HexEffect> EffectResolver::resolve(
    const lwe::hex::Coord& coord,
    const HexWeather& weather,
    const Tick& tick) const
{
    std::vector<HexEffect> effects;

    auto opt = world_.grid.get(coord);
    if (!opt) return effects;
    const auto& hex = opt->get();

    /* Skipping ocean */
    if (hex.biome == Biome::Ocean) return effects;

    check_temperature(effects, weather);
    check_precipitation(effects, weather);
    check_wind(effects, weather);
    check_storm(effects, weather);
    check_terrain(effects, hex);
    check_river(effects, hex, weather);
    check_altitude(effects, hex);
    check_season(effects, hex, tick);

    return effects;
}

/* Temperature effects */
void EffectResolver::check_temperature(std::vector<HexEffect>& effects, const HexWeather& weather) const {
    if (weather.temperature < -10) {
        effects.push_back({
            "Extreme cold",
            "Bitter winds and freezing temperatures threaten all who venture out.",
            {
                { ModType::SavingThrow, 10, "CON save each hour or gain 1 exhaustion" },
                { ModType::Damage, 4, "1d4 cold damage per hour without shelter" },
                { ModType::DifficultTerrain, 1, "ice and snow" },
                { ModType::SkillCheck, -3, "Survival checks" },
            }
        });
    } else if (weather.temperature < 0) {
        effects.push_back({
            "Freezing",
            "Frost coats every surface. Breath mists in the air.",
            {
                { ModType::DifficultTerrain, 1, "frozen ground and ice patched" },
                { ModType::SkillCheck, -2, "survival checks" },
                { ModType::MovementSpeed, -10, "slippery footing" },
            }
        });
    } else if (weather.temperature > 35) {
        effects.push_back({
            "Extreme Heat",
            "The sun beats down mercilessly, Metal armor becomes unbearable.",
            {
                { ModType::SavingThrow, 10, "CON save each hour or gain 1 exhaustion" },
                { ModType::Damage, 4, "1d4 fire damage per hour in heavy armor" },
                { ModType::SkillCheck, -2, "CON-based checks" },
                { ModType::MovementSpeed, -10, "heat exhaustion" },
            }
        });
    } else if (weather.temperature > 28) {
        effects.push_back({
            "Oppressive Heat",
            "The air is thich and humid. Heavy armor weighs twice as much.",
            {
                { ModType::SkillCheck, -1, "CON-based checks in heavy armor" },
                { ModType::MovementSpeed, -5, "sluggish in the heat" },
            }
        });
    }
}

/* Precipitation effects */
void EffectResolver::check_precipitation(std::vector<HexEffect>& effects, const HexWeather& weather) const {
    if (weather.is_stormy) return;  /* Storm check handles heavy precipitation */

    if (weather.precipitation > 0.8) {
        effects.push_back({
            "Downpour",
            "Torrential rain reduces visibility to a few feet. Rivers swell dangerously.",
            {
                { ModType::Visibility, 2, "heavily obscured" },
                { ModType::SkillCheck, -5, "Perception checks (sight)" },
                { ModType::AttackRoll, -2, "ranged attacks beyond 30ft" },
                { ModType::SkillCheck, -2, "Survival checks for tracking" },
            }
        });
    } else if (weather.precipitation > 0.6) {
        effects.push_back({
            "Heavy Rain",
            "Steady rain drums against everything. Puddles form on every path.",
            {
                { ModType::Visibility, 1, "lightly obscured" },
                { ModType::SkillCheck, -2, "Perception checks (sight)" },
                { ModType::AttackRoll, -1, "ranged attacks beyond 60ft" },
            }
        });
    } else if (weather.precipitation > 0.4) {
        effects.push_back({
            "Light Rain",
            "A gentle rain falls, dampening clothes and spirits.",
            {
                { ModType::SkillCheck, -1, "Perception checks (sight)" },
            }
        });
    }
}

/* Wind effects */
void EffectResolver::check_wind(std::vector<HexEffect>& effects, const HexWeather& weather) const {
    if (weather.is_stormy) return;  /* Storm check handles high winds */

    if (weather.wind_speed > 0.9) {
        effects.push_back({
            "Gale Force Wind",
            "Howling winds tear at cloaks and threaten to knock the unwary off their feet.",
            {
                { ModType::AttackRoll, -5, "ranged weapon attacks" },
                { ModType::SavingThrow, 12, "STR save or be pushed 10ft (Small creatures)" },
                { ModType::SavingThrow, 8, "STR save or be pushed 5ft (Medium creatures)" },
                { ModType::SkillCheck, -3, "Perception checks (hearing)" },
                { ModType::MovementSpeed, -15, "fighting the wind" },
            }
        });
    } else if (weather.wind_speed > 0.7) {
        effects.push_back({
            "Strong Wind",
            "The wind gusts fiercely, scattering loose objects and carrying voices away.",
            {
                { ModType::AttackRoll, -2, "ranged weapon attacks" },
                { ModType::SkillCheck, -2, "Perception checks (hearing)" },
                { ModType::MovementSpeed, -5, "headwind resistance" },
            }
        });
    }
}

/* Storm effects */
void EffectResolver::check_storm(std::vector<HexEffect>& effects, const HexWeather& weather) const {
    if (!weather.is_stormy) return;

    if (weather.temperature < 0) {
        /* Blizzard */
        effects.push_back({
            "Blizzard",
            "A wall of white. Snow and ice lash exposed skin. Navigation is nearly impossible.",
            {
                { ModType::Visibility, 2, "heavily obscured" },
                { ModType::SkillCheck, -5, "Perception checks" },
                { ModType::AttackRoll, -5, "ranged weapon attacks" },
                { ModType::DifficultTerrain, 1, "deep snow drifts" },
                { ModType::SavingThrow, 13, "CON save each hour or gain 1 exhaustion" },
                { ModType::Damage, 6, "1d6 cold damage per hour without shelter" },
                { ModType::SkillCheck, -5, "Survival checks for navigation" },
                { ModType::MovementSpeed, -20, "whiteout conditions" },
            }
        });
    } else if (weather.temperature > 30) {
        /* Sandstorm / heat storm */
        effects.push_back({
            "Sandstorm",
            "Searing wind carries stinging sand that scours exposed flesh.",
            {
                { ModType::Visibility, 2, "heavily obscured" },
                { ModType::SkillCheck, -5, "Perception checks" },
                { ModType::AttackRoll, -3, "ranged weapon attacks" },
                { ModType::Damage, 4, "1d4 slashing damage per round without cover" },
                { ModType::SavingThrow, 12, "CON save or blinded for 1 round" },
                { ModType::MovementSpeed, -15, "sand impedes movement" },
            }
        });
    } else {
        /* Thunderstorm */
        effects.push_back({
            "Thunderstorm",
            "Lightning splits the sky. Thunder shakes the ground. Rain falls in sheets.",
            {
                { ModType::Visibility, 2, "heavily obscured" },
                { ModType::SkillCheck, -5, "Perception checks" },
                { ModType::AttackRoll, -3, "ranged weapon attacks" },
                { ModType::SkillCheck, -3, "Perception checks (hearing, thunder)" },
                { ModType::Damage, 10, "3d10 lightning damage (5% chance per creature per hour in open)" },
                { ModType::DifficultTerrain, 1, "mud and flooding" },
                { ModType::MovementSpeed, -10, "mud and wind" },
            }
        });
    }
}

/* Terrain effects */
void EffectResolver::check_terrain(std::vector<HexEffect>& effects, const HexData& hex) const {
    switch (hex.biome) {
        case Biome::Swamp:
            effects.push_back({
                "Swampland",
                "Sucking mud and murky water make every step a struggle. The air buzzes with insects.",
                {
                    { ModType::DifficultTerrain, 1, "boggy ground" },
                    { ModType::MovementSpeed, -10, "wading through muck" },
                    { ModType::SkillCheck, -2, "Stealth checks (splashing)" },
                    { ModType::SavingThrow, 10, "CON save or contract disease (long rest in swamp)" },
                }
            });
            break;
        case Biome::Jungle:
            effects.push_back({
                "Dense Jungle",
                "Thick canopy blocks the sky. Vines and roots tangle every path.",
                {
                    { ModType::DifficultTerrain, 1, "undergrowth and roots" },
                    { ModType::Visibility, 1, "lightly obscured by canopy" },
                    { ModType::SkillCheck, -2, "Navigation (Survival) checks" },
                    { ModType::SkillCheck, +2, "Stealth checks (abundant cover)" },
                }
            });
            break;
        case Biome::Mountain:
            effects.push_back({
                "Mountain Pass",
                "Rocky terrain with steep inclines and narrow ledges.",
                {
                    { ModType::DifficultTerrain, 1, "rocky slopes" },
                    { ModType::MovementSpeed, -10, "climbing and scrambling" },
                    { ModType::SavingThrow, 10, "DEX save to avoid rockslide (when applicable)" },
                }
            });
            break;
        case Biome::Desert:
            effects.push_back({
                "Desert Sands",
                "Endless dunes shift beneath your feet. Water is scarce.",
                {
                    { ModType::DifficultTerrain, 1, "loose sand" },
                    { ModType::SkillCheck, -2, "Survival checks for finding water" },
                }
            });
            break;
        case Biome::Forest:
            effects.push_back({
                "Forest",
                "Tall trees provide shade and cover. The undergrowth rustles with life.",
                {
                    { ModType::SkillCheck, +1, "Stealth checks (cover)" },
                    { ModType::Visibility, 1, "lightly obscured in dense areas" },
                }
            });
            break;
        default:
            break;
    }
}

/* River crossing effects */
void EffectResolver::check_river(std::vector<HexEffect>& effects, const HexData& hex, const HexWeather& weather) const {
    if (!hex.has_river) return;

    if (weather.is_stormy || weather.precipitation > 0.7) {
        effects.push_back({
            "Raging River",
            "The river churns with muddy floodwaters. Crossing is treacherous.",
            {
                { ModType::SavingThrow, 15, "STR (Athletics) to swim across" },
                { ModType::Damage, 6, "1d6 bludgeoning on failed crossing" },
                { ModType::DifficultTerrain, 1, "flooded banks" },
            }
        });
    } else {
        effects.push_back({
            "River Crossing",
            "A steady river bars the path. Fording requires care.",
            {
                { ModType::SavingThrow, 12, "STR (Athletics) to swim or wade across" },
                { ModType::MovementSpeed, -10, "crossing the river" },
            }
        });
    }
}

/* Altitude effects */
void EffectResolver::check_altitude(std::vector<HexEffect>& effects, const HexData& hex) const {
    if (hex.biome != Biome::Peak) return;

    effects.push_back({
        "Extreme Altitude",
        "The air is thin and bitter cold. Each breath is a struggle.",
        {
            { ModType::SavingThrow, 13, "CON save each hour or gain 1 exhaustion (altitude sickness)" },
            { ModType::MovementSpeed, -15, "thin air" },
            { ModType::SkillCheck, -2, "all ability checks (oxygen deprivation)" },
            { ModType::DifficultTerrain, 1, "loose scree and ice" },
        }
    });
}

/* Seasonal effects */
void EffectResolver::check_season(std::vector<HexEffect>& effects, const HexData& hex, const Tick& tick) const {
    switch (tick.season) {
        case Season::Spring:
            if (hex.biome == Biome::Grassland || hex.biome == Biome::Forest) {
                effects.push_back({
                    "Spring Growth",
                    "New life blooms everywhere. Foraging is plentiful.",
                    {
                        { ModType::SkillCheck, +2, "Survival checks for foraging" },
                    }
                });
            }
            break;

        case Season::Summer:
            /* Long days */
            effects.push_back({
                "Long Days",
                "The sun lingers long, granting extra hours of daylight for travel.",
                {
                    { ModType::MovementSpeed, 5, "extended daylight for travel" },
                }
            });
            break;

        case Season::Autumn:
            if (hex.biome == Biome::Forest) {
                effects.push_back({
                    "Falling Leaves",
                    "A carpet of dry leaves crunches underfoot, betraying movement.",
                    {
                        { ModType::SkillCheck, -2, "Stealth checks in forests" },
                        { ModType::SkillCheck, +1, "Survival checks for tracking" },
                    }
                });
            }
            break;

        case Season::Winter:
            if (hex.climate != ClimateZone::Tropical && hex.climate != ClimateZone::Arid) {
                effects.push_back({
                    "Short Days",
                    "Darkness falls early. The nights are long and cold.",
                    {
                        { ModType::MovementSpeed, -5, "reduced daylight for travel" },
                    }
                });
            }
            break;
    }
}
