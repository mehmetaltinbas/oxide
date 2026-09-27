#pragma once

namespace client {

/** One layer of a gun's report: a burst of filtered noise, or a sliding tone. */
struct GunSound {
	struct Crack {
		double hz;
		double dur;
		double peak;
		/** 0 lowpass, 1 highpass, 2 bandpass, as the browser game named them. */
		int filter;
	} crack;
	struct Thump {
		double from;
		double to;
		double dur;
		double peak;
		/** 0 sine, 1 square, 2 triangle, 3 sawtooth. */
		int wave;
	} thump;
	struct Tail {
		double hz;
		double dur;
		double peak;
	} tail;
	bool hasTail;
	/** Mechanical clicks after the shot, in seconds: a pump racking. */
	double mech[2];
	int mechCount;
};

}  // namespace client
