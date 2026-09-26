#include "panel.hpp"

#include <SDL3/SDL.h>

#include <algorithm>

#include "held.hpp"
#include "palette.hpp"

namespace client {

namespace {

constexpr Color kPlate{24, 21, 17, 235};
constexpr Color kSlot{46, 41, 34, 255};
constexpr Color kRow{38, 34, 28, 255};
constexpr Color kRowReady{54, 66, 44, 255};
constexpr Color kText = rgb(0xefeadd);
constexpr Color kDim = rgb(0x8d8a80);

/** The pack is laid out six across, as the belt is. */
constexpr int kPackCols = 6;

void text(SDL_Renderer* renderer, float x, float y, float scale, Color color, const char* line) {
    SDL_SetRenderScale(renderer, scale, scale);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDebugText(renderer, x / scale, y / scale, line);
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);
}

}  // namespace

Panel::Layout Panel::layoutOf(int width, int height, float uiScale) {
    Layout out{};
    out.w = 760 * uiScale;
    out.h = 470 * uiScale;
    out.x = (width - out.w) * 0.5f;
    out.y = (height - out.h) * 0.5f - 30 * uiScale;
    out.slot = 52 * uiScale;
    out.packX = out.x + 24 * uiScale;
    out.packY = out.y + 58 * uiScale;
    out.listX = out.x + 400 * uiScale;
    out.listY = out.y + 58 * uiScale;
    out.rowH = 34 * uiScale;
    out.rowW = 336 * uiScale;
    out.queueY = out.y + out.h - 66 * uiScale;
    return out;
}

