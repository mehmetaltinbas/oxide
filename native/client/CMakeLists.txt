# The game as you play it: a window, input, drawing and sound. Every rule it
# obeys comes from oxide_sim, so the client cannot invent one the server has
# not agreed to.
add_executable(oxide_client
    src/main.cpp
    src/paint.cpp
    src/terrain.cpp
    src/sprites.cpp
    src/human.cpp
    src/held.cpp
    src/melee.cpp
    src/hud.cpp
)
target_link_libraries(oxide_client PRIVATE oxide_sim SDL3::SDL3)
set_target_properties(oxide_client PROPERTIES OUTPUT_NAME oxide)
