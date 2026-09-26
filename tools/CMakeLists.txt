# Small programs for checking the rules, not part of the game.
add_executable(island_check island_check.cpp)
target_link_libraries(island_check PRIVATE oxide_sim)

add_executable(play_check play_check.cpp)
target_link_libraries(play_check PRIVATE oxide_sim)

add_executable(net_check net_check.cpp)
target_link_libraries(net_check PRIVATE oxide_sim enet)

# The sounds, checked without a sound card.
add_executable(sound_check sound_check.cpp ../client/src/audio.cpp)
target_include_directories(sound_check PRIVATE ../client/src)
target_link_libraries(sound_check PRIVATE oxide_sim SDL3::SDL3)
