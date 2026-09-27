#pragma once

#include "sim/features/items/types/item-id.enum.hpp"

namespace sim {

/** Someone standing on the island. */
struct Player {
	double x = 0;
	double y = 0;
	/** Where they are looking, in radians. */
	double aim = 0;
	/** How far through a stride they are, for the legs and the arms. */
	double walkPhase = 0;
	bool swimming = false;
	bool sprinting = false;

	bool alive = true;
	double health = 100;
	/** What is left of you: food, water, warmth and what you have taken in. */
	double calories = 75;
	double hydration = 75;
	double temperature = 20;
	double radiation = 0;
	/** Seconds of bleeding left, and health still to come from a syringe. */
	double bleeding = 0;
	double healOverTime = 0;
	/** A moment of grace after a hit, so one wolf is not eight hits a second. */
	double invuln = 0;
	double hurtFlash = 0;
	double respawnTimer = 0;
	/** Seconds until the next blow may be thrown. */
	double attackTimer = 0;
	/** Seconds left of the swing being drawn. */
	double swingAnim = 0;
	/** How long the whole swing was, so the drawing knows where it is in it. */
	double swingLength = 0;

	/** What is loaded, and how many rounds of it are in the gun. */
	ItemId loaded = ItemId::None;
	int rounds = 0;
	/** Seconds left of a reload, and how long the whole reload is. */
	double reloadLeft = 0;
	double reloadTotal = 0;
	/** How long a bow has been drawn back. */
	double bowDraw = 0;

	/** What is being applied, and how long is left of applying it. */
	ItemId applying = ItemId::None;
	double useLeft = 0;
	double useTotal = 0;
};

}  // namespace sim
