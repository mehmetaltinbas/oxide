#pragma once

#include "body_frame.hpp"
#include "melee.hpp"
#include "paint.hpp"
#include "sim/item.hpp"

namespace client {

/**
 * What is in someone's hand, drawn in their own frame.
 *
 * A tool is a handle and a head, and which way round it lies is the whole of
 * whether a swing reads: carried upright it is seen end-on, so the head sits
 * over the fist and the handle runs away below it, out of the picture, and
 * mid-swing it lies flat and hangs off the grip.
 */
void drawHeldItem(Paint& paint, const BodyFrame& body, sim::ItemId item, const MeleePose& pose);

/** The same item as a flat icon, for the belt and the pack. */
void drawItemIcon(Paint& paint, sim::ItemId item, float x, float y, float size);

/** How a given item is swung. */
MeleeStyle meleeStyleOf(sim::ItemId item);

}  // namespace client
