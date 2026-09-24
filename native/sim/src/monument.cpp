#include "sim/monument.hpp"

namespace sim {

namespace {

// Carried over from the TypeScript game, loot tables and all.
constexpr MonumentDef kDefs[kMonumentKindCount] = {
    // Deliberately unguarded and un-irradiated: somewhere a fresh spawn can
    // loot on day one without a gun or a hazmat suit.
    {MonumentKind::Cabins, "Abandoned Cabins", 160, 0, 0, false, 3,
     {{ItemId::Wood, 40, 120},
      {ItemId::Cloth, 10, 30},
      {ItemId::Stone, 20, 60},
      {ItemId::Bandage, 1, 2},
      {ItemId::LowGrade, 5, 15}},
     5, 8, false, true, false},
    {MonumentKind::Lighthouse, "Lighthouse", 210, 0, 0, false, 3,
     {{ItemId::Scrap, 10, 25},
      {ItemId::Cloth, 10, 30},
      {ItemId::LowGrade, 10, 30},
      {ItemId::Metal, 20, 60},
      {ItemId::Bandage, 1, 3}},
     5, 6, true, false, false},
    {MonumentKind::Airfield, "Airfield", 340, 4, 4, false, 6,
     {{ItemId::Scrap, 25, 60},
      {ItemId::Metal, 60, 160},
      {ItemId::Sulfur, 20, 60},
      {ItemId::PistolAmmo, 8, 24},
      {ItemId::ShotgunShell, 4, 12},
      {ItemId::Medkit, 1, 2},
      {ItemId::Gunpowder, 10, 40}},
     7, 1, false, false, true},
    // Now and then an AK, as in Rust's elite crates.
    {MonumentKind::PowerPlant, "Power Plant", 380, 9, 7, false, 8,
     {{ItemId::Scrap, 50, 120},
      {ItemId::Metal, 120, 300},
      {ItemId::Sulfur, 60, 140},
      {ItemId::RifleAmmo, 10, 30},
      {ItemId::Hazmat, 0, 1},
      {ItemId::Ak47, 0, 1},
      {ItemId::Gunpowder, 30, 90}},
     7, 1, false, false, true},
    {MonumentKind::Military, "Military Base", 400, 6, 9, true, 9,
     {{ItemId::Scrap, 40, 100},
      {ItemId::Metal, 100, 260},
      {ItemId::RifleAmmo, 10, 30},
      {ItemId::PistolAmmo, 10, 30},
      {ItemId::ShotgunShell, 6, 18},
      {ItemId::Medkit, 1, 3},
      {ItemId::Hazmat, 0, 1},
      {ItemId::Ak47, 0, 1}},
     8, 1, false, false, true},
    {MonumentKind::Town, "Ashvale", 360, 0, 3, false, 7,
     {{ItemId::Scrap, 20, 50},
      {ItemId::Cloth, 20, 50},
      {ItemId::LowGrade, 15, 40},
      {ItemId::Metal, 40, 120},
      {ItemId::Bandage, 1, 4},
      {ItemId::Medkit, 0, 2},
      {ItemId::PistolAmmo, 6, 18}},
     7, 1, false, false, false},
};

}  // namespace

const MonumentDef& monumentDef(MonumentKind kind) { return kDefs[static_cast<int>(kind)]; }

}  // namespace sim
