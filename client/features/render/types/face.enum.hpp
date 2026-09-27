#pragma once

namespace client {

/**
 * The lettering.
 *
 * Two faces, because one cannot do both jobs. Most of this game's text is read
 * at ten or twelve pixels, and a comic display face is illegible that small: it
 * is built to shout across a panel, not to label a stack of eight iron. So
 * `Display` shouts on the few things meant to be read from across the room, and
 * `Body` carries everything you actually have to read.
 */
enum class Face { Display, Body, BodyBold };

}  // namespace client
