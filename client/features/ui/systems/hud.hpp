#pragma once

#include <SDL3/SDL.h>

#include <string>
#include <vector>

#include "client/features/render/systems/paint.hpp"
#include "client/features/render/systems/text.hpp"
#include "sim/features/items/systems/inventory.hpp"
#include "sim/features/items/types/food.struct.hpp"
#include "client/design/types/color.struct.hpp"
#include "client/features/ui/types/popup.struct.hpp"

namespace client {

/**
 * The interface: the belt, what you are carrying, your health, and the one
 * line of prompt that tells you what the key under your finger would do.
 *
 * Not a comic panel, so it is not inked like one: flat plates and plain text,
 * kept out of the middle of the screen.
 */
class Hud {
public:
	void say(const std::string& text, double x, double y, Color color);

	void update(double dt);

	/** The popups, in world coordinates, drawn with the world. */
	void drawPopups(SDL_Renderer* renderer, double cameraX, double cameraY, double scale, int width,
					int height) const;

	/**
	 * `uiScale` is how dense the display is: the interface is laid out in
	 * points and drawn in pixels, so on a retina screen it doubles rather than
	 * coming out half the size it was meant to be.
	 */
	void draw(Paint& paint, const sim::Inventory& inventory, int health, int width, int height,
			  const char* prompt, float uiScale) const;

	/**
	 * What is loaded and what is left, the reload's progress, and how far a
	 * bow is drawn. Rounds below zero means nothing is being aimed.
	 */
	void setAmmo(int carried, int loaded, double reloading, double bowDraw);

	/**
	 * Which gun is loaded and with how many, whether it is in your hand or
	 * not.
	 *
	 * Only one gun holds rounds at a time, because loading a second one is
	 * what empties the first. A rifle left on the belt with half a magazine in
	 * it says so on its own slot, so you can see what you are switching to
	 * before you switch to it.
	 */
	void setLoadedGun(sim::ItemId gun, int rounds) {
		loadedGun_ = gun;
		loadedRounds_ = rounds;
	}

	/** Food, water, warmth, what you have taken in, and what is being applied. */
	void setVitals(double calories, double hydration, double temperature, double radiation,
				   double bleeding, double applying);

	/**
	 * Health still to come back, which reads beside the bar like a wound does.
	 *
	 * The same shape as the bleed, in green rather than red: a number that is
	 * coming to you rather than going from you. Both at once is a real state
	 * to be in, so both are drawn.
	 */
	void setMending(double health) { mending_ = health; }

	/** The clock, for anything that beats or blinks. */
	void setClock(double seconds) { clock_ = seconds; }

	/** The lettering, which the interface needs as much as the world does. */
	void useText(Text* text) { lettering_ = text; }

private:
	std::vector<Popup> popups_;
	/** The one gun that holds rounds, and how many, wherever it is sitting. */
	sim::ItemId loadedGun_ = sim::ItemId::None;
	int loadedRounds_ = 0;
	int carried_ = -1;
	int loaded_ = 0;
	double reloading_ = 0;
	double bowDraw_ = 0;
	double calories_ = 100;
	double hydration_ = 100;
	double temperature_ = 20;
	double radiation_ = 0;
	/** Seconds of bleeding left, which at one a second is also the health it
	 * will cost: see PlayerVitals::kBleedDamage. */
	double bleeding_ = 0;
	/** Health still owed by a syringe, at one a second. */
	double mending_ = 0;
	double applying_ = 0;
	double clock_ = 0;
	Text* lettering_ = nullptr;
};

}  // namespace client
