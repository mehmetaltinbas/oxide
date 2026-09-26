#pragma once

#include <vector>

#include "sim/inventory.hpp"

#include <vector>

namespace sim {

/**
 * One thing you can make, what it takes, and how long it takes.
 *
 * `bench` is the workbench tier it needs: zero is anywhere, and the rest wait
 * on a bench being built, which is why only the hand-made things can be made
 * at the moment.
 */
struct Recipe {
	ItemId out;
	int amount;
	Cost cost[4];
	int costCount;
	int bench;
	double seconds;
};

/** Everything that can be made, in the order it is offered. */
const std::vector<Recipe>& recipes();

/** Whether the pack holds what a recipe asks for. */
bool canAfford(const Inventory& inventory, const Recipe& recipe);

/** How many of it the pack could pay for, at the bench you are standing at. */
/**
 * How many of a thing could be made now. In sandbox, where a recipe costs
 * nothing and needs no bench, the answer is however many the queue will hold.
 */
int craftableCount(const Inventory& inventory, const Recipe& recipe, int benchTier,
				   bool free = false);

/**
 * One order on the bench: what, how many, and how long is left of the one
 * currently under the hammer.
 *
 * Twenty arrows is one order of twenty, not twenty orders. A queue that fills
 * up with identical single jobs is unreadable and unarrangeable, and eight of
 * them was a queue you could fill by asking for one stack of anything.
 */
struct CraftJob {
	int id;
	const Recipe* recipe;
	double left;
	/** How many are still to be made, this one included. */
	int count = 1;
	/** How many were asked for, so the screen can show how far through it is. */
	int ordered = 1;
	/** Whether it cost anything, so a cancelled sandbox job refunds nothing. */
	bool paid = true;
};

/**
 * The queue.
 *
 * Materials are taken when a job is queued, not when it finishes, so two
 * hatchets in the queue cost two hatchets' worth of wood up front and a
 * cancelled job hands back exactly what it took.
 */
class Crafting {
public:
	/** How many orders may be waiting at once, whatever their size. */
	static constexpr int kQueueMax = 10;
	/** And how many of one thing a single order may be for. */
	static constexpr int kBatchMax = 999;

	/**
	 * Sandbox: any recipe, at any bench, for nothing.
	 *
	 * The one thing it does not do is hand out ammunition, which is still made
	 * a box at a time and still runs out in a fight.
	 */
	void setFree(bool free) { free_ = free; }
	bool isFree() const { return free_; }

	/**
	 * Puts an order on the bench, taking what all of it costs up front. Says
	 * whether it went on. An order for more than one lands as one entry.
	 */
	bool queue(Inventory& inventory, const Recipe& recipe, int benchTier, int count = 1);
	/** Takes one back off and returns what it cost. */
	void cancel(Inventory& inventory, int jobId);

	/**
	 * Moves a job one place up or down the queue.
	 *
	 * The one at the front is the one being made, so moving something to the
	 * front is how you say "that first". Nothing is refunded and nothing is
	 * restarted: only the order changes, and the job at the front keeps the
	 * progress it has.
	 */
	void reorder(int jobId, int by);

	/**
	 * Empties the queue and hands back everything it was holding.
	 *
	 * Called when you die: what you had half made is not yours any more, and
	 * the materials fall where you did along with the rest of your pack.
	 */
	std::vector<ItemStack> abandon();

	/** Runs the front of the queue, and puts what is finished in the pack. */
	void update(double dt, Inventory& inventory);

	const std::vector<CraftJob>& jobs() const { return jobs_; }

private:
	std::vector<CraftJob> jobs_;
	int nextId_ = 1;
	bool free_ = false;
};

}  // namespace sim
