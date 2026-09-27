# The game as you play it: a window, input, drawing and sound. Every rule it
# obeys comes from oxide_sim, so the client cannot invent one the server has
# not agreed to.
#
# Grouped by feature, like sim/. design/ holds the tokens every feature draws
# with; app/ holds the loop that owns them all.
add_executable(oxide_client
    # app: the window, the input and the frame
    app/main.cpp

    # render: the pen, the lettering, the ground, the night and the sparks
    features/render/systems/paint.cpp
    features/render/utils/shape-points.util.cpp
    features/render/systems/text.cpp
    features/render/systems/terrain.cpp
    features/render/systems/sprites.cpp
    features/render/systems/night.cpp
    features/render/systems/particles.cpp

    # creatures: drawing anything that walks
    features/creatures/draw/human.cpp
    features/creatures/draw/animal.cpp

    # items: the thing in the hand and the picture of it
    features/items/draw/item-glyph.cpp
    features/items/utils/melee-motion.util.cpp

    # building: drawing what has been built, put down or was always there
    features/building/draw/structure.cpp
    features/building/draw/build-plan-options.util.cpp
    features/building/draw/tier-options.util.cpp
    features/building/draw/deployable.cpp
    features/building/draw/monument.cpp

    # ui: the interface drawn over the world
    features/ui/systems/hud.cpp
    features/ui/systems/wheel.cpp
    features/ui/systems/panel.cpp
    features/ui/systems/map-screen.cpp
    features/ui/draw/vital-icon.cpp

    # audio and net
    features/audio/systems/audio.cpp
    features/net/systems/net-client.cpp
)
target_link_libraries(oxide_client PRIVATE oxide_sim SDL3::SDL3 SDL3_ttf::SDL3_ttf-static enet)
set_target_properties(oxide_client PROPERTIES OUTPUT_NAME oxide)

# The lettering travels with the binary, so the game can be run from anywhere
# rather than only from the directory above the fonts.
add_custom_command(
    TARGET oxide_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets/fonts
            $<TARGET_FILE_DIR:oxide_client>/assets/fonts
    COMMENT "Putting the fonts beside the game"
)
