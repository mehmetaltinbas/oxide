# The fair view

Everybody sees the same rectangle of island, whatever they are playing on. Read
this before changing anything about the camera, the zoom or the window.

## A monitor is not an advantage

### Rule

The world is drawn inside a rectangle of a fixed shape, sixteen by nine, that
always shows the same amount of ground: 1280 by 720 world units at a zoom of
one. A bigger window draws that rectangle larger. It does not draw more of it.
Anything outside it is covered over.

Three things follow, and none of them may be quietly broken:

- Resolution buys sharpness, never sight. A 4K window sees exactly what a 720p
  window sees, larger.
- Shape buys nothing. An ultrawide gets black bars down the sides, not two
  hundred more units of ground to the left and right.
- Pixel density buys nothing. A retina display and an ordinary one at the same
  size in points see the same rectangle.

Zoom is the player's own choice and applies to everybody equally, so it is not
an advantage: anyone can turn the wheel.

### Why

This is a game where what you can see before something sees you decides fights.
If a wider monitor shows more island, then the shop that sells monitors is
selling an advantage, and the game is pay to win however carefully the rest of
it is balanced. Nobody should have to buy hardware to compete, and nobody
should have to think about whether they are at a disadvantage for playing on a
laptop.

Letting the world fill an odd-shaped window is the easy thing to do and every
engine does it by default. It is worth the black bars not to.

### How to apply

The camera scale is worked out once a frame, from the window, in points:

```cpp
// The largest rectangle of the right shape that fits this window.
double fairW = pointsW;
double fairH = pointsW / kViewAspect;
if (fairH > pointsH) {
	fairH = pointsH;
	fairW = pointsH * kViewAspect;
}
// Drawn as much bigger as that rectangle is, so the ground in it is the same.
const double fit = std::sqrt(fairW * fairH / kViewArea);
const double scale = zoom * fit * density;
```

`kViewArea` is the reference rectangle and `kViewAspect` its shape; both live
beside the zoom limits in the client. The margins are filled after the world is
drawn and before the interface, so the interface still runs the full width of
the window: the bars hide island, not your health.

Anything that converts between the screen and the world, the cursor's position
above all, uses the same `scale`. There is one number and everything reads it.

### Exceptions

The interface is not held to this. A wider window gets a wider belt and a
wider crafting screen, because none of that is information about the island.

The map screen is not held to it either: the map shows what you have walked,
and it is the same map at any size.
