# Small programs for checking the rules, not part of the game.
add_executable(island_check island_check.cpp)
target_link_libraries(island_check PRIVATE oxide_sim)
