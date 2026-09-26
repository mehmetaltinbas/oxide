#pragma once

#include <vector>

#include "paint.hpp"

namespace client {

/** One speck: a chip of wood, a spark, a drop of blood. */
struct Particle {
	double x;
	double y;
	double vx;
	double vy;
	double life;
	double total;
	double size;
	double gravity;
	Color color;
};

/**
 * The specks.
 *
 * Nothing in here changes the game: it is what tells you a blow landed, which
 * the numbers alone do not. Kept in the client for that reason, where the
 * server does not have to care about it.
 */
class Particles {
public:
	/** A handful thrown out from a point, in every direction or in one. */
	void burst(double x, double y, int count, Color color, double speed, double life, double size,
			   double gravity = 0, double dir = 0, double spread = 6.28318530718);

	/** One ember drifting up off a fire. */
	void ember(double x, double y);

	/**
	 * A spent magazine thrown clear of a gun.
	 *
	 * Drawn rather than dotted: it is a thing, and a puff of specks would read
	 * as damage. It tumbles, slides to a stop, lies there a moment and is
	 * gone, which is what says a reload happened without leaving litter on the
	 * island for ever.
	 */
	void spentMagazine(double x, double y, double facing);

	void update(double dt);
	void draw(Paint& paint, double cameraX, double cameraY, double scale, int width,
			  int height) const;

	/** How much the camera is still shaking, and by how much. */
	void shake(double amount, double seconds);
	void shakeOffset(double& x, double& y) const;

private:
	/** One of them, lying where it was thrown. */
	struct Casing {
		double x;
		double y;
		double vx;
		double vy;
		double angle;
		double spin;
		double life;
		double total;
	};

	std::vector<Casing> casings_;
	std::vector<Particle> specks_;
	double shakeLeft_ = 0;
	double shakeTotal_ = 0;
	double shakeAmount_ = 0;
	std::uint32_t rolls_ = 1;
};

}  // namespace client
