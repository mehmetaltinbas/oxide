#pragma once

#include <functional>
#include <string>
#include <vector>

#include "paint.hpp"
#include "sim/craft.hpp"
#include "sim/deployable.hpp"
#include "sim/inventory.hpp"
#include "text.hpp"

namespace client {

/**
 * What the detail pane's buttons do.
 *
 * The pack screen knows what a thing is and where it sits; it does not know how
 * eating or getting dressed works, so the game hands it these and the screen
 * calls whichever one the button says.
 */
struct ItemActions {
    std::function<void(sim::ItemId)> consume;
    std::function<void(sim::ItemId)> wear;
    std::function<void()> takeOff;
    std::function<void(sim::ItemStack)> drop;
};

/** Which face of the one screen is showing. */
enum class Tab { Inventory, Craft, Sandbox, Container };

/**
 * The one screen: what you carry, what you can make of it, and what is inside
 * whatever you have opened.
 *
 * Tabs rather than separate screens, because they are three views of the same
 * question and you switch between them by clicking rather than by closing one
 * to open the next.
 */
class Panel {
public:
    bool open() const { return open_; }
    /** TAB: the pack, or away again. */
    void toggleInventory();
    /** C: the bench, or away again. */
    void toggleCraft();
    void close();

    /** Opens onto something's insides: a box, a fire, a crate at a monument. */
    void openContainer(sim::Container* container, const char* title,
                       const sim::Deployable* fire = nullptr);
    const sim::Container* container() const { return container_; }

    /** The best workbench within reach, which decides what can be made. */
    void setBench(int tier) { bench_ = tier; }
    /** Creative mode adds a shelf of every item there is. */
    void setSandbox(bool on) { sandbox_ = on; }

    /** Looks at one pack slot, as a click on it would. */
    void inspect(int packSlot) {
        inspecting_ = packSlot;
        inspectingWorn_ = false;
    }

    /** What Eat, Wear, Take off and Drop do. Handed over once at startup. */
    void useActions(ItemActions actions) { actions_ = std::move(actions); }

    /** Whether a queue-is-full refusal happened since this was last asked. */
    bool takeQueueFull() {
        const bool was = queueFull_;
        queueFull_ = false;
        return was;
    }

    /** A click. Says whether the screen took it. */
    bool click(sim::Inventory& inventory, sim::Crafting& crafting, float x, float y, bool right,
               int width, int height, float uiScale);
    /** Picking a stack up to carry it somewhere, and putting it down again. */
    void press(sim::Inventory& inventory, float x, float y, int width, int height, float uiScale);
    void release(sim::Inventory& inventory, float x, float y, int width, int height, float uiScale,
                 const std::function<void(sim::ItemStack)>& dropped);
    const sim::ItemStack& dragging() const { return drag_; }

    /** A transfer in progress, which finishes on its own. */
    void update(double dt, sim::Inventory& inventory);

    void draw(Paint& paint, const sim::Inventory& inventory, const sim::Crafting& crafting,
              int width, int height, float uiScale) const;

private:
    struct Layout {
        float x;
        float y;
        float w;
        float h;
        /** Under the header, where the columns start. */
        float top;
        float slot;
        float pitch;
    };

    Layout layoutOf(int width, int height, float uiScale) const;
    /** Where the tabs sit, and which one a point is over. */
    int tabUnder(const Layout& l, float x, float y, float uiScale) const;
    void drawTabs(Paint& paint, const Layout& l, float uiScale, float mouseX, float mouseY) const;
    void drawInventory(Paint& paint, const sim::Inventory& inventory, const Layout& l,
                       float uiScale, float mouseX, float mouseY) const;
    void drawCraft(Paint& paint, const sim::Inventory& inventory, const sim::Crafting& crafting,
                   const Layout& l, float uiScale, float mouseX, float mouseY) const;
    void drawShelf(Paint& paint, const Layout& l, float uiScale, float mouseX, float mouseY) const;
    void drawContainer(Paint& paint, const sim::Inventory& inventory, const Layout& l,
                       float uiScale, float mouseX, float mouseY) const;
    void putBack(sim::Inventory& inventory, const sim::ItemStack& stack);

    /** One button under the detail pane. */
    struct Action {
        const char* label;
        /** Drop, which is the one that loses you something. */
        bool danger;
    };
    /** What can be done with a thing, in the order the buttons stack. */
    int actionsFor(sim::ItemId id, bool worn, Action out[4]) const;
    /** Where the nth of `count` buttons sits. */
    void actionBox(const Layout& l, float uiScale, int index, int count, float& bx, float& by,
                   float& bw, float& bh) const;

    bool open_ = false;
    Tab tab_ = Tab::Inventory;
    bool sandbox_ = false;
    int bench_ = 0;
    bool queueFull_ = false;

    sim::Container* container_ = nullptr;
    std::string title_;
    const sim::Deployable* fire_ = nullptr;

    /** What is being looked at in the pack, and what is chosen at the bench. */
    int inspecting_ = -1;
    /** Which of the pack's runs the inspected slot is in. */
    bool inspectingWorn_ = false;
    int category_ = 0;
    int selected_ = -1;
    int amount_ = 1;

    struct Move {
        bool intoContainer = false;
        bool fromBelt = false;
        int slot = 0;
        double left = 0;
        double total = 0;
    };
    Move move_;

    ItemActions actions_;
    sim::ItemStack drag_{};
    enum class From : std::uint8_t { None, Belt, Pack, Container, Worn };
    From dragFrom_ = From::None;
    int dragSlot_ = 0;
};

}  // namespace client
