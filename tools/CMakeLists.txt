# Small programs for checking the rules, not part of the game. One file each:
# a check is a whole program, so it is its own artifact already.
add_executable(island_check island_check.cpp)
target_link_libraries(island_check PRIVATE oxide_sim)

add_executable(play_check play_check.cpp)
target_link_libraries(play_check PRIVATE oxide_sim)

add_executable(net_check net_check.cpp)
target_link_libraries(net_check PRIVATE oxide_sim enet)

# The sounds, checked without a sound card. It reaches into the client for the
# one file that makes them, which is the only place a tool does that.
add_executable(sound_check sound_check.cpp ../client/features/audio/systems/audio.cpp)
target_link_libraries(sound_check PRIVATE oxide_sim SDL3::SDL3)
