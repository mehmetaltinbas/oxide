#pragma once

#include "client/features/render/systems/paint.hpp"
#include "client/design/types/color.struct.hpp"

namespace client {

/**
 * The interface's colours, corners and gaps.
 *
 * Nothing that draws chrome picks a colour of its own: it names one from here,
 * so changing the look of the whole interface is changing values in this file.
 * Carried over from the browser game's tokens.
 */
namespace ui {

/**
 * Panels are dark and translucent, the same material as the belt slots, so the
 * whole interface reads as one thing and the world still shows through it.
 */
inline constexpr Color kSurface{16, 18, 16, 209};
inline constexpr Color kSurfaceAlt{255, 255, 255, 15};
inline constexpr Color kHairline{255, 255, 255, 26};
inline constexpr Color kHover{255, 255, 255, 26};
inline constexpr Color kSelected{10, 110, 235, 77};
inline constexpr Color kSlotEdge{255, 255, 255, 20};
inline constexpr Color kInk{242, 241, 236, 255};
inline constexpr Color kSubtle{242, 241, 236, 158};
inline constexpr Color kFaint{242, 241, 236, 87};
inline constexpr Color kDisabled{242, 241, 236, 77};
inline constexpr Color kAccent{10, 110, 235, 255};
inline constexpr Color kAccentHover{43, 132, 242, 255};
inline constexpr Color kAccentInk{143, 188, 255, 255};
inline constexpr Color kOk{92, 208, 122, 255};
inline constexpr Color kWarn{255, 122, 98, 255};
inline constexpr Color kScrim{0, 0, 0, 100};
/** The belt and anything else floating over the world: lighter still. */
inline constexpr Color kGlass{18, 20, 18, 120};
inline constexpr Color kGlassEdge{255, 255, 255, 18};

/** Corners: belt slots and chips, buttons and wells, whole panels. */
inline constexpr float kRadiusSmall = 6;
inline constexpr float kRadiusMedium = 10;
inline constexpr float kRadiusLarge = 16;

/** Whether a point is inside a rectangle, which is most of what a menu does. */
inline bool inside(float px, float py, float x, float y, float w, float h) {
	return px >= x && px <= x + w && py >= y && py <= y + h;
}

}  // namespace ui

}  // namespace client
