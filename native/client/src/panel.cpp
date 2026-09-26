#include "panel.hpp"

#include "hud.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>

#include "held.hpp"
#include "palette.hpp"
#include "ui.hpp"

namespace client {

namespace {

/** How the recipes are grouped, and what each group is called. */
struct Category {
    const char* label;
    sim::ItemCategory of;
};

constexpr Category kCategories[] = {
    {"Tools", sim::ItemCategory::Tool},
    {"Weapons", sim::ItemCategory::Weapon},
    {"Ammunition", sim::ItemCategory::Ammo},
    {"Construction", sim::ItemCategory::Deployable},
    {"Medical", sim::ItemCategory::Consumable},
    {"Attire", sim::ItemCategory::Clothing},
    {"Resources", sim::ItemCategory::Resource},
    {"Explosives", sim::ItemCategory::Explosive},
};
constexpr int kCategoryCount = 8;

/** The pack is six across, as the belt is. */
constexpr int kPackCols = 6;

/** One line of the interface's lettering. */
void say(Paint& paint, float x, float y, float size, Color color, const char* line,
         Face face = Face::Body, Align align = Align::Left) {
    if (Text* lettering = paint.text()) lettering->draw(line, x, y, size, color, face, align);
}

float widthOf(Paint& paint, const char* line, float size, Face face = Face::Body) {
    return paint.text() ? paint.text()->widthOf(line, size, face) : 0;
}

/** A well for one thing: the same material as the belt, with its own edge. */
void slot(Paint& paint, float x, float y, float size, bool hovered, bool selected) {
    paint.fillRoundRect(x, y, size, size, ui::kRadiusMedium,
                        selected ? ui::kSelected : ui::kSurfaceAlt);
    paint.outlineRoundRect(x, y, size, size, ui::kRadiusMedium, 1.5f,
                           hovered ? Color{120, 180, 255, 178} : ui::kSlotEdge);
}

/** A slot with whatever is in it, and how many of them. */
void filled(Paint& paint, const sim::ItemStack& stack, float x, float y, float size, float uiScale,
            bool hovered, bool selected) {
    slot(paint, x, y, size, hovered, selected);
    if (stack.id == sim::ItemId::None) return;
    drawItemIcon(paint, stack.id, x + size * 0.5f, y + size * 0.5f, size * 0.62f);
    if (stack.count > 1) {
        char count[8];
        SDL_snprintf(count, sizeof(count), "%d", stack.count);
        say(paint, x + size - 5 * uiScale, y + size - 16 * uiScale, 11 * uiScale, ui::kInk, count,
            Face::BodyBold, Align::Right);
    }
}

/** Which slot of a run a point is over, or minus one. */
int slotUnder(float px, float py, float ox, float oy, float size, float pitch, int count,
              int cols) {
    for (int i = 0; i < count; ++i) {
        const float sx = ox + (i % cols) * pitch;
        const float sy = oy + (i / cols) * pitch;
        if (ui::inside(px, py, sx, sy, size, size)) return i;
    }
    return -1;
}

}  // namespace

void Panel::toggleInventory() {
    if (open_ && tab_ == Tab::Inventory) {
        close();
        return;
    }
    open_ = true;
    tab_ = Tab::Inventory;
}

void Panel::toggleCraft() {
    if (open_ && tab_ == Tab::Craft) {
        close();
        return;
    }
    open_ = true;
    tab_ = Tab::Craft;
}

void Panel::close() {
    open_ = false;
    container_ = nullptr;
    fire_ = nullptr;
    inspecting_ = -1;
}

void Panel::openContainer(sim::Container* container, const char* title,
                          const sim::Deployable* fire) {
    open_ = true;
    tab_ = Tab::Container;
    container_ = container;
    title_ = title;
    fire_ = fire;
}

Panel::Layout Panel::layoutOf(int width, int height, float uiScale) const {
    Layout out{};
    // The bench needs more room than the pack, as it has four columns to the
    // pack's two.
    const float wanted = tab_ == Tab::Craft ? 1080.0f : 760.0f;
    const float tall = tab_ == Tab::Craft ? 560.0f : 500.0f;
    out.w = std::min(wanted * uiScale, width - 80 * uiScale);
    out.h = std::min(tall * uiScale, height - 100 * uiScale);
    out.x = std::round((width - out.w) * 0.5f);
    out.y = std::round((height - out.h) * 0.5f);
    out.top = out.y + 72 * uiScale;
    out.slot = 40 * uiScale;
    out.pitch = 46 * uiScale;
    return out;
}

int Panel::tabUnder(const Layout& l, float x, float y, float uiScale) const {
    const float h = 30 * uiScale;
    const float ty = l.y - h - 6 * uiScale;
    float tx = l.x;
    const int count = sandbox_ ? 3 : 2;
    for (int i = 0; i < count + (container_ ? 1 : 0); ++i) {
        const float w = 110 * uiScale;
        if (ui::inside(x, y, tx, ty, w, h)) return i;
        tx += w + 6 * uiScale;
    }
    return -1;
}

void Panel::drawTabs(Paint& paint, const Layout& l, float uiScale, float mouseX,
                     float mouseY) const {
    const float h = 30 * uiScale;
    const float ty = l.y - h - 6 * uiScale;
    float tx = l.x;
    const char* labels[4] = {"Inventory", "Craft", sandbox_ ? "Sandbox" : "", title_.c_str()};
    const Tab tabs[4] = {Tab::Inventory, Tab::Craft, Tab::Sandbox, Tab::Container};
    const int count = (sandbox_ ? 3 : 2) + (container_ ? 1 : 0);
    for (int i = 0; i < count; ++i) {
        const int at = (!sandbox_ && i == 2) ? 3 : i;
        const float w = 110 * uiScale;
        const bool active = tab_ == tabs[at];
        const bool hovered = ui::inside(mouseX, mouseY, tx, ty, w, h);
        paint.fillRoundRect(tx, ty, w, h, 8 * uiScale,
                            active ? ui::kSurface
                            : hovered ? Color{20, 20, 20, 184}
                                      : Color{20, 20, 20, 140});
        paint.outlineRoundRect(tx, ty, w, h, 8 * uiScale, 1, ui::kHairline);
        say(paint, tx + w * 0.5f, ty + 6 * uiScale, 13 * uiScale,
            active ? ui::kInk : Color{242, 241, 236, 184}, labels[at], Face::BodyBold,
            Align::Centre);
        tx += w + 6 * uiScale;
    }
}

void Panel::update(double dt, sim::Inventory& inventory) {
    if (move_.left <= 0 || !container_) return;
    move_.left -= dt;
    if (move_.left > 0) return;
    move_.left = 0;
    if (move_.intoContainer) {
        sim::ItemStack& stack =
            move_.fromBelt ? inventory.hotbar()[move_.slot] : inventory.pack()[move_.slot];
        if (stack.id == sim::ItemId::None) return;
        const int left = container_->add(stack.id, stack.count);
        stack.count = left;
        if (left <= 0) stack = sim::ItemStack{};
        return;
    }
    if (move_.slot >= static_cast<int>(container_->slots.size())) return;
    sim::ItemStack& stack = container_->slots[move_.slot];
    if (stack.id == sim::ItemId::None) return;
    const int left = inventory.add(stack.id, stack.count);
    stack.count = left;
    if (left <= 0) stack = sim::ItemStack{};
}

// ---- the pack

int Panel::actionsFor(sim::ItemId id, bool worn, Action out[4]) const {
    const sim::ItemDef& def = sim::itemDef(id);
    int n = 0;
    if (def.food.calories > 0 || def.food.hydration > 0 || def.food.health != 0) {
        out[n++] = Action{def.category == sim::ItemCategory::Consumable ? "Use" : "Eat", false};
    }
    // Wear only makes sense from the pack: from the worn slot the same button
    // would put on what is already on.
    if (def.category == sim::ItemCategory::Clothing && !worn) out[n++] = Action{"Wear", false};
    if (worn) out[n++] = Action{"Take off", false};
    out[n++] = Action{"Drop", true};
    return n;
}

void Panel::actionBox(const Layout& l, float uiScale, int index, int count, float& bx, float& by,
                      float& bw, float& bh) const {
    const float left = l.x + 24 * uiScale;
    bx = left + kPackCols * l.pitch + 24 * uiScale;
    bw = l.x + l.w - bx - 24 * uiScale;
    bh = 34 * uiScale;
    // Stacked upwards off the bottom of the card, so the last one is always in
    // the same place whatever the thing can do.
    by = l.y + l.h - 44 * uiScale - (count - 1 - index) * 42 * uiScale;
}

void Panel::drawInventory(Paint& paint, const sim::Inventory& inventory, const Layout& l,
                          float uiScale, float mouseX, float mouseY) const {
    const float left = l.x + 24 * uiScale;
    const float gridY = l.top + 36 * uiScale;
    say(paint, left, gridY - 20 * uiScale, 11 * uiScale, ui::kSubtle, "MAIN");
    for (int i = 0; i < sim::kPackSlots; ++i) {
        const float sx = left + (i % kPackCols) * l.pitch;
        const float sy = gridY + (i / kPackCols) * l.pitch;
        filled(paint, inventory.pack()[i], sx, sy, l.slot, uiScale,
               ui::inside(mouseX, mouseY, sx, sy, l.slot, l.slot),
               inspecting_ == i && !inspectingWorn_);
    }

    // What you have on, in a card of its own under the pack, so the worn slot
    // does not read as one more pack slot.
    const int rows = (sim::kPackSlots + kPackCols - 1) / kPackCols;
    const float wornY = gridY + rows * l.pitch + 34 * uiScale;
    paint.fillRoundRect(left - 10 * uiScale, wornY - 28 * uiScale, l.slot + 20 * uiScale,
                        l.slot + 32 * uiScale, ui::kRadiusMedium, ui::kSurfaceAlt);
    say(paint, left, wornY - 22 * uiScale, 11 * uiScale, ui::kSubtle, "WORN");
    filled(paint, inventory.worn(), left, wornY, l.slot, uiScale,
           ui::inside(mouseX, mouseY, left, wornY, l.slot, l.slot), inspectingWorn_);

    // The right-hand column: what is selected, and what it is.
    const float detX = left + kPackCols * l.pitch + 24 * uiScale;
    const sim::ItemStack chosen = inspecting_ < 0 ? sim::ItemStack{}
                                  : inspectingWorn_ ? inventory.worn()
                                                    : inventory.pack()[inspecting_];
    if (chosen.id == sim::ItemId::None) {
        say(paint, detX, l.top + 28 * uiScale, 13 * uiScale, ui::kSubtle, "Select an item");
        return;
    }
    const sim::ItemDef& def = sim::itemDef(chosen.id);
    drawItemIcon(paint, chosen.id, detX + 22 * uiScale, l.top + 40 * uiScale, 44 * uiScale);
    say(paint, detX + 56 * uiScale, l.top + 22 * uiScale, 17 * uiScale, ui::kInk, def.name,
        Face::BodyBold);
    char line[96];
    SDL_snprintf(line, sizeof(line), "%s   ·   stacks to %d",
                 def.category == sim::ItemCategory::Tool       ? "Tool"
                 : def.category == sim::ItemCategory::Weapon   ? "Weapon"
                 : def.category == sim::ItemCategory::Ammo     ? "Ammunition"
                 : def.category == sim::ItemCategory::Consumable ? "Consumable"
                 : def.category == sim::ItemCategory::Clothing ? "Attire"
                 : def.category == sim::ItemCategory::Deployable ? "Construction"
                 : def.category == sim::ItemCategory::Explosive ? "Explosive"
                                                               : "Resource",
                 def.stack);
    say(paint, detX + 56 * uiScale, l.top + 44 * uiScale, 11 * uiScale, ui::kSubtle, line);
    say(paint, detX, l.top + 84 * uiScale, 12 * uiScale, ui::kSubtle, def.desc);

    float row = l.top + 120 * uiScale;
    const auto stat = [&](const char* name, const char* value) {
        say(paint, detX, row, 12 * uiScale, ui::kSubtle, name);
        say(paint, detX + 200 * uiScale, row, 12 * uiScale, ui::kInk, value, Face::BodyBold);
        row += 22 * uiScale;
    };
    char value[48];
    if (def.melee.damage > 0) {
        SDL_snprintf(value, sizeof(value), "%.0f", def.melee.damage);
        stat("Damage", value);
        SDL_snprintf(value, sizeof(value), "%.0f", def.melee.reach);
        stat("Reach", value);
    }
    if (def.gun.damage > 0) {
        SDL_snprintf(value, sizeof(value), "%.0f", def.gun.damage);
        stat("Damage", value);
        SDL_snprintf(value, sizeof(value), "%.0f", def.gun.range);
        stat("Range", value);
        if (def.gun.magazine > 0) {
            SDL_snprintf(value, sizeof(value), "%d", def.gun.magazine);
            stat("Magazine", value);
        }
        stat("Ammunition", sim::itemDef(def.gun.ammo).name);
    }
    if (def.wear.warmth > 0) {
        SDL_snprintf(value, sizeof(value), "%.0f", def.wear.warmth);
        stat("Warmth", value);
        SDL_snprintf(value, sizeof(value), "%.0f%%", def.wear.armor * 100);
        stat("Armour", value);
    }
    if (def.boom.damage > 0) {
        SDL_snprintf(value, sizeof(value), "%.0f", def.boom.damage);
        stat("Blast", value);
        SDL_snprintf(value, sizeof(value), "%.1fs", def.boom.fuse);
        stat("Fuse", value);
    }
    if (def.food.calories > 0 || def.food.hydration > 0 || def.food.health != 0) {
        if (def.food.calories > 0) {
            SDL_snprintf(value, sizeof(value), "+%.0f", def.food.calories);
            stat("Food", value);
        }
        if (def.food.hydration > 0) {
            SDL_snprintf(value, sizeof(value), "+%.0f", def.food.hydration);
            stat("Water", value);
        }
        if (def.food.health != 0) {
            SDL_snprintf(value, sizeof(value), "%+.0f", def.food.health);
            stat("Health", value);
        }
    }

    // What you can do with it, along the bottom of the pane: the thing you
    // reached for it to do, one click away instead of a key to remember.
    Action actions[4];
    const int count = actionsFor(chosen.id, inspectingWorn_, actions);
    for (int i = 0; i < count; ++i) {
        float bx = 0;
        float by = 0;
        float bw = 0;
        float bh = 0;
        actionBox(l, uiScale, i, count, bx, by, bw, bh);
        const bool hovered = ui::inside(mouseX, mouseY, bx, by, bw, bh);
        const Color face = actions[i].danger
                               ? (hovered ? Color{194, 69, 47, 41} : ui::kSurfaceAlt)
                               : (hovered ? ui::kAccentHover : ui::kAccent);
        paint.fillRoundRect(bx, by, bw, bh, ui::kRadiusMedium, face);
        say(paint, bx + bw / 2, by + 9 * uiScale, 14 * uiScale,
            actions[i].danger ? ui::kWarn : rgb(0xffffff), actions[i].label, Face::BodyBold,
            Align::Centre);
    }
}

// ---- the bench

void Panel::drawCraft(Paint& paint, const sim::Inventory& inventory, const sim::Crafting& crafting,
                      const Layout& l, float uiScale, float mouseX, float mouseY) const {
    const auto& all = sim::recipes();
    const float listY = l.y + 88 * uiScale;
    const float queueW = 218 * uiScale;

    // Column one: the categories, with how many recipes are in each.
    const float catX = l.x + 20 * uiScale;
    const float catW = 190 * uiScale;
    say(paint, catX + 12 * uiScale, listY - 18 * uiScale, 11 * uiScale, ui::kSubtle, "CATEGORY");
    for (int i = 0; i < kCategoryCount; ++i) {
        const float ry = listY + i * 42 * uiScale;
        const bool chosen = category_ == i;
        const bool hovered = ui::inside(mouseX, mouseY, catX, ry, catW, 38 * uiScale);
        if (chosen || hovered) {
            paint.fillRoundRect(catX, ry, catW, 38 * uiScale, ui::kRadiusMedium,
                                chosen ? ui::kSelected : ui::kHover);
        }
        say(paint, catX + 14 * uiScale, ry + 8 * uiScale, 13 * uiScale,
            chosen ? ui::kAccentInk : ui::kInk, kCategories[i].label, Face::BodyBold);
        int count = 0;
        for (const sim::Recipe& recipe : all) {
            if (sim::itemDef(recipe.out).category == kCategories[i].of) ++count;
        }
        char number[8];
        SDL_snprintf(number, sizeof(number), "%d", count);
        say(paint, catX + catW - 14 * uiScale, ry + 11 * uiScale, 11 * uiScale, ui::kSubtle,
            number, Face::Body, Align::Right);
    }

    // Column two: what is in the category, and whether you can make it.
    const float midX = catX + catW + 16 * uiScale;
    const float midW = 280 * uiScale;
    paint.fillRect(midX - 8 * uiScale, l.y + 80 * uiScale, 1, l.h - 100 * uiScale, ui::kHairline);
    char heading[32];
    SDL_snprintf(heading, sizeof(heading), "%s", kCategories[category_].label);
    for (char* at = heading; *at; ++at) *at = static_cast<char>(SDL_toupper(*at));
    say(paint, midX + 12 * uiScale, listY - 18 * uiScale, 11 * uiScale, ui::kSubtle, heading);

    int shown = 0;
    for (int i = 0; i < static_cast<int>(all.size()); ++i) {
        const sim::Recipe& recipe = all[i];
        if (sim::itemDef(recipe.out).category != kCategories[category_].of) continue;
        const float ry = listY + shown * 40 * uiScale;
        ++shown;
        if (ry + 36 * uiScale > l.y + l.h - 20 * uiScale) break;
        const bool locked = recipe.bench > bench_;
        const bool ready = !locked && sim::canAfford(inventory, recipe);
        const bool chosen = selected_ == i;
        const bool hovered = ui::inside(mouseX, mouseY, midX, ry, midW, 36 * uiScale);
        if (chosen || hovered) {
            paint.fillRoundRect(midX, ry, midW, 36 * uiScale, ui::kRadiusMedium,
                                chosen ? ui::kSelected : ui::kHover);
        }
        drawItemIcon(paint, recipe.out, midX + 21 * uiScale, ry + 18 * uiScale, 26 * uiScale);
        char name[64];
        if (recipe.amount > 1) {
            SDL_snprintf(name, sizeof(name), "%s x%d", sim::itemDef(recipe.out).name,
                         recipe.amount);
        } else {
            SDL_snprintf(name, sizeof(name), "%s", sim::itemDef(recipe.out).name);
        }
        say(paint, midX + 42 * uiScale, ry + 3 * uiScale, 13 * uiScale,
            locked ? ui::kDisabled : chosen ? ui::kAccentInk : ui::kInk, name, Face::BodyBold);
        char state[32];
        if (locked) {
            SDL_snprintf(state, sizeof(state), "Workbench %d", recipe.bench);
        } else {
            SDL_snprintf(state, sizeof(state), "%s", ready ? "Ready" : "Missing materials");
        }
        say(paint, midX + 42 * uiScale, ry + 19 * uiScale, 11 * uiScale,
            locked ? ui::kDisabled : ready ? ui::kOk : ui::kWarn, state);
    }

    // Column three: the one that is chosen.
    const float detX = midX + midW + 24 * uiScale;
    const float detW = l.x + l.w - detX - 24 * uiScale - queueW - 24 * uiScale;
    paint.fillRect(detX - 12 * uiScale, l.y + 80 * uiScale, 1, l.h - 100 * uiScale, ui::kHairline);

    // Column four: the queue.
    const float queueX = l.x + l.w - queueW - 20 * uiScale;
    paint.fillRect(queueX - 16 * uiScale, l.y + 80 * uiScale, 1, l.h - 100 * uiScale,
                   ui::kHairline);
    char queueLine[32];
    SDL_snprintf(queueLine, sizeof(queueLine), "QUEUE  %zu/%d", crafting.jobs().size(),
                 sim::Crafting::kQueueMax);
    say(paint, queueX, listY - 18 * uiScale, 11 * uiScale, ui::kSubtle, queueLine);
    if (crafting.jobs().empty()) {
        say(paint, queueX, listY + 4 * uiScale, 12 * uiScale, ui::kFaint, "nothing queued");
    } else {
        for (std::size_t i = 0; i < crafting.jobs().size(); ++i) {
            const sim::CraftJob& job = crafting.jobs()[i];
            const float jy = listY + i * 42 * uiScale;
            paint.fillRoundRect(queueX, jy, queueW - 20 * uiScale, 36 * uiScale,
                                ui::kRadiusMedium, ui::kSurfaceAlt);
            drawItemIcon(paint, job.recipe->out, queueX + 20 * uiScale, jy + 18 * uiScale,
                         24 * uiScale);
            say(paint, queueX + 40 * uiScale, jy + 4 * uiScale, 12 * uiScale, ui::kInk,
                sim::itemDef(job.recipe->out).name, Face::BodyBold);
            // How far through it is, along the bottom of its chip.
            const double done = 1 - job.left / job.recipe->seconds;
            paint.fillRect(queueX + 8 * uiScale, jy + 28 * uiScale,
                           static_cast<float>((queueW - 36 * uiScale) * done), 3 * uiScale,
                           ui::kAccent);
        }
        say(paint, queueX, l.y + l.h - 30 * uiScale, 11 * uiScale, ui::kFaint,
            "right click to cancel");
    }

    if (selected_ < 0 || selected_ >= static_cast<int>(all.size())) {
        say(paint, detX, listY, 12 * uiScale, ui::kSubtle, "Select an item");
        return;
    }
    const sim::Recipe& recipe = all[selected_];
    const sim::ItemDef& def = sim::itemDef(recipe.out);
    drawItemIcon(paint, recipe.out, detX + 22 * uiScale, listY + 18 * uiScale, 44 * uiScale);
    say(paint, detX + 56 * uiScale, listY - 2 * uiScale, 17 * uiScale, ui::kInk, def.name,
        Face::BodyBold);
    char timing[32];
    SDL_snprintf(timing, sizeof(timing), "%.0fs to craft", recipe.seconds);
    say(paint, detX + 56 * uiScale, listY + 22 * uiScale, 11 * uiScale, ui::kSubtle, timing);
    say(paint, detX, listY + 60 * uiScale, 12 * uiScale, ui::kSubtle, def.desc);

    say(paint, detX, listY + 118 * uiScale, 11 * uiScale, ui::kSubtle, "MATERIALS");
    for (int i = 0; i < recipe.costCount; ++i) {
        const sim::Cost& cost = recipe.cost[i];
        const int have = inventory.count(cost.id);
        const float ry = listY + (140 + i * 26) * uiScale;
        drawItemIcon(paint, cost.id, detX + 8 * uiScale, ry + 8 * uiScale, 18 * uiScale);
        say(paint, detX + 26 * uiScale, ry, 12 * uiScale, ui::kInk, sim::itemDef(cost.id).name);
        char amount[32];
        SDL_snprintf(amount, sizeof(amount), "%d / %d", std::min(have, cost.count), cost.count);
        say(paint, detX + detW, ry, 12 * uiScale, have >= cost.count ? ui::kOk : ui::kWarn, amount,
            Face::BodyBold, Align::Right);
    }

    // How many, and the button that starts them.
    const int possible = sim::craftableCount(inventory, recipe, bench_, sandbox_);
    const float qy = l.y + l.h - 124 * uiScale;
    say(paint, detX, qy - 18 * uiScale, 11 * uiScale, ui::kSubtle, "AMOUNT");
    const float stepW = 34 * uiScale;
    const auto stepper = [&](const char* label, float sx, bool enabled) {
        const bool hovered = ui::inside(mouseX, mouseY, sx, qy, stepW, stepW) && enabled;
        paint.fillRoundRect(sx, qy, stepW, stepW, ui::kRadiusSmall,
                            hovered ? ui::kHover : ui::kSurfaceAlt);
        say(paint, sx + stepW * 0.5f, qy + 7 * uiScale, 15 * uiScale,
            enabled ? ui::kInk : ui::kDisabled, label, Face::BodyBold, Align::Centre);
    };
    stepper("-", detX, amount_ > 1);
    paint.fillRoundRect(detX + 40 * uiScale, qy, 58 * uiScale, stepW, ui::kRadiusSmall,
                        ui::kSurfaceAlt);
    paint.outlineRoundRect(detX + 40 * uiScale, qy, 58 * uiScale, stepW, ui::kRadiusSmall, 1,
                           ui::kHairline);
    char amount[8];
    SDL_snprintf(amount, sizeof(amount), "%d", amount_);
    say(paint, detX + 69 * uiScale, qy + 7 * uiScale, 15 * uiScale, ui::kInk, amount,
        Face::BodyBold, Align::Centre);
    stepper("+", detX + 104 * uiScale, amount_ < std::max(1, possible));
    const float maxX = detX + 144 * uiScale;
    const bool maxHover = ui::inside(mouseX, mouseY, maxX, qy, 52 * uiScale, stepW);
    paint.fillRoundRect(maxX, qy, 52 * uiScale, stepW, ui::kRadiusSmall,
                        maxHover ? ui::kHover : ui::kSurfaceAlt);
    say(paint, maxX + 26 * uiScale, qy + 7 * uiScale, 13 * uiScale, ui::kInk, "max",
        Face::BodyBold, Align::Centre);
    char possibleLine[32];
    SDL_snprintf(possibleLine, sizeof(possibleLine), "%d possible", possible);
    say(paint, detX + 210 * uiScale, qy + 9 * uiScale, 11 * uiScale, ui::kSubtle, possibleLine);

    const float buttonY = qy + 46 * uiScale;
    const bool can = possible >= amount_ && amount_ > 0;
    const bool buttonHover = ui::inside(mouseX, mouseY, detX, buttonY, detW, 42 * uiScale) && can;
    paint.fillRoundRect(detX, buttonY, detW, 42 * uiScale, ui::kRadiusMedium,
                        !can ? Color{255, 255, 255, 13}
                             : buttonHover ? ui::kAccentHover
                                           : ui::kAccent);
    say(paint, detX + detW * 0.5f, buttonY + 11 * uiScale, 16 * uiScale,
        can ? ui::kInk : ui::kDisabled, "Craft", Face::BodyBold, Align::Centre);
    say(paint, detX + detW * 0.5f, buttonY + 48 * uiScale, 11 * uiScale, ui::kFaint,
        "shift +10   ·   ctrl all", Face::Body, Align::Centre);
}

// ---- the shelf, and whatever is open

void Panel::drawShelf(Paint& paint, const Layout& l, float uiScale, float mouseX,
                      float mouseY) const {
    const float left = l.x + 24 * uiScale;
    const float top = l.top + 36 * uiScale;
    say(paint, left, top - 20 * uiScale, 11 * uiScale, ui::kSubtle,
        "EVERY ITEM   ·   click for one, right click for a stack");
    const int cols = 12;
    for (int i = 1; i < sim::kItemCount; ++i) {
        const int at = i - 1;
        const float sx = left + (at % cols) * l.pitch;
        const float sy = top + (at / cols) * l.pitch;
        if (sy + l.slot > l.y + l.h - 20 * uiScale) break;
        slot(paint, sx, sy, l.slot, ui::inside(mouseX, mouseY, sx, sy, l.slot, l.slot), false);
        drawItemIcon(paint, static_cast<sim::ItemId>(i), sx + l.slot * 0.5f, sy + l.slot * 0.5f,
                     l.slot * 0.62f);
    }
}

void Panel::drawContainer(Paint& paint, const sim::Inventory& inventory, const Layout& l,
                          float uiScale, float mouseX, float mouseY) const {
    const float left = l.x + 24 * uiScale;
    const float gridY = l.top + 36 * uiScale;
    say(paint, left, gridY - 20 * uiScale, 11 * uiScale, ui::kSubtle, "MAIN");
    for (int i = 0; i < sim::kPackSlots; ++i) {
        const float sx = left + (i % kPackCols) * l.pitch;
        const float sy = gridY + (i / kPackCols) * l.pitch;
        filled(paint, inventory.pack()[i], sx, sy, l.slot, uiScale,
               ui::inside(mouseX, mouseY, sx, sy, l.slot, l.slot), false);
    }

    const float inX = left + kPackCols * l.pitch + 24 * uiScale;
    say(paint, inX, gridY - 20 * uiScale, 11 * uiScale, ui::kSubtle, title_.c_str());
    if (!container_) return;
    for (std::size_t i = 0; i < container_->slots.size(); ++i) {
        const float sx = inX + (i % 4) * l.pitch;
        const float sy = gridY + (i / 4) * l.pitch;
        filled(paint, container_->slots[i], sx, sy, l.slot, uiScale,
               ui::inside(mouseX, mouseY, sx, sy, l.slot, l.slot), false);
    }
    if (fire_) {
        char line[64];
        SDL_snprintf(line, sizeof(line), "%s   ·   %s", fire_->lit ? "Lit" : "Out",
                     fire_->lit ? "burning wood" : "E to light it");
        say(paint, inX, gridY + 3 * l.pitch + 16 * uiScale, 12 * uiScale, ui::kSubtle, line);
    }
    if (move_.left > 0 && move_.total > 0) {
        const float w = 160 * uiScale;
        const float by = gridY + 3 * l.pitch + 40 * uiScale;
        paint.fillRoundRect(inX, by, w, 8 * uiScale, ui::kRadiusSmall, ui::kSurfaceAlt);
        paint.fillRoundRect(inX, by, static_cast<float>(w * (1 - move_.left / move_.total)),
                            8 * uiScale, ui::kRadiusSmall, ui::kAccent);
    }
}

// ---- the whole screen

void Panel::draw(Paint& paint, const sim::Inventory& inventory, const sim::Crafting& crafting,
                 int width, int height, float uiScale) const {
    if (!open_) return;
    const Layout l = layoutOf(width, height, uiScale);
    float mouseX = 0;
    float mouseY = 0;
    SDL_GetMouseState(&mouseX, &mouseY);
    mouseX *= uiScale;
    mouseY *= uiScale;

    // The world dimmed behind it, then the panel: dark, translucent, one
    // hairline round it, so the island still shows through an open menu.
    paint.fillRect(0, 0, static_cast<float>(width), static_cast<float>(height), ui::kScrim);
    paint.fillRoundRect(l.x, l.y, l.w, l.h, ui::kRadiusLarge, ui::kSurface);
    paint.outlineRoundRect(l.x, l.y, l.w, l.h, ui::kRadiusLarge, 1, ui::kHairline);
    drawTabs(paint, l, uiScale, mouseX, mouseY);

    const char* title = tab_ == Tab::Craft     ? "Crafting"
                        : tab_ == Tab::Sandbox ? "Sandbox"
                        : tab_ == Tab::Container ? title_.c_str()
                                                 : "Inventory";
    say(paint, l.x + 26 * uiScale, l.y + 14 * uiScale, 21 * uiScale, ui::kInk, title,
        Face::BodyBold);

    char under[96];
    if (tab_ == Tab::Craft) {
        if (bench_ > 0) {
            SDL_snprintf(under, sizeof(under), "Workbench level %d nearby", bench_);
        } else {
            SDL_snprintf(under, sizeof(under), "No workbench nearby, level 0 recipes only");
        }
    } else if (tab_ == Tab::Sandbox) {
        SDL_snprintf(under, sizeof(under), "Nothing costs anything here");
    } else {
        SDL_snprintf(under, sizeof(under), "drag to move  ·  drag out to drop  ·  click to inspect");
    }
    say(paint, l.x + 26 * uiScale, l.y + 44 * uiScale, 12 * uiScale, ui::kSubtle, under);
    say(paint, l.x + l.w - 26 * uiScale, l.y + 44 * uiScale, 12 * uiScale, ui::kSubtle,
        tab_ == Tab::Craft ? "esc to close" : "tab to close", Face::Body, Align::Right);
    paint.fillRect(l.x + 1, l.y + 72 * uiScale, l.w - 2, 1, ui::kHairline);

    switch (tab_) {
        case Tab::Inventory: drawInventory(paint, inventory, l, uiScale, mouseX, mouseY); break;
        case Tab::Craft: drawCraft(paint, inventory, crafting, l, uiScale, mouseX, mouseY); break;
        case Tab::Sandbox: drawShelf(paint, l, uiScale, mouseX, mouseY); break;
        case Tab::Container: drawContainer(paint, inventory, l, uiScale, mouseX, mouseY); break;
    }

    if (drag_.id != sim::ItemId::None) {
        // What is on the cursor, under it.
        drawItemIcon(paint, drag_.id, mouseX, mouseY, l.slot * 0.62f);
        if (drag_.count > 1) {
            char count[8];
            SDL_snprintf(count, sizeof(count), "%d", drag_.count);
            say(paint, mouseX + 10 * uiScale, mouseY + 6 * uiScale, 11 * uiScale, ui::kInk, count,
                Face::BodyBold);
        }
    }
}

// ---- what a click does

bool Panel::click(sim::Inventory& inventory, sim::Crafting& crafting, float x, float y, bool right,
                  int width, int height, float uiScale) {
    if (!open_) return false;
    const Layout l = layoutOf(width, height, uiScale);

    const int tab = tabUnder(l, x, y, uiScale);
    if (tab >= 0) {
        const Tab order[4] = {Tab::Inventory, Tab::Craft, Tab::Sandbox, Tab::Container};
        const int at = (!sandbox_ && tab == 2) ? 3 : tab;
        if (at != 3 || container_) {
            tab_ = order[at];
            inspecting_ = -1;
        }
        return true;
    }
    if (!ui::inside(x, y, l.x, l.y, l.w, l.h)) return false;

    const float left = l.x + 24 * uiScale;
    const float gridY = l.top + 36 * uiScale;

    if (tab_ == Tab::Inventory && inspecting_ >= 0) {
        // The buttons under the detail pane, before the grid: they sit inside
        // the card too, and a click on one is not a click on a slot.
        const sim::ItemStack chosen =
            inspectingWorn_ ? inventory.worn() : inventory.pack()[inspecting_];
        if (chosen.id != sim::ItemId::None) {
            Action actions[4];
            const int count = actionsFor(chosen.id, inspectingWorn_, actions);
            for (int i = 0; i < count; ++i) {
                float bx = 0;
                float by = 0;
                float bw = 0;
                float bh = 0;
                actionBox(l, uiScale, i, count, bx, by, bw, bh);
                if (!ui::inside(x, y, bx, by, bw, bh)) continue;
                const std::string label = actions[i].label;
                if (label == "Drop") {
                    if (actions_.drop) actions_.drop(chosen);
                    (inspectingWorn_ ? inventory.worn() : inventory.pack()[inspecting_]) =
                        sim::ItemStack{};
                    inspecting_ = -1;
                } else if (label == "Wear") {
                    if (actions_.wear) actions_.wear(chosen.id);
                } else if (label == "Take off") {
                    if (actions_.takeOff) actions_.takeOff();
                    inspecting_ = -1;
                } else if (actions_.consume) {
                    actions_.consume(chosen.id);
                }
                return true;
            }
        }
    }

    if (tab_ == Tab::Inventory) {
        const int at = slotUnder(x, y, left, gridY, l.slot, l.pitch, sim::kPackSlots, kPackCols);
        if (at >= 0) {
            inspecting_ = at;
            inspectingWorn_ = false;
            return true;
        }
        const int rows = (sim::kPackSlots + kPackCols - 1) / kPackCols;
        const float wornY = gridY + rows * l.pitch + 34 * uiScale;
        if (ui::inside(x, y, left, wornY, l.slot, l.slot)) {
            inspecting_ = 0;
            inspectingWorn_ = true;
            return true;
        }
        return true;
    }

    if (tab_ == Tab::Sandbox) {
        const int cols = 12;
        const float top = l.top + 36 * uiScale;
        for (int i = 1; i < sim::kItemCount; ++i) {
            const int at = i - 1;
            const float sx = left + (at % cols) * l.pitch;
            const float sy = top + (at / cols) * l.pitch;
            if (!ui::inside(x, y, sx, sy, l.slot, l.slot)) continue;
            const auto id = static_cast<sim::ItemId>(i);
            inventory.add(id, right ? sim::itemDef(id).stack : 1);
            return true;
        }
        return true;
    }

    if (tab_ == Tab::Container && container_) {
        const auto start = [&](bool into, bool belt, int at, int count) {
            move_ = Move{into, belt, at, sim::Transfer::seconds(count),
                         sim::Transfer::seconds(count)};
        };
        const int packAt =
            slotUnder(x, y, left, gridY, l.slot, l.pitch, sim::kPackSlots, kPackCols);
        if (packAt >= 0 && move_.left <= 0) {
            const sim::ItemStack& stack = inventory.pack()[packAt];
            if (stack.id != sim::ItemId::None) start(true, false, packAt, stack.count);
            return true;
        }
        const float inX = left + kPackCols * l.pitch + 24 * uiScale;
        const int inAt = slotUnder(x, y, inX, gridY, l.slot, l.pitch,
                                   static_cast<int>(container_->slots.size()), 4);
        if (inAt >= 0 && move_.left <= 0) {
            const sim::ItemStack& stack = container_->slots[inAt];
            if (stack.id != sim::ItemId::None) start(false, false, inAt, stack.count);
            return true;
        }
        return true;
    }

    // The bench.
    const auto& all = sim::recipes();
    const float listY = l.y + 88 * uiScale;
    const float catX = l.x + 20 * uiScale;
    const float catW = 190 * uiScale;
    for (int i = 0; i < kCategoryCount; ++i) {
        if (!ui::inside(x, y, catX, listY + i * 42 * uiScale, catW, 38 * uiScale)) continue;
        category_ = i;
        selected_ = -1;
        amount_ = 1;
        return true;
    }

    const float midX = catX + catW + 16 * uiScale;
    const float midW = 280 * uiScale;
    int shown = 0;
    for (int i = 0; i < static_cast<int>(all.size()); ++i) {
        if (sim::itemDef(all[i].out).category != kCategories[category_].of) continue;
        const float ry = listY + shown * 40 * uiScale;
        ++shown;
        if (!ui::inside(x, y, midX, ry, midW, 36 * uiScale)) continue;
        selected_ = i;
        amount_ = 1;
        return true;
    }

    // A job taken back off the queue.
    const float queueW = 218 * uiScale;
    const float queueX = l.x + l.w - queueW - 20 * uiScale;
    for (std::size_t i = 0; i < crafting.jobs().size(); ++i) {
        if (!ui::inside(x, y, queueX, listY + i * 42 * uiScale, queueW - 20 * uiScale,
                        36 * uiScale)) {
            continue;
        }
        if (right) crafting.cancel(inventory, crafting.jobs()[i].id);
        return true;
    }

    if (selected_ < 0 || selected_ >= static_cast<int>(all.size())) return true;
    const sim::Recipe& recipe = all[selected_];
    const float detX = midX + midW + 24 * uiScale;
    const float detW = l.x + l.w - detX - 24 * uiScale - queueW - 24 * uiScale;
    const float qy = l.y + l.h - 124 * uiScale;
    const float stepW = 34 * uiScale;
    const int possible = sim::craftableCount(inventory, recipe, bench_, sandbox_);
    if (ui::inside(x, y, detX, qy, stepW, stepW)) {
        amount_ = std::max(1, amount_ - 1);
        return true;
    }
    if (ui::inside(x, y, detX + 104 * uiScale, qy, stepW, stepW)) {
        amount_ = std::min(std::max(1, possible), amount_ + 1);
        return true;
    }
    if (ui::inside(x, y, detX + 144 * uiScale, qy, 52 * uiScale, stepW)) {
        amount_ = std::max(1, possible);
        return true;
    }
    if (ui::inside(x, y, detX, qy + 46 * uiScale, detW, 42 * uiScale)) {
        // However many were asked for, one job each, up to what the queue holds.
        for (int i = 0; i < amount_; ++i) {
            if (static_cast<int>(crafting.jobs().size()) >= sim::Crafting::kQueueMax) {
                queueFull_ = true;
                break;
            }
            if (!crafting.queue(inventory, recipe, bench_)) break;
        }
        amount_ = 1;
        return true;
    }
    return true;
}

// ---- dragging a stack about

void Panel::press(sim::Inventory& inventory, float x, float y, int width, int height,
                  float uiScale) {
    if (!open_ || drag_.id != sim::ItemId::None) return;
    // The belt is reachable from every tab: it is on the screen the whole time
    // the pack is open, so it would be strange for it to stop taking stacks
    // because you are looking at the bench.
    const int belt = beltSlotUnder(x, y, width, height, uiScale);
    if (belt >= 0) {
        if (inventory.hotbar()[belt].id == sim::ItemId::None) return;
        drag_ = inventory.hotbar()[belt];
        inventory.hotbar()[belt] = sim::ItemStack{};
        dragFrom_ = From::Belt;
        dragSlot_ = belt;
        return;
    }
    if (tab_ != Tab::Inventory && tab_ != Tab::Container) return;
    const Layout l = layoutOf(width, height, uiScale);
    const float left = l.x + 24 * uiScale;
    const float gridY = l.top + 36 * uiScale;

    const int pack = slotUnder(x, y, left, gridY, l.slot, l.pitch, sim::kPackSlots, kPackCols);
    if (pack >= 0 && inventory.pack()[pack].id != sim::ItemId::None) {
        drag_ = inventory.pack()[pack];
        inventory.pack()[pack] = sim::ItemStack{};
        dragFrom_ = From::Pack;
        dragSlot_ = pack;
        return;
    }
    if (tab_ == Tab::Inventory) {
        const int rows = (sim::kPackSlots + kPackCols - 1) / kPackCols;
        const float wornY = gridY + rows * l.pitch + 34 * uiScale;
        if (ui::inside(x, y, left, wornY, l.slot, l.slot) &&
            inventory.worn().id != sim::ItemId::None) {
            drag_ = inventory.worn();
            inventory.worn() = sim::ItemStack{};
            dragFrom_ = From::Worn;
            return;
        }
    }
    if (tab_ == Tab::Container && container_) {
        const float inX = left + kPackCols * l.pitch + 24 * uiScale;
        const int at = slotUnder(x, y, inX, gridY, l.slot, l.pitch,
                                 static_cast<int>(container_->slots.size()), 4);
        if (at >= 0 && container_->slots[at].id != sim::ItemId::None) {
            drag_ = container_->slots[at];
            container_->slots[at] = sim::ItemStack{};
            dragFrom_ = From::Container;
            dragSlot_ = at;
        }
    }
}

void Panel::release(sim::Inventory& inventory, float x, float y, int width, int height,
                    float uiScale, const std::function<void(sim::ItemStack)>& dropped) {
    if (drag_.id == sim::ItemId::None) return;
    const sim::ItemStack carried = drag_;
    drag_ = sim::ItemStack{};
    const Layout l = layoutOf(width, height, uiScale);
    const float left = l.x + 24 * uiScale;
    const float gridY = l.top + 36 * uiScale;

    // Into whatever slot it was let go over: merged if it matches, swapped if
    // it does not.
    const auto land = [&](sim::ItemStack& slot) {
        if (slot.id == carried.id) {
            const int room = sim::itemDef(carried.id).stack - slot.count;
            const int put = std::min(room, carried.count);
            slot.count += put;
            if (put < carried.count) {
                sim::ItemStack rest = carried;
                rest.count -= put;
                putBack(inventory, rest);
            }
            return;
        }
        const sim::ItemStack was = slot;
        slot = carried;
        if (was.id != sim::ItemId::None) putBack(inventory, was);
    };

    const int belt = beltSlotUnder(x, y, width, height, uiScale);
    if (belt >= 0) {
        land(inventory.hotbar()[belt]);
        return;
    }
    const int pack = slotUnder(x, y, left, gridY, l.slot, l.pitch, sim::kPackSlots, kPackCols);
    if (pack >= 0) {
        land(inventory.pack()[pack]);
        return;
    }
    const int rows = (sim::kPackSlots + kPackCols - 1) / kPackCols;
    const float wornY = gridY + rows * l.pitch + 34 * uiScale;
    if (tab_ == Tab::Inventory && ui::inside(x, y, left, wornY, l.slot, l.slot)) {
        // Only what can actually be worn goes on.
        if (sim::itemDef(carried.id).category == sim::ItemCategory::Clothing) {
            land(inventory.worn());
        } else {
            putBack(inventory, carried);
        }
        return;
    }
    if (tab_ == Tab::Container && container_) {
        const float inX = left + kPackCols * l.pitch + 24 * uiScale;
        const int at = slotUnder(x, y, inX, gridY, l.slot, l.pitch,
                                 static_cast<int>(container_->slots.size()), 4);
        if (at >= 0) {
            land(container_->slots[at]);
            return;
        }
    }
    if (!ui::inside(x, y, l.x, l.y, l.w, l.h)) {
        // Let go outside the screen: on the floor it goes.
        dropped(carried);
        return;
    }
    putBack(inventory, carried);
}

void Panel::putBack(sim::Inventory& inventory, const sim::ItemStack& stack) {
    // Back where it came from if that slot is still free, and anywhere it fits
    // if it is not.
    if (dragFrom_ == From::Pack && inventory.pack()[dragSlot_].id == sim::ItemId::None) {
        inventory.pack()[dragSlot_] = stack;
        return;
    }
    if (dragFrom_ == From::Belt && inventory.hotbar()[dragSlot_].id == sim::ItemId::None) {
        inventory.hotbar()[dragSlot_] = stack;
        return;
    }
    if (dragFrom_ == From::Worn && inventory.worn().id == sim::ItemId::None) {
        inventory.worn() = stack;
        return;
    }
    if (dragFrom_ == From::Container && container_ &&
        dragSlot_ < static_cast<int>(container_->slots.size()) &&
        container_->slots[dragSlot_].id == sim::ItemId::None) {
        container_->slots[dragSlot_] = stack;
        return;
    }
    inventory.add(stack.id, stack.count);
}

}  // namespace client
