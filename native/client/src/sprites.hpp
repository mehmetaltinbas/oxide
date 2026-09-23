#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <vector>

#include "paint.hpp"
#include "sim/node.hpp"

namespace client {

/** How a node is dressed: its own look, and whether it stands in the snow. */
struct SpriteKey {
    sim::NodeKind kind;
    int variant;
    bool snowy;
    bool broadleaf;
};

/**
 * Every tree, rock and barrel, drawn once into a texture and then stamped.
 *
 * The TypeScript game rebuilt each one path by path every frame, and two thirds
 * of a heavy frame went on it. Here each look is painted once at a generous
 * size and afterwards costs one textured quad, which is what a rewrite for
 * Steam is for. Each kind has a handful of variants picked by the node's own
 * seed, so a forest still has no two trees alike.
 */
class Sprites {
public:
    explicit Sprites(SDL_Renderer* renderer) : renderer_(renderer) {}
    ~Sprites();

    Sprites(const Sprites&) = delete;
    Sprites& operator=(const Sprites&) = delete;

    /** How many looks each kind is drawn in. */
    static constexpr int kVariants = 12;
    /** The radius everything is baked at, before it is scaled to the node. */
    static constexpr float kBakeRadius = 48.0f;

    /** Draw one node, its foot at the given screen point, at this scale. */
    void draw(const sim::ResourceNode& node, float screenX, float screenY, float scale,
              bool snowy, bool broadleaf, float alpha = 1.0f);

private:
    struct Baked {
        SDL_Texture* texture = nullptr;
        /** Where the foot of the thing sits inside the texture. */
        float originX = 0;
        float originY = 0;
        float width = 0;
        float height = 0;
    };

    SDL_Renderer* renderer_ = nullptr;
    /** kind, variant, snowy, broadleaf. */
    std::array<Baked, sim::kNodeKindCount * kVariants * 2 * 2> baked_{};

    const Baked& bake(SpriteKey key);
};

}  // namespace client
