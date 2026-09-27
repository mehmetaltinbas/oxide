#pragma once

#include "sim/features/items/types/boom.struct.hpp"
#include "sim/features/items/types/food.struct.hpp"
#include "sim/features/items/types/gun.struct.hpp"
#include "sim/features/items/types/item-category.enum.hpp"
#include "sim/features/items/types/item-id.enum.hpp"
#include "sim/features/items/types/melee.struct.hpp"
#include "sim/features/items/types/wear.struct.hpp"

namespace sim {

struct ItemDef {
	ItemId id;
	const char* name;
	/** One line on what it is for, as the pack screen prints it. */
	const char* desc;
	ItemCategory category;
	int stack;
	/** Zero damage means it is not a thing you swing. */
	Melee melee;
	/** Zero damage means it is not a thing you fire. */
	Gun gun;
	/** All zero means it is not a thing you consume. */
	Food food;
	/** All zero means it is not a thing you wear. */
	Wear wear;
	/** All zero means it is not a thing that goes off. */
	Boom boom;
};

}  // namespace sim
