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
