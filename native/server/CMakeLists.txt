# The authority: no window, no drawing, just the rules and the wire. Runs on
# the Linux box; built here so a change to the rules is compiled against both
# sides at once.
add_executable(oxide_server src/main.cpp)
target_link_libraries(oxide_server PRIVATE oxide_sim enet)
