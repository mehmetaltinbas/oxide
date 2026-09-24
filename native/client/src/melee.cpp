#include "melee.hpp"

#include <cmath>

namespace client {

namespace {

/** The right shoulder, where every swing pivots, and the arm off it. */
constexpr float kShoulderX = 10;
constexpr float kShoulderY = -1;
constexpr float kArm = 12.5f;

/** One key pose: the arm's angle from straight ahead, the tool's, and so on. */
struct Key {
    float arm;
    float tool;
    float stretch;
    float twist;
    /** A hand worked out from the arm unless one is given here. */
    bool hasHand;
    float handX;
    float handY;
};

Point handOf(const Key& k) {
    if (k.hasHand) return {k.handX, k.handY};
    return {kShoulderX + std::sin(k.arm) * kArm, kShoulderY - std::cos(k.arm) * kArm};
}

Key mix(const Key& a, const Key& b, float u) {
    const auto l = [u](float x, float y) { return x + (y - x) * u; };
    const Point ha = handOf(a);
    const Point hb = handOf(b);
    return Key{l(a.arm, b.arm),   l(a.tool, b.tool),   l(a.stretch, b.stretch),
               l(a.twist, b.twist), true,              l(ha.x, hb.x),
               l(ha.y, hb.y)};
}

MeleePose pose(const Key& k) {
    MeleePose out;
    out.hand = handOf(k);
    out.angle = k.tool;
    out.stretch = k.stretch;
    out.twist = k.twist;
    // Carried upright, what you see over the fist is the head; mid-swing the
    // tool lies flat and hangs off the grip as it should.
    out.overHand = k.stretch < 0.9f;
    return out;
}

float easeOut(float u) { return 1 - (1 - u) * (1 - u) * (1 - u); }
float easeInOut(float u) { return u * u * (3 - 2 * u); }

/**
 * How each is carried between blows: the hand low at the right hip, the head
 * of the tool out ahead and a little to the side, not pointed at the sky.
 */
constexpr Key kCarry[3] = {
    // Chop: upright at the hip, handle up out of the page, the head turned so
    // its two ends point ahead of you and behind you, as you carry an axe.
    {0.75f, 1.5707963f, 0.55f, 0, true, 12, -6},
    {0.4f, 0, 1, 0, true, 9.5f, -9},
    // Thrust: a spear or a gun, held out ahead of you and pointed where you
    // are looking.
    {0.3f, 0, 1, 0, true, 7.5f, -12},
};

/** Where each blow starts and where it lands. */
constexpr Key kWindup[3] = {
    // Drawn back flat, out to the right, head pointing back past the hip.
    {1.6f, 1.9f, 1, 0.3f, false, 0, 0},
    // Cocked back beside the ear.
    {2.2f, 0, 1, 0.2f, true, 11, 2},
    {0.3f, 0, 1, 0, true, 7.5f, -12},
};
constexpr Key kStrike[3] = {
    // Landing side-on: hand in front, the tool lying across the front with its
    // head to the left, into the side of whatever is there.
    {0.15f, -1.4f, 1, -0.25f, true, 6, -12},
    // Driven forward into what is in front, across to the centre line.
    {-0.1f, 0, 1, -0.35f, true, 3.5f, -17},
    // A spear goes straight out and straight back.
    {0.3f, 0, 1, 0, true, 4.5f, -34},
};

}  // namespace

MeleePose meleeMotion(MeleeStyle style, float t, float walkPhase) {
    const int s = static_cast<int>(style);
    if (t < 0) {
        Key carry = kCarry[s];
        // The carried hand rides with the stride.
        carry.handY += std::sin(walkPhase) * 0.8f;
        return pose(carry);
    }
    // The strike takes the first third and eases out, the way a swing is
    // fastest at the start and brakes through the follow-through.
    constexpr float kStrikeEnd = 0.34f;
    if (t < kStrikeEnd) {
        return pose(mix(kWindup[s], kStrike[s], easeOut(t / kStrikeEnd)));
    }
    return pose(mix(kStrike[s], kCarry[s], easeInOut((t - kStrikeEnd) / (1 - kStrikeEnd))));
}

}  // namespace client
