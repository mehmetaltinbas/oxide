#pragma once

#include <functional>
#include <vector>

#include "paint.hpp"
#include "sim/craft.hpp"
#include "sim/deployable.hpp"
#include "sim/inventory.hpp"

namespace client {

/**
 * The pack and the bench: what you are carrying, and what you can make of it.
 *
 * One screen rather than two, because at this stage everything you can make is
 * made out of what is in the pack, and having both in front of you is how you
 * decide whether to chop another tree or go home.
 */
class Panel {
public:
    bool open() const { return open_; }
    void toggle() {
        open_ = !open_;
        if (!open_) {
            container_ = nullptr;
            fire_ = nullptr;
        }
    }
    void close() {
        open_ = false;
        container_ = nullptr;
        fire_ = nullptr;
    }

    /**
     * Opens onto something's insides rather than onto the recipes: a box, a
     * fire, a crate at a monument. `fire` is the thing burning, when the thing
     * being looked into is one that burns.
     */
    void openContainer(sim::Container* container, const char* title,
                       const sim::Deployable* fire = nullptr) {
        open_ = true;
        container_ = container;
        title_ = title;
        fire_ = fire;
    }
    const sim::Container* container() const { return container_; }

    /**
     * A click at a point on the screen. Left queues what is under it, right
     * cancels a job in the queue. Says whether the click was the panel's.
     */
    bool click(sim::Inventory& inventory, sim::Crafting& crafting, float x, float y, bool right,
               int width, int height, float uiScale);

    /**
     * Picking a stack up to carry it somewhere.
     *
     * Dragging is how a pack is meant to be sorted: press on a slot, let go on
     * another, and the two swap or merge. Letting go outside the screen throws
     * the stack on the floor, which is what `dropped` is called with.
     */
    void press(sim::Inventory& inventory, float x, float y, int width, int height, float uiScale);
    void release(sim::Inventory& inventory, float x, float y, int width, int height, float uiScale,
                 const std::function<void(sim::ItemStack)>& dropped);

    /** What is being carried on the cursor, if anything. */
    const sim::ItemStack& dragging() const { return drag_; }

    /** The best workbench within reach, which decides what can be made. */
    void setBench(int tier) { bench_ = tier; }

    /** A transfer in progress, which finishes on its own. */
    void update(double dt, sim::Inventory& inventory);

    /** Creative mode puts a shelf of every item where the recipes go. */
    void setShelf(bool on) { shelf_ = on; }

    /** Whether a queue-is-full refusal happened since this was last asked. */
    bool takeQueueFull() {
        const bool was = queueFull_;
        queueFull_ = false;
        return was;
    }

    void draw(Paint& paint, const sim::Inventory& inventory, const sim::Crafting& crafting,
              int width, int height, float uiScale) const;

private:
    bool open_ = false;
    /** What is being looked into, or nothing when it is the bench. */
    sim::Container* container_ = nullptr;
    const char* title_ = "";
    int bench_ = 0;
    bool shelf_ = false;
    bool queueFull_ = false;
    /** What is being moved, where to, and how long is left of moving it. */
    struct Move {
        bool intoContainer = false;
        bool fromBelt = false;
        int slot = 0;
        double left = 0;
        double total = 0;
    };
    Move move_;
    /** The stack on the cursor, and where it came from if it goes back. */
    sim::ItemStack drag_{};
    enum class From : std::uint8_t { None, Belt, Pack, Container };
    From dragFrom_ = From::None;
    int dragSlot_ = 0;

    /** Puts a carried stack back where it came from, or anywhere it fits. */
    void putBack(sim::Inventory& inventory, const sim::ItemStack& stack);
    const sim::Deployable* fire_ = nullptr;

    /** Where everything is, worked out once and used by both drawing and clicks. */
    struct Layout {
        float x;
        float y;
        float w;
        float h;
        float slot;
        float packX;
        float packY;
        float listX;
        float listY;
        float rowH;
        float rowW;
        float queueY;
    };

    static Layout layoutOf(int width, int height, float uiScale);
};

}  // namespace client
