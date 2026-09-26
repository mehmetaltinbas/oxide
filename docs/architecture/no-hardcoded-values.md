# No hardcoded values

Every number that could be tuned lives somewhere it can be found, with the
reason it is that number beside it. Read this before typing a literal into a
call.

## A tuning number has a name and a reason

### Rule

A number that decides how the game plays or looks is a named constant or a
field on a definition. It is never a literal at the call site, and it carries a
comment saying what it is pinned by.

### Why

An unnamed number cannot be found. When the wolf felt wrong there was one place
to look, because its speed is a field on its row with a paragraph beside it
explaining that it has to sit under a sprint and over an elk. Had it been a
`184` inside the chase code, the only way to find it would have been to read
the chase code.

The reason matters as much as the name. Half these numbers are pinned from both
sides, and moving one to satisfy one constraint silently breaks the other.

### How to apply

```cpp
// No.
if (dist(npc.x, npc.y, player.x, player.y) < 150) { /* bolt */ }

// Yes.
/**
 * How close you get to something peaceful before it bolts.
 *
 * At 260 anything you could see was already running, so the herds were never
 * seen grazing, only fleeing; at 150 you watch an elk until you take a step
 * too many. A hit makes it run whatever the distance.
 */
constexpr double kSkittishRange = 150;
```

Where the number belongs:

| What it tunes | Where it lives |
| --- | --- |
| One variant's behaviour | a field on its row in the definition table |
| A rule shared by a whole system | a `constexpr` at the top of that system's header |
| A colour, an ink weight, a spacing | `client/src/palette.hpp` or `client/src/ui.hpp` |
| The shape or size of one drawing | named locals at the top of that drawing routine |

### Exceptions

Numbers that are the geometry of a specific drawing stay in it: the `0.46` that
puts an arrowhead at the end of its shaft is not a tuning value, it is part of
the picture, and hoisting it out would scatter one drawing across two files.
They still get names when they mean something, as `pawAlong` and `pawAcross`
do.

Zero, one, and two as counts and indices are not tuning values.

## Measure, then write the measurement down

### Rule

A balance number or a performance claim comes from the running game, and the
measurement goes in a comment beside the value it justifies.

### Why

Guessed numbers cannot be defended or revisited: nobody later knows whether 176
was chosen or typed.

### How to apply

Change the number, run the game or the relevant check, and record what you saw:

```
outrun: sprinting away from a wolf for ten seconds, gap 178 from 90
speeds: sprint 185, wolf 176, bear 150, elk 165, kangaroo 168
```

Where a number is pinned by a relationship rather than by taste, add a check
that asserts the relationship. `play_check` asserts that a sprint escapes a
wolf and that a wolf catches an elk, because either one alone can be satisfied
by a number that breaks the other.
