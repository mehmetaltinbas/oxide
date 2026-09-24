#pragma once

#include <SDL3/SDL.h>

#include <string>
#include <vector>

#include "paint.hpp"
#include "sim/inventory.hpp"

namespace client {

/** A number that floats up off something and fades: what you just gained. */
struct Popup {
    std::string text;
    double x;
    double y;
    double life;
    Color color;
};

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

    /** Food, water, warmth, what you have taken in, and what is being applied. */
    void setVitals(double calories, double hydration, double temperature, double radiation,
                   bool bleeding, double applying);

private:
    std::vector<Popup> popups_;
    int carried_ = -1;
    int loaded_ = 0;
    double reloading_ = 0;
    double bowDraw_ = 0;
    double calories_ = 100;
    double hydration_ = 100;
    double temperature_ = 20;
    double radiation_ = 0;
    bool bleeding_ = false;
    double applying_ = 0;
};

}  // namespace client
