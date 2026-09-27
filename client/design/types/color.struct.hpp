#pragma once

#include <cstdint>

namespace client {

struct Color {
	std::uint8_t r = 0;
	std::uint8_t g = 0;
	std::uint8_t b = 0;
	std::uint8_t a = 255;
};

/** A colour written the way the TypeScript game writes it: 0xRRGGBB. */
constexpr Color rgb(std::uint32_t hex, std::uint8_t alpha = 255) {
	return Color{static_cast<std::uint8_t>((hex >> 16) & 0xff), static_cast<std::uint8_t>((hex >> 8) & 0xff),
				 static_cast<std::uint8_t>(hex & 0xff), alpha};
}

}  // namespace client
