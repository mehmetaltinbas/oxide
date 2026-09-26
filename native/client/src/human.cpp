#include "human.hpp"

#include <algorithm>
#include <cmath>

#include "held.hpp"
#include "palette.hpp"

namespace client {

namespace {

/** The right shoulder, where every swing pivots. */
constexpr Point kShoulder{10, -1};

Color shade(Color c, float amount = 0.72f) {
    return Color{static_cast<std::uint8_t>(c.r * amount), static_cast<std::uint8_t>(c.g * amount),
                 static_cast<std::uint8_t>(c.b * amount), c.a};
}

/** A limb: a length of arm or leg, as thick at one end as the other. */
void limb(Paint& paint, const BodyFrame& f, float x0, float y0, float x1, float y1, float width,
          Color color) {
    const Point a = f.at(x0, y0);
    const Point b = f.at(x1, y1);
    const float w = width * f.scale;
    paint.line(a.x, a.y, b.x, b.y, w + kInkWidth, kInk);
    paint.line(a.x, a.y, b.x, b.y, w, color);
}

void blob(Paint& paint, const BodyFrame& f, float x, float y, float rx, float ry, Color color,
          bool inked) {
    // An oval, as a ring of points in the body's own frame.
    std::vector<Point> pts;
    for (int i = 0; i < 18; ++i) {
        const float a = static_cast<float>(i) / 18 * 6.28318530718f;
        pts.push_back(f.at(x + std::cos(a) * rx, y + std::sin(a) * ry));
    }
    if (inked) {
        // The line is laid down as a slightly larger shape behind the fill
        // rather than stroked over it: at this size a stroke on top eats the
        // whole feature and a hand comes out as a black dot.
        std::vector<Point> edge;
        const float grow = (kInkWidth * 0.5f) / std::max(rx, ry);
        for (int i = 0; i < 18; ++i) {
            const float a = static_cast<float>(i) / 18 * 6.28318530718f;
            edge.push_back(f.at(x + std::cos(a) * rx * (1 + grow), y + std::sin(a) * ry * (1 + grow)));
        }
        paint.fillPoly(edge, kInk);
    }
    paint.fillPoly(pts, color);
}

}  // namespace

void drawHuman(Paint& paint, const HumanLook& look) {
    // The body is drawn at a radius of 13 and scaled from there, as the other
    // game did, so every measurement below is in the same units it used.
    const BodyFrame stance{look.x, look.y, std::cos(look.facing + 1.57079632679f),
                           std::sin(look.facing + 1.57079632679f), look.radius / 13.0f};
    // A blow comes from the hips: the whole upper body turns with it, and the
    // arms and the head are drawn in that turned frame.
    const MeleeStyle style = meleeStyleOf(look.held);
    const MeleePose pose = meleeMotion(style, look.swingT, look.phase);
    const BodyFrame f = look.swimming ? stance : stance.turned(pose.twist);
    const float step = std::sin(look.phase) * 4 * look.stride;
    // Hurt, every part of them takes the same colour.
    const auto tint = [&](Color c) { return look.hurt.a > 0 ? look.hurt : c; };
    const Color skin = tint(look.skin);
    const Color hair = tint(look.hair);
    const Color shirt = tint(look.shirt);
    const Color legs = tint(look.legs);

    if (look.swimming) {
        // Expanding rings behind a swimmer, timed to the stroke.
        for (int i = 0; i < 3; ++i) {
            const float phase = std::fmod(look.phase * 0.5f + i / 3.0f, 1.0f);
            const float rr = (12 + phase * 22) * look.radius / 13.0f;
            std::vector<Point> ring;
            for (int k = 0; k < 20; ++k) {
                const float a = static_cast<float>(k) / 20 * 6.28318530718f;
                ring.push_back({look.x + std::cos(a) * rr, look.y + std::sin(a) * rr * 0.45f});
            }
            paint.outlinePoly(ring, 2 * look.radius / 13.0f,
                              Color{190, 225, 245,
                                    static_cast<std::uint8_t>(87 * (1 - phase))});
        }
    }

    // Feet, under everything, one ahead of the other.
    if (!look.swimming) {
        blob(paint, stance, -4.6f, -step + 1, 3, 4.6f, shade(legs), true);
        blob(paint, stance, 4.6f, step + 1, 3, 4.6f, shade(legs), true);
    }

    // Arms, before the torso, so the shoulder sits over the top of the arm. A
    // swimmer reaches forward in turn instead of swinging against the stride.
    const auto stroke = [&](float side, float phase) {
        return Point{side * (9 - std::sin(phase) * 4.5f), -6 - std::cos(phase) * 10};
    };
    Point hands[2];
    if (look.swimming) {
        hands[0] = stroke(-1, look.phase);
        hands[1] = stroke(1, look.phase + 3.14159265f);
    } else {
        hands[0] = {-12.5f, -2 + step * 0.9f};
        // The right hand holds whatever is being carried or swung, so it goes
        // where the swing puts it rather than where the stride would.
        hands[1] = look.held != sim::ItemId::None ? pose.hand
                                                  : Point{12.5f, -2 - step * 0.9f};
        if (look.held == sim::ItemId::Bow) {
            // A bow is held differently from everything else: the bow arm goes
            // straight out ahead of the face and the other hand is at the nock,
            // coming back as the string is drawn.
            hands[1] = {1.5f, -17};
            hands[0] = {1.5f, -17 + 4.5f + look.bowDraw * 9.0f};
        }
    }
    const Point shoulders[2] = {{-10, -1}, {10, -1}};
    const Color sleeve = shade(shirt);
    for (int i = 0; i < 2; ++i) {
        limb(paint, f, shoulders[i].x, shoulders[i].y, hands[i].x, hands[i].y, 5.2f, sleeve);
        // The item goes under the fist, so the hand closes over its grip.
        // Unless it is carried upright: then what is over the fist is the head
        // of the tool, and the hand is behind it.
        const bool holding = i == 1 && look.held != sim::ItemId::None && !look.swimming;
        if (holding && look.held != sim::ItemId::Bow && !pose.overHand) {
            drawHeldItem(paint, f, look.held, pose, look.bowDraw);
        }
        blob(paint, f, hands[i].x, hands[i].y, 2.9f, 2.9f, skin, true);
        if (holding && look.held != sim::ItemId::Bow && pose.overHand) {
            drawHeldItem(paint, f, look.held, pose, look.bowDraw);
        }
    }

    if (look.swingT >= 0 && look.swingT < 0.34f && look.held != sim::ItemId::None &&
        !look.swimming && meleeStyleOf(look.held) == MeleeStyle::Chop) {
        // Motion lines behind the head while it is still travelling.
        const float sweep = 1 - look.swingT / 0.34f;
        std::vector<Point> arc;
        for (int i = 0; i <= 10; ++i) {
            const float u = i / 10.0f;
            const float a = 1.6f + (pose.angle - 1.6f) * u;
            const float reach = 12.5f + 20.0f;
            arc.push_back(f.at(kShoulder.x + std::sin(a) * reach,
                               kShoulder.y - std::cos(a) * reach));
        }
        paint.outlinePoly(arc, 1.6f * f.scale,
                          Color{20, 17, 13, static_cast<std::uint8_t>(120 * sweep)}, false);
    }

    if (look.held == sim::ItemId::Bow && !look.swimming) {
        // Over both hands rather than under one: a bow is held out in front of
        // you, and the arrow on it has to be seen.
        MeleePose bowPose;
        bowPose.hand = hands[1];
        bowPose.angle = 0;
        bowPose.stretch = 1;
        bowPose.overHand = false;
        drawHeldItem(paint, f, look.held, bowPose, look.bowDraw);
    }

    // The shoulders and chest: wider than deep, rounded at the ends.
    const float halfW = look.swimming ? 9.5f : 11.5f;
    const float halfD = look.swimming ? 5.0f : 6.2f;
    std::vector<Point> chest;
    for (int i = 0; i < 24; ++i) {
        const float a = static_cast<float>(i) / 24 * 6.28318530718f;
        // A rounded bar: the ends are circles, the middle is straight.
        const float x = std::cos(a) * (halfW - halfD) + std::cos(a) * halfD;
        const float y = std::sin(a) * halfD + 1;
        chest.push_back(f.at(x, y));
    }
    std::vector<Point> chestEdge;
    for (int i = 0; i < 24; ++i) {
        const float a = static_cast<float>(i) / 24 * 6.28318530718f;
        const float x = std::cos(a) * halfW * 1.06f;
        const float y = std::sin(a) * halfD * 1.12f + 1;
        chestEdge.push_back(f.at(x, y));
    }
    paint.fillPoly(chestEdge, kInk);
    paint.fillPoly(chest, shirt);

    // The head: ears first, so the skull covers their inner half, then the
    // nose, which is the one thing that says which way a face is turned when
    // all you can see is the top of a head.
    const float hy = -1.2f;
    blob(paint, f, -6.6f, hy - 0.4f, 1.6f, 2.3f, skin, true);
    blob(paint, f, 6.6f, hy - 0.4f, 1.6f, 2.3f, skin, true);
    blob(paint, f, 0, hy, 6.8f, 6.8f, skin, true);
    blob(paint, f, 0, hy - 7, 1.5f, 1.9f, skin, true);
    blob(paint, f, 0, hy + 1.7f, 6.6f, 6.6f, hair, true);
}

}  // namespace client
