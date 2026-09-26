#include "animal.hpp"

#include <cmath>

#include "body_frame.hpp"
#include "palette.hpp"
#include "text.hpp"

namespace client {

namespace {

Color coatOf(sim::NpcKind kind) {
    switch (kind) {
        case sim::NpcKind::Boar: return rgb(0x966e50);
        case sim::NpcKind::Wolf: return rgb(0x8e97a5);
        case sim::NpcKind::Bear: return rgb(0x88522c);
        // The people of the monuments: a lab coat and a field green.
        case sim::NpcKind::Scientist: return rgb(0xe6ebf0);
        case sim::NpcKind::Soldier: return rgb(0x6f7a52);
    }
    return rgb(0x966e50);
}

Color darkOf(sim::NpcKind kind) {
    switch (kind) {
        case sim::NpcKind::Boar: return rgb(0x573d27);
        case sim::NpcKind::Wolf: return rgb(0x505d6d);
        case sim::NpcKind::Bear: return rgb(0x523119);
        case sim::NpcKind::Scientist: return rgb(0x758494);
        case sim::NpcKind::Soldier: return rgb(0x454d31);
    }
    return rgb(0x573d27);
}

void oval(Paint& paint, const BodyFrame& f, float x, float y, float rx, float ry, Color color,
          bool inked) {
    std::vector<Point> pts;
    for (int i = 0; i < 20; ++i) {
        const float a = static_cast<float>(i) / 20 * 6.28318530718f;
        pts.push_back(f.at(x + std::cos(a) * rx, y + std::sin(a) * ry));
    }
    if (inked) {
        paint.inkedPoly(pts, color, kInkWidth);
    } else {
        paint.fillPoly(pts, color);
    }
}

}  // namespace

void drawAnimal(Paint& paint, const sim::Npc& npc, float x, float y, float scale) {
    const sim::NpcDef& def = sim::npcDef(npc.kind);
    const float r = static_cast<float>(def.radius) * scale;
    // The animal is laid out nose-forward along positive x, and turned by the
    // frame, so nothing below has to know which way it is facing.
    const BodyFrame f{x, y, std::cos(static_cast<float>(npc.facing)),
                      std::sin(static_cast<float>(npc.facing)), 1.0f};

    const bool hurt = npc.flash > 0;
    const Color coat = hurt ? rgb(0xffdede) : coatOf(npc.kind);
    const Color dark = hurt ? rgb(0xffb0b0) : darkOf(npc.kind);

    // Legs, under the body, front and back stepping against each other.
    const float gait = std::sin(static_cast<float>(npc.animPhase)) * 2.5f * scale;
    for (int side = -1; side <= 1; side += 2) {
        const float s = static_cast<float>(side);
        oval(paint, f, -r * 0.45f + gait * s * 0.3f, s * r * 0.6f, r * 0.26f, r * 0.16f, dark, true);
        oval(paint, f, r * 0.55f - gait * s * 0.3f, s * r * 0.6f, r * 0.26f, r * 0.16f, dark, true);
    }

    // Body, the shadowed line down its back, then the head.
    oval(paint, f, 0, 0, r * 1.2f, r * 0.85f, coat, true);
    oval(paint, f, -r * 0.2f, 0, r * 0.75f, r * 0.45f, dark, false);
    oval(paint, f, r * 1.0f, 0, r * 0.55f, r * 0.5f, coat, true);

    // A boar's tusks, and the ears of the things that hunt.
    if (npc.kind == sim::NpcKind::Boar) {
        for (int side = -1; side <= 1; side += 2) {
            const float s = static_cast<float>(side);
            paint.fillPoly({f.at(r * 1.35f, s * r * 0.3f), f.at(r * 1.7f, s * r * 0.12f),
                            f.at(r * 1.38f, s * r * 0.12f)},
                           rgb(0xeae2cf));
        }
    } else {
        for (int side = -1; side <= 1; side += 2) {
            const float s = static_cast<float>(side);
            oval(paint, f, r * 0.85f, s * r * 0.45f, r * 0.2f, r * 0.22f, dark, true);
        }
    }

    // Eyes: the one thing that says which end is which at a glance.
    const Color eye = npc.kind == sim::NpcKind::Bear ? rgb(0xff5c3a) : rgb(0xffd24a);
    for (int side = -1; side <= 1; side += 2) {
        const float s = static_cast<float>(side);
        oval(paint, f, r * 1.2f, s * r * 0.22f, r * 0.1f, r * 0.1f, eye, false);
    }

    // A tail, which is most of what tells a wolf from a boar from behind.
    paint.line(f.at(-r * 1.15f, 0).x, f.at(-r * 1.15f, 0).y,
               f.at(-r * (npc.kind == sim::NpcKind::Wolf ? 1.75f : 1.4f), gait * 0.4f).x,
               f.at(-r * (npc.kind == sim::NpcKind::Wolf ? 1.75f : 1.4f), gait * 0.4f).y,
               r * 0.18f, dark);
}

void drawAnimalTag(Paint& paint, const sim::Npc& npc, float x, float y, float scale,
                   float uiScale) {
    const sim::NpcDef& def = sim::npcDef(npc.kind);
    const float r = static_cast<float>(def.radius) * scale;
    const bool hurt = npc.hp < def.hp;
    if (hurt) {
        // White in black, so it reads on snow and on grass alike.
        const float w = r * 2.4f;
        const float h = 5 * uiScale;
        const float bx = x - w / 2;
        const float by = y - r - 14 * uiScale;
        paint.fillRect(bx - 2, by - 2, w + 4, h + 4, kInk);
        paint.fillRect(bx, by, w * npc.hp / def.hp, h, rgb(0xffffff));
    }
    if (Text* lettering = paint.text()) {
        lettering->draw(def.name, x, y - r - (hurt ? 32 : 22) * uiScale, 13 * uiScale,
                        rgb(0xe8b0a0), Face::Body, Align::Centre);
    }
}

}  // namespace client
