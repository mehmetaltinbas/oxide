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
void drawHeldItem(Paint& paint, const BodyFrame& body, sim::ItemId item, const MeleePose& pose,
				  float bowDraw = 0);

/**
 * The same item as a drawn glyph, for the belt, the pack and the hand.
 *
 * `angle` turns the picture about its own middle, which is how a thing in a
 * hand is drawn: there is one picture of a hatchet and it is used everywhere,
 * so the hatchet on your belt and the hatchet you are swinging cannot drift
 * into two different hatchets.
 */
void drawItemIcon(Paint& paint, sim::ItemId item, float x, float y, float size, float angle = 0);

/**
 * How one item sits in a fist.
 *
 * Read off each glyph: where its handle ends, and which way its blade or its
 * barrel points. `size` is how big the picture is drawn in the hand, and the
 * grip is the point of the picture that lands on the knuckles.
 */
struct HeldPose {
	float size;
	float gripX;
	float gripY;
	float angle;
};

const HeldPose& heldPoseOf(sim::ItemId item);

/** How a given item is swung. */
MeleeStyle meleeStyleOf(sim::ItemId item);

}  // namespace client
