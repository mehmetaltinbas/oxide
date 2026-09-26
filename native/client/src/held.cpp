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

void drawHeldItem(Paint& paint, const BodyFrame& body, sim::ItemId item, const MeleePose& pose,
                  float bowDraw) {
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
            // Seen from above with the grip in the fist: the limbs sweep back
            // towards the archer, the string runs between their tips, and
            // drawing pulls it into a V with an arrow on it.
            const float tipAcross = 9.5f;
            // The tips sit behind the hand, and come back further as it draws.
            const float tipAlong = -(5.5f - bowDraw * 1.5f);
            const float nock = -(4.5f + bowDraw * 9.0f);
            const Point leftTip = tool.at(tipAlong, -tipAcross);
            const Point rightTip = tool.at(tipAlong, tipAcross);
            const Point nockAt = tool.at(nock, 0);
            paint.line(leftTip.x, leftTip.y, nockAt.x, nockAt.y, 1.0f, kInk);
            paint.line(nockAt.x, nockAt.y, rightTip.x, rightTip.y, 1.0f, kInk);
            if (bowDraw > 0) {
                // The arrow on the string, its head out ahead of the grip.
                const float head = nock + 26;
                const Point shaft = tool.at(head - 3, 0);
                paint.line(nockAt.x, nockAt.y, shaft.x, shaft.y, 1.8f, kWoodHandle);
                inked(paint, {tool.at(head + 1, 0), tool.at(head - 3, -2), tool.at(head - 3, 2)},
                      rgb(0xcfd8e0));
            }
            // The limbs: one curve from tip to tip, bowing out ahead of the
            // hand, which is what makes it a bow rather than a stick.
            std::vector<Point> limbs;
            for (int i = 0; i <= 12; ++i) {
                const float u = i / 12.0f;
                const float w0 = (1 - u) * (1 - u);
                const float w1 = 2 * u * (1 - u);
                const float w2 = u * u;
                const float along = w0 * tipAlong + w1 * (-tipAlong * 2.0f) + w2 * tipAlong;
                const float across = w0 * -tipAcross + w2 * tipAcross;
                limbs.push_back(tool.at(along, across));
            }
            paint.outlinePoly(limbs, 2.6f + kInkWidth, kInk, false);
            paint.outlinePoly(limbs, 2.6f, kWoodHandle, false);
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
        case sim::ItemId::Leather: {
            // A cut hide, darker and stiffer than cloth.
            paint.inkedPoly({{x - s * 0.6f, y - s * 0.5f}, {x + s * 0.55f, y - s * 0.6f},
                             {x + s * 0.6f, y + s * 0.45f}, {x - s * 0.5f, y + s * 0.6f}},
                            rgb(0x9c6a3c), kInkFine);
            paint.line(x - s * 0.3f, y - s * 0.2f, x + s * 0.3f, y - s * 0.1f, kInkFine * 0.8f, kInk);
            break;
        }
        case sim::ItemId::MeatRaw: {
            paint.inkedPoly({{x - s * 0.55f, y - s * 0.1f}, {x - s * 0.2f, y - s * 0.6f},
                             {x + s * 0.5f, y - s * 0.3f}, {x + s * 0.4f, y + s * 0.5f},
                             {x - s * 0.4f, y + s * 0.4f}},
                            rgb(0xb6544f), kInkFine);
            paint.fillCircle(x + s * 0.1f, y, s * 0.18f, rgb(0xe08a84));
            break;
        }
        case sim::ItemId::Bone: {
            paint.line(x - s * 0.45f, y + s * 0.3f, x + s * 0.45f, y - s * 0.3f, s * 0.3f,
                       rgb(0xe8e2d0));
            paint.fillCircle(x - s * 0.5f, y + s * 0.35f, s * 0.22f, rgb(0xe8e2d0));
            paint.fillCircle(x + s * 0.5f, y - s * 0.35f, s * 0.22f, rgb(0xe8e2d0));
            break;
        }
        case sim::ItemId::AnimalFat:
            paint.inkedCircle(x, y, s * 0.55f, rgb(0xf0e3c0), kInkFine);
            break;
        case sim::ItemId::Charcoal:
            paint.inkedPoly({{x - s * 0.5f, y - s * 0.3f}, {x, y - s * 0.6f},
                             {x + s * 0.55f, y - s * 0.1f}, {x + s * 0.2f, y + s * 0.55f},
                             {x - s * 0.45f, y + s * 0.4f}},
                            rgb(0x3a3733), kInkFine);
            break;
        case sim::ItemId::Gunpowder: {
            // A little heap of it, as it is poured.
            paint.inkedPoly({{x - s * 0.6f, y + s * 0.45f}, {x, y - s * 0.55f},
                             {x + s * 0.6f, y + s * 0.45f}},
                            rgb(0x4a4a52), kInkFine);
            break;
        }
        case sim::ItemId::Arrow: {
            paint.line(x - s * 0.5f, y + s * 0.45f, x + s * 0.4f, y - s * 0.4f, s * 0.14f,
                       rgb(0x8a5a2e));
            paint.fillPoly({{x + s * 0.3f, y - s * 0.5f}, {x + s * 0.6f, y - s * 0.6f},
                            {x + s * 0.5f, y - s * 0.25f}},
                           rgb(0xd5dae0));
            break;
        }
        case sim::ItemId::ShotgunShell: {
            paint.inkedPoly({{x - s * 0.25f, y - s * 0.6f}, {x + s * 0.25f, y - s * 0.6f},
                             {x + s * 0.25f, y + s * 0.2f}, {x - s * 0.25f, y + s * 0.2f}},
                            rgb(0xb0473a), kInkFine);
            paint.inkedPoly({{x - s * 0.25f, y + s * 0.2f}, {x + s * 0.25f, y + s * 0.2f},
                             {x + s * 0.25f, y + s * 0.6f}, {x - s * 0.25f, y + s * 0.6f}},
                            rgb(0xc9a227), kInkFine);
            break;
        }
        case sim::ItemId::RifleAmmo: {
            paint.inkedPoly({{x - s * 0.22f, y + s * 0.6f}, {x - s * 0.22f, y - s * 0.3f},
                             {x, y - s * 0.7f}, {x + s * 0.22f, y - s * 0.3f},
                             {x + s * 0.22f, y + s * 0.6f}},
                            rgb(0xb69269), kInkFine);
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
        case sim::ItemId::MeatCooked: {
            paint.inkedPoly({{x - s * 0.55f, y - s * 0.1f}, {x - s * 0.2f, y - s * 0.6f},
                             {x + s * 0.5f, y - s * 0.3f}, {x + s * 0.4f, y + s * 0.5f},
                             {x - s * 0.4f, y + s * 0.4f}},
                            rgb(0xb36425), kInkFine);
            paint.line(x - s * 0.3f, y, x + s * 0.3f, y - s * 0.1f, kInkFine, kInk);
            break;
        }
        case sim::ItemId::Water: {
            // A skin of it, which is how you carry water about.
            paint.inkedPoly({{x - s * 0.35f, y - s * 0.6f}, {x + s * 0.35f, y - s * 0.6f},
                             {x + s * 0.5f, y + s * 0.5f}, {x - s * 0.5f, y + s * 0.5f}},
                            rgb(0x66c0f7), kInkFine);
            break;
        }
        case sim::ItemId::Bandage: {
            paint.inkedPoly({{x - s * 0.6f, y - s * 0.25f}, {x + s * 0.6f, y - s * 0.25f},
                             {x + s * 0.6f, y + s * 0.25f}, {x - s * 0.6f, y + s * 0.25f}},
                            rgb(0xebebeb), kInkFine);
            paint.line(x - s * 0.2f, y - s * 0.45f, x + s * 0.2f, y + s * 0.45f, kInkFine * 1.4f,
                       rgb(0xd8483a));
            break;
        }
        case sim::ItemId::Medkit: {
            paint.inkedPoly({{x - s * 0.15f, y - s * 0.6f}, {x + s * 0.15f, y - s * 0.6f},
                             {x + s * 0.15f, y + s * 0.6f}, {x - s * 0.15f, y + s * 0.6f}},
                            rgb(0xf95177), kInkFine);
            paint.line(x - s * 0.4f, y - s * 0.2f, x + s * 0.4f, y - s * 0.2f, kInkFine,
                       rgb(0xffffff));
            break;
        }
        case sim::ItemId::Clothing:
        case sim::ItemId::Hazmat: {
            // A jerkin, seen flat: shoulders, body and two sleeves.
            const Color cloth = item == sim::ItemId::Hazmat ? rgb(0x64e6b7) : rgb(0xc87f46);
            paint.inkedPoly({{x - s * 0.35f, y - s * 0.55f}, {x + s * 0.35f, y - s * 0.55f},
                             {x + s * 0.4f, y + s * 0.6f}, {x - s * 0.4f, y + s * 0.6f}},
                            cloth, kInkFine);
            paint.inkedPoly({{x - s * 0.65f, y - s * 0.5f}, {x - s * 0.35f, y - s * 0.55f},
                             {x - s * 0.35f, y - s * 0.1f}, {x - s * 0.6f, y - s * 0.05f}},
                            cloth, kInkFine);
            paint.inkedPoly({{x + s * 0.65f, y - s * 0.5f}, {x + s * 0.35f, y - s * 0.55f},
                             {x + s * 0.35f, y - s * 0.1f}, {x + s * 0.6f, y - s * 0.05f}},
                            cloth, kInkFine);
            break;
        }
        case sim::ItemId::BuildingPlan: {
            // A rolled plan with a line of drawing on it.
            paint.inkedPoly({{x - s * 0.6f, y - s * 0.45f}, {x + s * 0.6f, y - s * 0.45f},
                             {x + s * 0.6f, y + s * 0.45f}, {x - s * 0.6f, y + s * 0.45f}},
                            rgb(0xffe095), kInkFine);
            paint.line(x - s * 0.35f, y + s * 0.2f, x - s * 0.35f, y - s * 0.2f, kInkFine, kInk);
            paint.line(x - s * 0.35f, y - s * 0.2f, x + s * 0.35f, y - s * 0.2f, kInkFine, kInk);
            paint.line(x + s * 0.35f, y - s * 0.2f, x + s * 0.35f, y + s * 0.2f, kInkFine, kInk);
            break;
        }
        case sim::ItemId::Lock: {
            paint.inkedPoly({{x - s * 0.4f, y - s * 0.1f}, {x + s * 0.4f, y - s * 0.1f},
                             {x + s * 0.4f, y + s * 0.55f}, {x - s * 0.4f, y + s * 0.55f}},
                            rgb(0xc9a227), kInkFine);
            for (int i = 0; i < 8; ++i) {
                const float a = 3.14159265f + i / 7.0f * 3.14159265f;
                const float bx = x + std::cos(a) * s * 0.28f;
                const float by = y - s * 0.1f + std::sin(a) * s * 0.28f;
                paint.fillCircle(bx, by, kInkFine, kInk);
            }
            break;
        }
        case sim::ItemId::Campfire: {
            paint.inkedCircle(x, y, s * 0.6f, rgb(0x6f6a5e), kInkFine);
            paint.fillPoly({{x - s * 0.25f, y + s * 0.2f}, {x, y - s * 0.45f},
                            {x + s * 0.25f, y + s * 0.2f}},
                           rgb(0xff8c2e));
            break;
        }
        case sim::ItemId::Furnace: {
            paint.inkedCircle(x, y, s * 0.6f, rgb(0x7e858c), kInkFine);
            paint.fillPoly({{x - s * 0.2f, y + s * 0.15f}, {x + s * 0.2f, y + s * 0.15f},
                            {x + s * 0.15f, y + s * 0.6f}, {x - s * 0.15f, y + s * 0.6f}},
                           rgb(0x2e2a22));
            break;
        }
        case sim::ItemId::WoodenBox: {
            paint.inkedPoly({{x - s * 0.6f, y - s * 0.45f}, {x + s * 0.6f, y - s * 0.45f},
                             {x + s * 0.6f, y + s * 0.5f}, {x - s * 0.6f, y + s * 0.5f}},
                            rgb(0x8a6034), kInkFine);
            paint.line(x - s * 0.6f, y, x + s * 0.6f, y, kInkFine, rgb(0x5d4022));
            break;
        }
        case sim::ItemId::ToolCupboard: {
            paint.inkedPoly({{x - s * 0.55f, y - s * 0.55f}, {x + s * 0.55f, y - s * 0.55f},
                             {x + s * 0.55f, y + s * 0.55f}, {x - s * 0.55f, y + s * 0.55f}},
                            rgb(0x6b5540), kInkFine);
            paint.inkedPoly({{x - s * 0.3f, y - s * 0.3f}, {x + s * 0.3f, y - s * 0.3f},
                             {x + s * 0.3f, y + s * 0.1f}, {x - s * 0.3f, y + s * 0.1f}},
                            rgb(0x8a7a5a), kInkFine);
            break;
        }
        case sim::ItemId::SleepingBag: {
            paint.inkedPoly({{x - s * 0.4f, y - s * 0.6f}, {x + s * 0.4f, y - s * 0.6f},
                             {x + s * 0.4f, y + s * 0.6f}, {x - s * 0.4f, y + s * 0.6f}},
                            rgb(0xa05a5a), kInkFine);
            paint.fillCircle(x, y - s * 0.3f, s * 0.2f, rgb(0xc98a8a));
            break;
        }
        case sim::ItemId::Workbench1:
        case sim::ItemId::Workbench2:
        case sim::ItemId::Workbench3: {
            paint.inkedPoly({{x - s * 0.65f, y - s * 0.2f}, {x + s * 0.65f, y - s * 0.2f},
                             {x + s * 0.65f, y + s * 0.25f}, {x - s * 0.65f, y + s * 0.25f}},
                            rgb(0x8a6034), kInkFine);
            const int tier = item == sim::ItemId::Workbench3   ? 3
                             : item == sim::ItemId::Workbench2 ? 2
                                                               : 1;
            for (int i = 0; i < tier; ++i) {
                paint.fillCircle(x - s * 0.3f + i * s * 0.3f, y + s * 0.45f, s * 0.1f,
                                 rgb(0xc9a227));
            }
            break;
        }
        case sim::ItemId::Satchel:
        case sim::ItemId::C4: {
            // A bundle with a fuse out of the top of it.
            const Color body = item == sim::ItemId::C4 ? rgb(0xd8d2c0) : rgb(0x8a7a5a);
            paint.inkedPoly({{x - s * 0.45f, y - s * 0.2f}, {x + s * 0.45f, y - s * 0.2f},
                             {x + s * 0.45f, y + s * 0.5f}, {x - s * 0.45f, y + s * 0.5f}},
                            body, kInkFine);
            paint.line(x, y - s * 0.2f, x + s * 0.2f, y - s * 0.6f, kInkFine, kInk);
            paint.fillCircle(x + s * 0.2f, y - s * 0.6f, s * 0.1f, rgb(0xff6b4a));
            break;
        }
        case sim::ItemId::Rocket: {
            paint.inkedPoly({{x - s * 0.15f, y + s * 0.5f}, {x - s * 0.15f, y - s * 0.2f},
                             {x, y - s * 0.6f}, {x + s * 0.15f, y - s * 0.2f},
                             {x + s * 0.15f, y + s * 0.5f}},
                            rgb(0x6b6f76), kInkFine);
            paint.fillPoly({{x - s * 0.15f, y + s * 0.5f}, {x - s * 0.35f, y + s * 0.6f},
                            {x - s * 0.15f, y + s * 0.25f}},
                           rgb(0xc8433a));
            paint.fillPoly({{x + s * 0.15f, y + s * 0.5f}, {x + s * 0.35f, y + s * 0.6f},
                            {x + s * 0.15f, y + s * 0.25f}},
                           rgb(0xc8433a));
            break;
        }
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