bool Panel::click(sim::Inventory& inventory, sim::Crafting& crafting, float x, float y, bool right,
                  int width, int height, float uiScale) {
    if (!open_) return false;
    const Layout l = layoutOf(width, height, uiScale);
    if (x < l.x || y < l.y || x > l.x + l.w || y > l.y + l.h) return false;

    // A job in the queue, taken back off it.
    const auto& jobs = crafting.jobs();
    for (std::size_t i = 0; i < jobs.size(); ++i) {
        const float jx = l.x + 24 * uiScale + i * (54 * uiScale);
        if (x < jx || x > jx + 48 * uiScale) continue;
        if (y < l.queueY || y > l.queueY + 48 * uiScale) continue;
        if (right) crafting.cancel(inventory, jobs[i].id);
        return true;
    }

    // A slot, moved between the pack and whatever is open.
    if (container_) {
        const auto slotHit = [&](float ox, float oy, int count, int cols) {
            for (int i = 0; i < count; ++i) {
                const float sx = ox + (i % cols) * (l.slot + 4 * uiScale);
                const float sy = oy + (i / cols) * (l.slot + 4 * uiScale);
                if (x >= sx && x <= sx + l.slot && y >= sy && y <= sy + l.slot) return i;
            }
            return -1;
        };
        const int packSlot = slotHit(l.packX, l.packY, sim::kPackSlots, kPackCols);
        const float beltY = l.packY + 4 * (l.slot + 4 * uiScale) + 22 * uiScale;
        const int beltSlot = slotHit(l.packX, beltY, sim::kHotbarSlots, sim::kHotbarSlots);
        const int inSlot =
            slotHit(l.listX, l.listY, static_cast<int>(container_->slots.size()), 4);
        // A click sends a stack the other way: into the box, or out of it.
        // The move takes a moment rather than happening on the click.
        if (move_.left > 0) return true;
        if (packSlot >= 0 || beltSlot >= 0) {
            const sim::ItemStack& stack =
                packSlot >= 0 ? inventory.pack()[packSlot] : inventory.hotbar()[beltSlot];
            if (stack.id == sim::ItemId::None) return true;
            move_ = Move{true, beltSlot >= 0, beltSlot >= 0 ? beltSlot : packSlot,
                         sim::Transfer::seconds(stack.count), sim::Transfer::seconds(stack.count)};
            return true;
        }
        if (inSlot >= 0) {
            const sim::ItemStack& stack = container_->slots[inSlot];
            if (stack.id == sim::ItemId::None) return true;
            move_ = Move{false, false, inSlot, sim::Transfer::seconds(stack.count),
                         sim::Transfer::seconds(stack.count)};
            return true;
        }
        return true;
    }

    // The sandbox shelf: a click takes one of whatever is under it.
    if (shelf_ && !container_) {
        const int cols = 6;
        for (int i = 1; i < sim::kItemCount; ++i) {
            const int at = i - 1;
            const float sx = l.listX + (at % cols) * (l.slot + 4 * uiScale);
            const float sy = l.listY + (at / cols) * (l.slot + 4 * uiScale);
            if (x < sx || x > sx + l.slot || y < sy || y > sy + l.slot) continue;
            const auto id = static_cast<sim::ItemId>(i);
            inventory.add(id, right ? sim::itemDef(id).stack : 1);
            return true;
        }
        return true;
    }

    // A recipe, queued.
    if (!right && x >= l.listX && x <= l.listX + l.rowW) {
        const int row = static_cast<int>((y - l.listY) / l.rowH);
        const auto& all = sim::recipes();
        if (row >= 0 && row < static_cast<int>(all.size())) {
            // Only what can be made by hand, for as long as there is no bench.
            crafting.queue(inventory, all[row], bench_);
        }
    }
    return true;
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

void Panel::draw(Paint& paint, const sim::Inventory& inventory, const sim::Crafting& crafting,
                 int width, int height, float uiScale) const {
    if (!open_) return;
    SDL_Renderer* renderer = paint.renderer();
    const Layout l = layoutOf(width, height, uiScale);

    // The screen behind it, darkened, so the panel is plainly in front.
    paint.fillRect(0, 0, static_cast<float>(width), static_cast<float>(height),
                   Color{0, 0, 0, 110});
    paint.fillRect(l.x - 3 * uiScale, l.y - 3 * uiScale, l.w + 6 * uiScale, l.h + 6 * uiScale, kInk);
    paint.fillRect(l.x, l.y, l.w, l.h, kPlate);

    text(renderer, l.packX, l.y + 24 * uiScale, 2.0f * uiScale, kText, "PACK");
    text(renderer, l.listX, l.y + 24 * uiScale, 2.0f * uiScale, kText,
         container_ ? title_ : (shelf_ ? "SHELF" : "CRAFT"));

    // The pack, and the belt under it.
    const auto slotAt = [&](float x, float y, const sim::ItemStack& stack) {
        paint.fillRect(x, y, l.slot, l.slot, kSlot);
        if (stack.id == sim::ItemId::None) return;
        drawItemIcon(paint, stack.id, x + l.slot * 0.5f, y + l.slot * 0.5f, l.slot * 0.7f);
        if (stack.count > 1) {
            char count[8];
            SDL_snprintf(count, sizeof(count), "%d", stack.count);
            const float size = 1.4f * uiScale;
            const float textW = static_cast<float>(SDL_strlen(count)) * 8 * size;
            text(renderer, x + l.slot - 4 * uiScale - textW, y + l.slot - 16 * uiScale, size, kText,
                 count);
        }
    };
    for (int i = 0; i < sim::kPackSlots; ++i) {
        const float sx = l.packX + (i % kPackCols) * (l.slot + 4 * uiScale);
        const float sy = l.packY + (i / kPackCols) * (l.slot + 4 * uiScale);
        slotAt(sx, sy, inventory.pack()[i]);
    }
    const float beltY = l.packY + 4 * (l.slot + 4 * uiScale) + 22 * uiScale;
    text(renderer, l.packX, beltY - 16 * uiScale, 1.4f * uiScale, kDim, "BELT");
    for (int i = 0; i < sim::kHotbarSlots; ++i) {
        const float sx = l.packX + i * (l.slot + 4 * uiScale);
        slotAt(sx, beltY, inventory.hotbar()[i]);
    }

    if (container_) {
        // What is inside it, four across, and what it is doing if it is lit.
        const auto& slots = container_->slots;
        for (std::size_t i = 0; i < slots.size(); ++i) {
            const float sx = l.listX + (i % 4) * (l.slot + 4 * uiScale);
            const float sy = l.listY + (i / 4) * (l.slot + 4 * uiScale);
            slotAt(sx, sy, slots[i]);
        }
        if (fire_) {
            const float statusY = l.listY + 3 * (l.slot + 4 * uiScale);
            char line[96];
            SDL_snprintf(line, sizeof(line), "%s   %s", fire_->lit ? "Lit" : "Out",
                         fire_->lit ? "burning wood" : "E to light it");
            text(renderer, l.listX, statusY, 1.4f * uiScale, kDim, line);
        }
        if (move_.left > 0 && move_.total > 0) {
            // The spinner: how much longer the lifting takes.
            const float w = 160 * uiScale;
            const float bx = l.listX;
            const float by = l.listY + 3 * (l.slot + 4 * uiScale) + 24 * uiScale;
            paint.fillRect(bx - 2, by - 2, w + 4, 10 * uiScale + 4, kInk);
            paint.fillRect(bx, by, static_cast<float>(w * (1 - move_.left / move_.total)),
                           10 * uiScale, rgb(0xe8c87a));
        }
        text(renderer, l.listX, l.y + l.h - 22 * uiScale, 1.4f * uiScale, kDim,
             "click a slot to move it   TAB to close");
        return;
    }

    if (shelf_) {
        // Every item there is, six across, a click for one and a right click
        // for a stack.
        const int cols = 6;
        for (int i = 1; i < sim::kItemCount; ++i) {
            const int at = i - 1;
            const float sx = l.listX + (at % cols) * (l.slot + 4 * uiScale);
            const float sy = l.listY + (at / cols) * (l.slot + 4 * uiScale);
            if (sy + l.slot > l.y + l.h - 40 * uiScale) break;
            paint.fillRect(sx, sy, l.slot, l.slot, kSlot);
            drawItemIcon(paint, static_cast<sim::ItemId>(i), sx + l.slot * 0.5f,
                         sy + l.slot * 0.5f, l.slot * 0.7f);
        }
        text(renderer, l.listX, l.y + l.h - 22 * uiScale, 1.4f * uiScale, kDim,
             "click for one, right click for a stack");
        return;
    }

    // What can be made, with what it costs beside it.
    const auto& all = sim::recipes();
    for (std::size_t i = 0; i < all.size(); ++i) {
        const sim::Recipe& recipe = all[i];
        const float ry = l.listY + i * l.rowH;
        if (ry + l.rowH > l.queueY - 10 * uiScale) break;
        const bool byHand = recipe.bench <= bench_;
        const bool ready = byHand && sim::canAfford(inventory, recipe);
        paint.fillRect(l.listX, ry, l.rowW, l.rowH - 3 * uiScale, ready ? kRowReady : kRow);
        drawItemIcon(paint, recipe.out, l.listX + 18 * uiScale, ry + l.rowH * 0.45f,
                     22 * uiScale);

        char line[96];
        if (!byHand) {
            SDL_snprintf(line, sizeof(line), "%s   (needs a level %d bench)",
                         sim::itemDef(recipe.out).name, recipe.bench);
        } else {
            // What it costs, in the order the recipe lists it.
            char cost[64] = {0};
            for (int c = 0; c < recipe.costCount; ++c) {
                char part[32];
                SDL_snprintf(part, sizeof(part), "%s%d %s", c ? ", " : "", recipe.cost[c].count,
                             sim::itemDef(recipe.cost[c].id).name);
                SDL_strlcat(cost, part, sizeof(cost));
            }
            SDL_snprintf(line, sizeof(line), "%s   %s", sim::itemDef(recipe.out).name, cost);
        }
        text(renderer, l.listX + 36 * uiScale, ry + 10 * uiScale, 1.4f * uiScale,
             byHand ? kText : kDim, line);
    }

    // The queue, with the one being made running down.
    const auto& jobs = crafting.jobs();
    for (std::size_t i = 0; i < jobs.size(); ++i) {
        const float jx = l.packX + i * (54 * uiScale);
        const float size = 48 * uiScale;
        paint.fillRect(jx, l.queueY, size, size, kSlot);
        drawItemIcon(paint, jobs[i].recipe->out, jx + size * 0.5f, l.queueY + size * 0.5f,
                     size * 0.7f);
        const double progress = 1 - jobs[i].left / jobs[i].recipe->seconds;
        paint.fillRect(jx, l.queueY + size - 5 * uiScale, static_cast<float>(size * progress),
                       4 * uiScale, rgb(0xe8c87a));
    }
    text(renderer, l.packX, l.queueY - 14 * uiScale, 1.4f * uiScale, kDim,
         "QUEUE   right click to cancel");
    // What the cursor is over, named along the bottom.
    float mouseX = 0;
    float mouseY = 0;
    SDL_GetMouseState(&mouseX, &mouseY);
    const float px = mouseX * uiScale;
    const float py = mouseY * uiScale;
    const auto overSlot = [&](float ox, float oy, int count, int cols, auto&& stackAt) {
        for (int i = 0; i < count; ++i) {
            const float sx = ox + (i % cols) * (l.slot + 4 * uiScale);
            const float sy = oy + (i / cols) * (l.slot + 4 * uiScale);
            if (px < sx || px > sx + l.slot || py < sy || py > sy + l.slot) continue;
            return stackAt(i).id;
        }
        return sim::ItemId::None;
    };
    sim::ItemId over = overSlot(l.packX, l.packY, sim::kPackSlots, kPackCols,
                                [&](int i) { return inventory.pack()[i]; });
    if (over == sim::ItemId::None) {
        over = overSlot(l.packX, beltY, sim::kHotbarSlots, sim::kHotbarSlots,
                        [&](int i) { return inventory.hotbar()[i]; });
    }
    if (over != sim::ItemId::None) {
        const sim::ItemDef& def = sim::itemDef(over);
        // Its name and what it is for, under the pack where there is room.
        const float detailY = l.y + l.h - 76 * uiScale;
        drawItemIcon(paint, over, l.packX + 14 * uiScale, detailY + 14 * uiScale, 26 * uiScale);
        text(renderer, l.packX + 34 * uiScale, detailY + 6 * uiScale, 1.7f * uiScale, kText,
             def.name);
        text(renderer, l.packX + 34 * uiScale, detailY + 24 * uiScale, 1.2f * uiScale, kDim,
             def.desc);
        char line[96];
        if (def.gun.damage > 0) {
            SDL_snprintf(line, sizeof(line), "%s   %.0f damage, %d in the magazine", def.name,
                         def.gun.damage, def.gun.magazine);
        } else if (def.melee.damage > 0) {
            SDL_snprintf(line, sizeof(line), "%s   %.0f damage, reaches %.0f", def.name,
                         def.melee.damage, def.melee.reach);
        } else if (def.wear.warmth > 0) {
            SDL_snprintf(line, sizeof(line), "%s   %.0f warmth, turns %.0f%% of a blow", def.name,
                         def.wear.warmth, def.wear.armor * 100);
        } else if (def.boom.damage > 0) {
            SDL_snprintf(line, sizeof(line), "%s   %.0f damage, %.1fs fuse", def.name,
                         def.boom.damage, def.boom.fuse);
        } else {
            SDL_snprintf(line, sizeof(line), "%s   stacks to %d", def.name, def.stack);
        }
        text(renderer, l.packX + 34 * uiScale, l.y + l.h - 22 * uiScale, 1.3f * uiScale, kText,
             line);
    }
    text(renderer, l.listX, l.y + l.h - 22 * uiScale, 1.4f * uiScale, kDim, "TAB to close");
}

}  // namespace client
