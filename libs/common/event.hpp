#pragma once

#include "types.hpp"
#include "coord.hpp"
#include "world_clock.hpp"

#include <variant>

/* State-machine events */
struct RaidOccurred { int settlement_id; int poi_id; POIType type; int casualties; int food_stolen; };
struct RaidRepelled { int settlement_id; int poi_id; int garrison_losses; };
struct Starvation { int settlement_id; int deaths; };
struct SettlementPromoted { int settlement_id; SettlementType from, to; };
struct StormDamage { int settlement_id; int casualties; };
struct WarDeclared { int attacker_kingdom; int defender_kingdom; };
struct AllianceFormed { int kingdom_a; int kingdom_b; };
struct DiplomaticShift { int kingdom_a; int kingdom_b; int delta; };
struct RoadDecayed { lwe::hex::Coord coord; };

/* Cascade events */
struct FearSpread { int settlement_id; };
struct RefugeeFlee { int settlement_id_from; int settlement_id_to; int count; };
struct MigratedFromStarving { int settlement_id_from; int settlement_id_to; int count; };
struct WarFront { int settlement_id; };
struct Prosperity { int settlement_id; };
struct WarLoss { int settlement_id; int count; };

using EventPayload = std::variant<
    RaidOccurred, RaidRepelled, Starvation, SettlementPromoted,
    StormDamage, WarDeclared, AllianceFormed, DiplomaticShift, RoadDecayed,
    FearSpread, RefugeeFlee, MigratedFromStarving, WarFront, Prosperity, WarLoss>;

struct WorldEvent {
    Tick when;
    EventPayload payload;
};

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;
