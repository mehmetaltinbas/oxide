# The rules of the game, shared by both binaries. No SDL, no sockets, no
# platform: anything that needs those belongs in client/ or server/.
add_library(oxide_sim STATIC
    src/rng.cpp
    src/world_size.cpp
    src/world.cpp
    src/node.cpp
    src/nodes.cpp
    src/player.cpp
)
target_include_directories(oxide_sim PUBLIC include)
