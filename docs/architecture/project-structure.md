# Project structure

Three binaries built over one rules library. Read this before adding a file, and
before reaching for an include that crosses a boundary.

```
CMakeLists.txt     the three binaries and the fetched libraries
sim/               the rules of the game. No SDL, no sockets, no platform
  include/sim/     the headers every other part includes
  src/
client/            the game you play: window, input, drawing, sound
server/            the headless authority, run on a machine with no screen
tools/             headless checks that drive the real rules
assets/            fonts, and anything else read at runtime
docs/              this
```

## The arrows point one way

### Rule

`sim/` knows nothing about anything else. It includes no SDL, opens no socket,
reads no file it was not handed, and draws nothing. `client/`, `server/` and
`tools/` all include `sim/`, and none of them includes another.

If two of them need the same thing, it moves into `sim/`.

### Why

The client and the server have to agree about the rules exactly, or a player
sees one game and the server runs another. The way to make that impossible is
for there to be one copy of the rules that both of them use rather than two
that are kept in step by hand. This is the single largest thing the C++ rewrite
bought: the game it replaced kept its numbers in three places and they drifted.

It also makes the checks in `tools/` worth something. They link `sim/` and
nothing else, so what they assert on is the real game and not a test double of
it.

### How to apply

Ask what a thing is. If it is a rule about what happens, it goes in `sim/`. If
it is about what that looks like or sounds like, it goes in `client/`.

```cpp
// sim/: how far a wolf can see, how much a blow takes off.
inline constexpr double kSkittishRange = 150;
// client/: what a wolf is drawn with, and how heavy the line round it is.
constexpr Color kInk = rgb(0x14110d);
```

A header in `sim/include/sim/` is public to all three. A header in
`client/src/` is private to the client. There is no third case.

### Exceptions

`tools/` may reach into the client for something it is checking about drawing,
as `sound_check` does: it renders the audio to memory with no sound card. That
is a check of the client, so it links the client. Nothing in `client/` or
`server/` may do the same in reverse.

## One binary per job

### Rule

`client/` produces `oxide`, `server/` produces `oxide_server`, and each file in
`tools/` produces its own check binary. A binary does one job and is run one
way.

### Why

A check that has to be reached through a flag on the game is a check nobody
runs. `./build/bin/play_check` is a command; `--run-tests` is a thing you have
to remember.

### How to apply

Adding a check means adding one `.cpp` to `tools/` and one line to
`tools/CMakeLists.txt`. It prints what it found in plain English and returns
non-zero when the thing it exists to catch has happened.
