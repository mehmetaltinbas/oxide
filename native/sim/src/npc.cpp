#include "sim/npc.hpp"

namespace sim {

namespace {

// Carried over from the TypeScript game, where these were tuned by playing.
constexpr NpcDef kDefs[kNpcKindCount] = {
    {NpcKind::Boar, "Boar", 90, 120, 14, 12, 30, 1.2, false,
     {{ItemId::MeatRaw, 3, 5}, {ItemId::Leather, 8, 14}, {ItemId::Bone, 4, 8},
      {ItemId::AnimalFat, 4, 9}}},
    {NpcKind::Wolf, "Wolf", 80, 155, 13, 22, 28, 0.9, true,
     {{ItemId::MeatRaw, 2, 4}, {ItemId::Leather, 6, 12}, {ItemId::Bone, 3, 6},
      {ItemId::AnimalFat, 2, 5}}},
    {NpcKind::Bear, "Bear", 340, 92, 22, 42, 40, 1.3, true,
     {{ItemId::MeatRaw, 8, 14}, {ItemId::Leather, 20, 34}, {ItemId::Bone, 10, 18},
      {ItemId::AnimalFat, 14, 28}}},
};

}  // namespace

const NpcDef& npcDef(NpcKind kind) { return kDefs[static_cast<int>(kind)]; }

}  // namespace sim
