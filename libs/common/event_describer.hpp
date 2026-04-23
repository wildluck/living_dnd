#pragma once

#include "common/event.hpp"
#include "common/types.hpp"
#include "common/world_data.hpp"

#include <format>
#include <string>
#include <variant>

inline std::string describe(const WorldEvent& event, const WorldData& world) {
    return std::visit(overloaded{
        [&](const RaidOccurred& r) {
            const char* poi_type = "";
            switch (r.type) {
                case POIType::Cave:         poi_type = "creatures from the caves"; break;
                case POIType::Ruins:        poi_type = "raiders from the ruins"; break;
                case POIType::DragonLair:   poi_type = "a dragon"; break;
                case POIType::BanditCamp:   poi_type = "bandits"; break;
                case POIType::Crypt:        poi_type = "undead from the crypt"; break;
                case POIType::WizardTower:  poi_type = "arcane constructs"; break;
                case POIType::GoblinWarren: poi_type = "goblins"; break;
                case POIType::Shrine:       poi_type = "cultists from the shrine"; break;
            }
            return std::format("[Y{} D{}] {} was raided by {} ({})! {} casualties, {} food stolen.",
                event.when.year, event.when.day,
                world.settlements[r.settlement_id].name, poi_type, world.pois[r.poi_id].name, r.casualties, r.food_stolen);
        },
        [&](const RaidRepelled& r) { return std::format("[Y{} D{}] {}'s garrison repelled a raid from {} (lost {} guards).",
            event.when.year, event.when.day,
            world.settlements[r.settlement_id].name, world.pois[r.poi_id].name, r.garrison_losses); },
        [&](const Starvation& s) { return std::format("[Y{} D{}] {} is starving! {} people lost",
            event.when.year, event.when.day, world.settlements[s.settlement_id].name, s.deaths); },
        [&](const SettlementPromoted& p) {
            return std::format("[Y{} D{}] {} has grown into a {}!",
                event.when.year, event.when.day,
                world.settlements[p.settlement_id].name,
                p.to == SettlementType::City ? "City" :
                p.to == SettlementType::Town ? "Town" :
                p.to == SettlementType::Village ? "Village" : "Hamlet");
        },
        [&](const StormDamage& d) { return std::format("[Y{} D{}] {} suffered storm damage, {} casualties",
            event.when.year, event.when.day, world.settlements[d.settlement_id].name, d.casualties); },
        [&](const WarDeclared& w) { return std::format("[Y{} D{}] WAR! {} declares war on {}!",
            event.when.year, event.when.day,
            world.kingdoms[w.attacker_kingdom].name, world.kingdoms[w.defender_kingdom].name); },
        [&](const AllianceFormed& a) { return std::format("[Y{} D{}] {} and {} form an alliance!",
            event.when.year, event.when.day, world.kingdoms[a.kingdom_a].name, world.kingdoms[a.kingdom_b].name); },
        [&](const DiplomaticShift& s) {
            if (s.delta > 0) return std::format("[Y{} D{}] {} and {} strengthen diplomatic ties",
                event.when.year, event.when.day, world.kingdoms[s.kingdom_a].name, world.kingdoms[s.kingdom_b].name);
            else return std::format("[Y{} D{}] Tensions rise between {} and {}",
                event.when.year, event.when.day, world.kingdoms[s.kingdom_a].name, world.kingdoms[s.kingdom_b].name);
        },
        [&](const RoadDecayed& r) { return std::format("[Y{} D{}] Road at ({},{}) has fallen into disrepair",
            event.when.year, event.when.day, r.coord.q, r.coord.r); },
        [&](const FearSpread& f) { return std::format("[Y{} D{} CASCADE] Fear spreads from {} after the raid",
            event.when.year, event.when.day, world.settlements[f.settlement_id].name); },
        [&](const RefugeeFlee& r) { return std::format("[Y{} D{} CASCADE] {} refugees flee from {} to {}",
            event.when.year, event.when.day, r.count,
            world.settlements[r.settlement_id_from].name, world.settlements[r.settlement_id_to].name); },
        [&](const MigratedFromStarving& m) { return std::format("[Y{} D{} CASCADE] {} people migrate from starving {} to {}",
            event.when.year, event.when.day, m.count,
            world.settlements[m.settlement_id_from].name, world.settlements[m.settlement_id_to].name); },
        [&](const WarFront& w) { return std::format("[Y{} D{} CASCADE] {} is on the war front!",
            event.when.year, event.when.day, world.settlements[w.settlement_id].name); },
        [&](const Prosperity& p) { return std::format("[Y{} D{} CASCADE] Prosperity radiates from the growing {}",
            event.when.year, event.when.day, world.settlements[p.settlement_id].name); },
        [&](const WarLoss& l) { return std::format("[Y{} D{} CASCADE] {} loses {} soldiers on the war front",
            event.when.year, event.when.day, world.settlements[l.settlement_id].name, l.count); },
    }, event.payload);
}
