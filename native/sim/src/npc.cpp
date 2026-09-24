#include "sim/npc.hpp"

namespace sim {

namespace {

// Carried over from the TypeScript game, where these were tuned by playing.
constexpr NpcGun kNoGun{0, 0, 0, 0, 0};

constexpr NpcDef kDefs[kNpcKindCount] = {
    {NpcKind::Boar, "Boar", 90, 120, 14, 12, 30, 1.2, false, false, kNoGun,
     {{ItemId::MeatRaw, 3, 5},
      {ItemId::Leather, 8, 14},
      {ItemId::Bone, 4, 8},
      {ItemId::AnimalFat, 4, 9}},
     4},
    {NpcKind::Wolf, "Wolf", 80, 155, 13, 22, 28, 0.9, true, false, kNoGun,
     {{ItemId::MeatRaw, 2, 4},
      {ItemId::Leather, 6, 12},
      {ItemId::Bone, 3, 6},
      {ItemId::AnimalFat, 2, 5}},
     4},
    {NpcKind::Bear, "Bear", 340, 92, 22, 42, 40, 1.3, true, false, kNoGun,
     {{ItemId::MeatRaw, 8, 14},
      {ItemId::Leather, 20, 34},
      {ItemId::Bone, 10, 18},
      {ItemId::AnimalFat, 14, 28}},
     4},
    // The monuments' own: sealed in a suit, and armed.
    {NpcKind::Scientist, "Scientist", 130, 118, 12, 14, 30, 1.0, true, true,
     {19, 470, 1.15, 900, 0.07},
     {{ItemId::Scrap, 8, 20},
      {ItemId::PistolAmmo, 4, 12},
      {ItemId::Metal, 10, 30},
      {ItemId::Medkit, 0, 1}},
     4},
    // Better trained than the scientists, in the field green of an army.
    {NpcKind::Soldier, "Soldier", 170, 126, 12, 18, 30, 0.9, true, true,
     {24, 520, 1.0, 950, 0.06},
     {{ItemId::Scrap, 12, 28},
      {ItemId::RifleAmmo, 6, 16},
      {ItemId::Metal, 20, 50},
      {ItemId::Medkit, 0, 2},
      {ItemId::Ak47, 0, 1}},
     5},
};

}  // namespace

const NpcDef& npcDef(NpcKind kind) { return kDefs[static_cast<int>(kind)]; }

}  // namespace sim
