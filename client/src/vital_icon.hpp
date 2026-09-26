#pragma once

#include "paint.hpp"

namespace client {

/** The three gauges: what is left of you, and what you have put in. */
enum class Vital { Health, Food, Water };

/**
 * The picture at the head of a gauge: a heart, a drumstick, a drop.
 *
 * A word under each bar was one more thing to read in a corner you only glance
 * at. Drawn in a unit square scaled to `size`, and inked like the item icons,
 * so the gauges sit in the same printed style as everything else.
 */
void drawVitalIcon(Paint& paint, Vital vital, float cx, float cy, float size, Color color);

}  // namespace client
