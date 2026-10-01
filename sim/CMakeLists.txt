# The rules of the game, shared by every binary. No SDL, no sockets, no
# platform: anything that needs those belongs in client/ or server/.
#
# The list below is grouped by feature, and the grouping is the architecture:
# one folder per feature, everything that feature owns inside it, and shared/
# for the few things no single feature owns. See
# docs/architecture/project-structure.md.
add_library(oxide_sim STATIC
    # shared: owned by no feature, used by several
    shared/utils/rng.util.cpp
    shared/utils/collide.util.cpp
    shared/constants/world-size.constant.cpp

    # world: the island, its resource nodes and what falls on the ground
    features/world/constants/node-defs.constant.cpp
    features/world/systems/world.cpp
    features/world/systems/world-nodes.cpp
    features/world/systems/world-life.cpp

    # monuments: the hand-placed landmarks and their loot
    features/monuments/constants/monument-defs.constant.cpp
    features/monuments/constants/monument-rooms.constant.cpp
    features/monuments/systems/monument-placement.cpp
    features/monuments/utils/raise-monuments.util.cpp

    # items: what a thing is, and the bags it sits in
    features/items/constants/item-defs.constant.cpp
    features/items/systems/inventory.cpp

    # crafting: recipes and the queue that works through them
    features/crafting/systems/crafting.cpp

    # survival: the player's body, its meters and how it moves
    features/survival/utils/step-player.util.cpp
    features/survival/systems/survival.cpp

    # combat: swinging, shooting and blowing things up
    features/combat/systems/action.cpp
    features/combat/systems/projectiles.cpp
    features/combat/systems/explosives.cpp

    # wildlife: the animals and the food chain between them
    features/wildlife/constants/npc-defs.constant.cpp
    features/wildlife/systems/npc-system.cpp

    # building: walls, doors and the things you put down
    features/building/systems/build-system.cpp
    features/building/constants/deployable-defs.constant.cpp
    features/building/constants/deploy-defs.constant.cpp
    features/building/constants/deploy-footprint.constant.cpp

    # session: writing a world to disk and reading it back
    features/session/systems/save.cpp
)
# Every include is absolute from the repository root, so a file's include line
# says where that file lives. No relative "../.." paths, no per-feature include
# directories to keep in step.
target_include_directories(oxide_sim PUBLIC ${CMAKE_SOURCE_DIR})
