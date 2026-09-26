#pragma once

#include <vector>

#include "sim/inventory.hpp"

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

/** One thing being made: what, and how long is left of it. */
struct CraftJob {
    int id;
    const Recipe* recipe;
    double left;
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
    /** How many jobs may be waiting at once. */
    static constexpr int kQueueMax = 8;

    /** Queues one, taking its cost. Says whether it went on. */
    bool queue(Inventory& inventory, const Recipe& recipe, int benchTier);
    /** Takes one back off and returns what it cost. */
    void cancel(Inventory& inventory, int jobId);

    /** Runs the front of the queue, and puts what is finished in the pack. */
    void update(double dt, Inventory& inventory);

    const std::vector<CraftJob>& jobs() const { return jobs_; }

private:
    std::vector<CraftJob> jobs_;
    int nextId_ = 1;
};

}  // namespace sim
