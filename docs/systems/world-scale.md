# World scale

What is drawn in the world is measured in world units, and what is drawn on the
interface is measured in points. Read this before drawing anything that sits on
the island: a border, a health bar, a name tag, a mark inside a shape. Getting
it wrong is the easiest mistake in this codebase to make and the easiest to
miss, because at a zoom of one every wrong value looks right.

## Everything in the world scales with the view

### Rule

If a thing belongs to the world, every measurement of it is in **world units**
and is multiplied by the view's scale before it reaches the screen. That
includes the parts that are not the object itself:

- the black line round it,
- the marks inside it: hatching, stipple, grain, veins,
- its health bar, its border and its height,
- its name tag, and the type size of that tag,
- the gap between the tag and the thing it names.

Zoom in and all of it gets bigger together. Zoom out and all of it gets smaller
together. **The proportion between an object and its own ink never changes.**

Nothing in the world is ever sized off the display density alone. Density says
how many pixels a point is worth; the view's scale says how many pixels a world
unit is worth. They are different numbers and only the second one belongs here.

### Why

A border that keeps its pixel width while the object behind it doubles stops
reading as a drawn line and starts reading as a sticker laid over the picture.
At high zoom the object swells away from its outline; at low zoom the outline
eats the object, and a distant tree becomes a black dot. The same applies to a
name tag: type pinned to the screen turns a zoomed-out island into a wall of
labels with nothing under them, and a zoomed-in one into a whisper.

The failure is silent. At the default zoom of one, world scale and display
density are often the same number, so a value written against the wrong one
looks correct until somebody turns the wheel.

### How to apply

In the C++ client the pen carries the scale, so no caller has to remember it:

```cpp
// Once, at the top of the frame, before anything in the world is drawn.
paint.useWorldScale(static_cast<float>(scale));   // scale = zoom * density
...
// Once, before the interface.
paint.useWorldScale(1);
```

`Paint::line` multiplies every width by that scale, and every outline, inked
shape and circle in the codebase goes through `Paint::line`, so passing
`kInkWidth` gives a line of `kInkWidth` **world units** wherever you are.

Lengths the pen cannot see are still yours to convert. A bar's width and
height, a tag's type size, the drop from a thing's top to its label: multiply
each by the view's scale by hand.

```cpp
const float w = 32 * scale;          // yes: a bar 32 world units across
const float h = 2.5f * scale;        // yes
const float w = 32 * density;        // no: pinned to the screen
```

When you are drawing inside a frame of your own, hand the pen that frame's
scale and express widths in the frame's units. `drawHuman` does this: a body is
laid out at a radius of 13, so it sets the pen to the body's scale, draws with
numbers in body units, and puts the pen back when it is done. Anything it calls
while that is set, a held tool for instance, inherits the right pen for free.

If you have already worked a width out in screen pixels, hand it back through
`Paint::inWorld` rather than letting the pen scale it a second time.

### Exceptions

The interface is not in the world and does not follow this rule. The belt, the
panels, the gauges, the prompt, the title and the death screen are measured in
points and multiplied by the display's density, so they stay the same physical
size on the screen whatever the view is doing. The map screen is an interface
surface too, not a window into the world.

Floating text over the world is world-scaled like everything else, even though
it is lettering, because it is attached to a place rather than to a corner of
the screen.
