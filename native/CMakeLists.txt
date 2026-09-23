cmake_minimum_required(VERSION 3.24)
project(oxide CXX)

# The rewrite of the TypeScript game into C++. Three pieces:
#
#   sim/     the game itself: world generation, collision, movement, combat.
#            It knows nothing about windows, drawing or sockets, so the server
#            can run it untouched and the two sides can never disagree about
#            the rules the way the old client and Node server did.
#   client/  SDL3: a window, input, drawing, sound.
#   server/  headless and authoritative, ENet over UDP.
#
# The libraries are fetched and built here rather than installed on the machine,
# so a build on this Mac and a build on the CI's Windows runner use the same
# versions of everything.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

# Windowing, input, drawing and audio for the client.
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.16
    GIT_SHALLOW TRUE
)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

# Reliable and unreliable channels over UDP, which is what a game needs and
# what WebSocket and JSON could never give us.
FetchContent_Declare(
    enet
    GIT_REPOSITORY https://github.com/lsalzman/enet.git
    GIT_TAG v1.3.18
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL3 enet)

# ENet's own build predates the habit of telling consumers where its headers
# are, so it is said here once rather than in both binaries that use it.
target_include_directories(enet PUBLIC ${enet_SOURCE_DIR}/include)

add_subdirectory(sim)
add_subdirectory(client)
add_subdirectory(server)
