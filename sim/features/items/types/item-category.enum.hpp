#pragma once

#include "sim/features/building/types/deployable.struct.hpp"

#include <cstdint>

namespace sim {

enum class ItemCategory : std::uint8_t {
	Resource,
	Tool,
	Ammo,
	Weapon,
	Consumable,
	Clothing,
	Deployable,
	Explosive,
};

}  // namespace sim
