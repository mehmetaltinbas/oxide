# Small programs for checking the rules, not part of the game.
add_executable(island_check island_check.cpp)
target_link_libraries(island_check PRIVATE oxide_sim)

add_executable(play_check play_check.cpp)
target_link_libraries(play_check PRIVATE oxide_sim)

add_executable(net_check net_check.cpp)
target_link_libraries(net_check PRIVATE oxide_sim enet)
