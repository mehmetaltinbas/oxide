#include "held.hpp"

#include <cmath>
#include <vector>

#include "palette.hpp"

namespace client {

namespace {

constexpr Color kSteel = rgb(0xd5dae0);
constexpr Color kSteelDark = rgb(0x8d959e);
constexpr Color kWoodHandle = rgb(0x8a5a2e);

/** A shape given in the tool's own frame, laid into the body's. */
struct ToolFrame {
    const BodyFrame& body;
    Point grip;
    float angle;
    float stretch;

    Point at(float along, float across) const {
        // `along` runs down the handle, `across` is at right angles to it. The
        // stretch squashes the length only, which is what foreshortening is.
        const float a = along * stretch;
        const float c = std::cos(angle);
        const float s = std::sin(angle);
        return body.at(grip.x + a * s + across * c, grip.y - a * c + across * s);
    }
};

void inked(Paint& paint, const std::vector<Point>& pts, Color fill) {
    paint.fillPoly(pts, fill);
    paint.outlinePoly(pts, kInkFine, kInk);
}

/** The handle every tool hangs off, from the grip forward. */
void handle(Paint& paint, const ToolFrame& t, float from, float to, float half) {
    inked(paint, {t.at(from, -half), t.at(to, -half), t.at(to, half), t.at(from, half)}, kWoodHandle);
}

}  // namespace

MeleeStyle meleeStyleOf(sim::ItemId item) {
    switch (item) {
        case sim::ItemId::Hatchet:
        case sim::ItemId::Pickaxe:
        case sim::ItemId::Hammer: return MeleeStyle::Chop;
        case sim::ItemId::Spear:
        case sim::ItemId::Bow:
        case sim::ItemId::Revolver:
        case sim::ItemId::Waterpipe:
        case sim::ItemId::PumpShotgun:
        case sim::ItemId::Rifle:
        case sim::ItemId::Ak47: return MeleeStyle::Thrust;
        default: return MeleeStyle::Smash;
    }
}

void drawHeldItem(Paint& paint, const BodyFrame& body, sim::ItemId item, const MeleePose& pose) {
    if (item == sim::ItemId::None) return;
    // Held upright, what stands over the fist is the head, so the whole tool is
    // slid down until its head is where the hand is.
    const float shift = pose.overHand ? 9.0f * pose.stretch : 0.0f;
    const ToolFrame tool{body, {pose.hand.x, pose.hand.y + shift}, pose.angle, pose.stretch};

    switch (item) {
        case sim::ItemId::Hatchet: {
            handle(paint, tool, -3, 11, 1.1f);
            // The bit forward, the poll behind: an axe head seen from above.
            inked(paint, {tool.at(11, -1), tool.at(15, -5.5f), tool.at(16.5f, 0), tool.at(15, 5.5f)},
                  kSteel);
            inked(paint, {tool.at(9, -2), tool.at(11, -1), tool.at(11, 1), tool.at(9, 2)}, kSteelDark);
            break;
        }
        case sim::ItemId::Pickaxe: {
            handle(paint, tool, -3, 10, 1.1f);
            // Two points, one each way, the way a pick is shaped.
            inked(paint, {tool.at(10, -1.4f), tool.at(17, -6), tool.at(17.5f, -4), tool.at(11, 1.4f)},
                  kSteel);
            inked(paint, {tool.at(10, 1.4f), tool.at(16, 6), tool.at(16.5f, 4), tool.at(11, -1.4f)},
                  kSteelDark);
            break;
        }
        case sim::ItemId::Hammer: {
            handle(paint, tool, -3, 10, 1.1f);
            inked(paint, {tool.at(10, -4.5f), tool.at(15, -4.5f), tool.at(15, 4.5f), tool.at(10, 4.5f)},
                  kSteel);
            break;
        }
        case sim::ItemId::Spear: {
            handle(paint, tool, -8, 22, 1.0f);
            inked(paint, {tool.at(22, -2), tool.at(30, 0), tool.at(22, 2)}, kSteelDark);
            break;
        }
        case sim::ItemId::Bow: {
            // The limbs curve away from you and the string runs between their
            // tips, which is the whole shape of a bow from above.
            std::vector<Point> limb;
            for (int i = 0; i <= 10; ++i) {
                const float u = i / 10.0f - 0.5f;
                limb.push_back(tool.at(2 - u * u * 18, u * 26));
            }
            paint.outlinePoly(limb, 2.2f, rgb(0x8a5a2e), false);
            paint.line(tool.at(-2.5f, -13).x, tool.at(-2.5f, -13).y, tool.at(-2.5f, 13).x,
                       tool.at(-2.5f, 13).y, 1.2f, kInk);
            break;
        }
        case sim::ItemId::Revolver: {
            inked(paint, {tool.at(0, -2.4f), tool.at(9, -2.4f), tool.at(9, 2.4f), tool.at(0, 2.4f)},
                  rgb(0x6b6f76));
            inked(paint, {tool.at(-1, -3.4f), tool.at(2.5f, -3.4f), tool.at(2.5f, 3.4f),
                          tool.at(-1, 3.4f)},
                  rgb(0x3f434a));
            break;
        }
        case sim::ItemId::Waterpipe:
        case sim::ItemId::PumpShotgun:
        case sim::ItemId::Rifle:
        case sim::ItemId::Ak47: {
            // A long gun, seen from above: a stock under the hand, a receiver,
            // and the barrel running out ahead.
            const bool wood = item == sim::ItemId::Waterpipe || item == sim::ItemId::PumpShotgun ||
                              item == sim::ItemId::Ak47;
            inked(paint, {tool.at(-9, -2.2f), tool.at(2, -2.2f), tool.at(2, 2.2f), tool.at(-9, 2.2f)},
                  wood ? rgb(0x8a5a2e) : rgb(0x4d5159));
            inked(paint, {tool.at(2, -2.8f), tool.at(12, -2.8f), tool.at(12, 2.8f), tool.at(2, 2.8f)},
                  rgb(0x4d5159));
            inked(paint, {tool.at(12, -1.4f), tool.at(24, -1.4f), tool.at(24, 1.4f), tool.at(12, 1.4f)},
                  rgb(0x6b6f76));
            // The magazine, hanging under the receiver.
            if (item == sim::ItemId::Ak47 || item == sim::ItemId::Rifle) {
                inked(paint, {tool.at(3, 2.8f), tool.at(8, 2.8f), tool.at(7, 7.5f), tool.at(4, 7.5f)},
                      rgb(0x3f434a));
            }
            break;
        }
        case sim::ItemId::Rock: {
            // A lump, not a tool: no handle, and no two of them alike enough
            // to matter at this size.
            inked(paint,
                  {tool.at(0, -4), tool.at(4, -3), tool.at(5.5f, 1), tool.at(2, 4.5f),
                   tool.at(-2.5f, 3), tool.at(-3.5f, -1.5f)},
                  rgb(0x9e9e9e));
            break;
        }
        default: break;
    }
}

void drawItemIcon(Paint& paint, sim::ItemId item, float x, float y, float size) {
    const float s = size * 0.5f;
    switch (item) {
        case sim::ItemId::Wood: {
            // A short length of log, seen end on.
            paint.inkedCircle(x, y, s * 0.6f, rgb(0xb36e25), kInkFine);
            paint.fillCircle(x, y, s * 0.28f, rgb(0x8a5220));
            break;
        }
        case sim::ItemId::Stone:
            paint.inkedPoly({{x - s * 0.7f, y}, {x - s * 0.3f, y - s * 0.6f}, {x + s * 0.5f, y - s * 0.4f},
                             {x + s * 0.6f, y + s * 0.4f}, {x - s * 0.2f, y + s * 0.6f}},
                            rgb(0xb0b9c1), kInkFine);
            break;
        case sim::ItemId::MetalOre:
        case sim::ItemId::SulfurOre: {
            const Color body = item == sim::ItemId::MetalOre ? rgb(0xb69269) : rgb(0xe7dd64);
            paint.inkedPoly({{x - s * 0.6f, y - s * 0.2f}, {x - s * 0.1f, y - s * 0.6f},
                             {x + s * 0.6f, y - s * 0.1f}, {x + s * 0.3f, y + s * 0.6f},
                             {x - s * 0.4f, y + s * 0.5f}},
                            body, kInkFine);
            break;
        }
        case sim::ItemId::Metal:
        case sim::ItemId::Sulfur: {
            const Color body = item == sim::ItemId::Metal ? rgb(0xe1e9f0) : rgb(0xfff382);
            for (int i = 0; i < 3; ++i) {
                const float ox = (i - 1) * s * 0.45f;
                const float oy = (i % 2 == 0 ? -1 : 1) * s * 0.2f;
                paint.inkedPoly({{x + ox - s * 0.3f, y + oy}, {x + ox, y + oy - s * 0.35f},
                                 {x + ox + s * 0.3f, y + oy}, {x + ox, y + oy + s * 0.3f}},
                                body, kInkFine);
            }
            break;
        }
        case sim::ItemId::Cloth: {
            paint.inkedPoly({{x - s * 0.6f, y - s * 0.4f}, {x + s * 0.6f, y - s * 0.6f},
                             {x + s * 0.5f, y + s * 0.5f}, {x - s * 0.5f, y + s * 0.6f}},
                            rgb(0xd9c8a8), kInkFine);
            break;
        }
        case sim::ItemId::Scrap: {
            // Rusty, torn metal: what the roads are for.
            paint.inkedPoly({{x - s * 0.7f, y - s * 0.3f}, {x - s * 0.1f, y - s * 0.7f},
                             {x + s * 0.6f, y - s * 0.3f}, {x + s * 0.4f, y + s * 0.6f},
                             {x - s * 0.5f, y + s * 0.4f}},
                            rgb(0xa5652f), kInkFine);
            paint.line(x - s * 0.3f, y + s * 0.1f, x + s * 0.3f, y - s * 0.2f, kInkFine, kInk);
            break;
        }
        case sim::ItemId::LowGrade:
            paint.inkedCircle(x, y, s * 0.6f, rgb(0xd8a24a), kInkFine);
            break;
        case sim::ItemId::PistolAmmo:
            paint.inkedPoly({{x - s * 0.2f, y + s * 0.6f}, {x - s * 0.2f, y - s * 0.2f},
                             {x, y - s * 0.6f}, {x + s * 0.2f, y - s * 0.2f},
                             {x + s * 0.2f, y + s * 0.6f}},
                            rgb(0xc9a227), kInkFine);
            break;
        default: {
            // The tools, drawn with the same routine that puts one in a hand,
            // so a hatchet on the belt is the hatchet you swing.
            BodyFrame body{x, y, 0, 1, size / 26.0f};
            MeleePose pose;
            pose.hand = {0, 6};
            pose.angle = 0;
            pose.stretch = 1;
            pose.overHand = false;
            drawHeldItem(paint, body, item, pose);
            break;
        }
    }
}

}  // namespace client
