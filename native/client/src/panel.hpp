#pragma once

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

    void draw(Paint& paint, const sim::Inventory& inventory, const sim::Crafting& crafting,
              int width, int height, float uiScale) const;

private:
    bool open_ = false;
    /** What is being looked into, or nothing when it is the bench. */
    sim::Container* container_ = nullptr;
    const char* title_ = "";
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
