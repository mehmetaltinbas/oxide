# The rules of the game, shared by both binaries. No SDL, no sockets, no
# platform: anything that needs those belongs in client/ or server/.
add_library(oxide_sim STATIC
    src/rng.cpp
    src/world_size.cpp
    src/world.cpp
    src/node.cpp
    src/nodes.cpp
    src/player.cpp
    src/item.cpp
    src/inventory.cpp
    src/action.cpp
    src/npc.cpp
    src/npcs.cpp
    src/projectile.cpp
    src/craft.cpp
    src/build.cpp
    src/survival.cpp
    src/deployable.cpp
    src/world_life.cpp
)
target_include_directories(oxide_sim PUBLIC include)
