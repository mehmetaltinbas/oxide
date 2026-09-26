# The game as you play it: a window, input, drawing and sound. Every rule it
# obeys comes from oxide_sim, so the client cannot invent one the server has
# not agreed to.
add_executable(oxide_client
    src/main.cpp
    src/paint.cpp
    src/terrain.cpp
    src/sprites.cpp
    src/human.cpp
    src/animal.cpp
    src/held.cpp
    src/melee.cpp
    src/hud.cpp
    src/panel.cpp
    src/built.cpp
    src/deploy_draw.cpp
    src/monument_draw.cpp
    src/map_screen.cpp
    src/net_client.cpp
    src/particles.cpp
    src/audio.cpp
    src/vital_icon.cpp
    src/text.cpp
)
target_link_libraries(oxide_client PRIVATE oxide_sim SDL3::SDL3 SDL3_ttf::SDL3_ttf-static enet)
set_target_properties(oxide_client PROPERTIES OUTPUT_NAME oxide)

# The lettering travels with the binary, so the game can be run from anywhere
# rather than only from the directory above the fonts.
add_custom_command(
    TARGET oxide_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/../assets/fonts
            $<TARGET_FILE_DIR:oxide_client>/assets/fonts
    COMMENT "Putting the fonts beside the game"
)
