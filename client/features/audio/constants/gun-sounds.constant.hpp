#pragma once

#include "client/features/audio/types/gun-sound.struct.hpp"
#include "sim/features/items/types/item-id.enum.hpp"

namespace client {

/** What a gun sounds like, or nothing for the ones that make no report. */
const GunSound* gunSoundOf(sim::ItemId id);

}  // namespace client
