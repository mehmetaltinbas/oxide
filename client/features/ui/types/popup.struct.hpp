#pragma once

#include "client/design/types/color.struct.hpp"

#include <string>

namespace client {

/** A number that floats up off something and fades: what you just gained. */
struct Popup {
	std::string text;
	double x;
	double y;
	double life;
	Color color;
};

}  // namespace client
