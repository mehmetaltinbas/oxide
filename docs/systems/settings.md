# Settings

`[Esc]` pauses and opens the settings panel. Two pages: **Controls** and **Sound**.

## Controls is the only copy

Every binding lives in `src/features/ui/constants/controls-reference.constant.ts` and nowhere else.
The hint strip under the belt is gone: it took up screen the whole run to say something you need
once. **Adding a binding means adding a row to that constant.**

The contextual line above the belt stays. That one only appears while you hold a building plan, and
it is about what you are doing right now rather than a general reference.

`[H]` is unchanged and stays: it explains how the island works, which is a different question to
which key does it.

## Display

Fullscreen, which has to be entered inside a user gesture. That is why the
toggle lives in the panel and is never called from the game loop.

### [Esc] does one thing at a time

The browser reserves `[Esc]` for leaving fullscreen and a page cannot prevent
it. Without help, that one keypress did two jobs: it dropped out of fullscreen
**and** closed whatever menu was open, so turning fullscreen on from settings and
pressing `[Esc]` to close the panel left you with neither.

`fullscreen.ts` records when fullscreen last ended.
`escapeSpentOnFullscreen()` answers whether this `[Esc]` went on leaving it, and
every Escape handler returns early when it does. The keypress is spent; the next
one closes the menu.

It is single-shot, so two handlers cannot both swallow the same press.
**Any new Escape handler has to call it first.**

That is as far as the guard can go: `[Esc]` will always leave fullscreen, and no
page can take the key back. So the panel also has a **Resume button**. In
fullscreen you click it and stay fullscreen; the key is never the only way out
of a menu.

## Sound

Master volume and a mute, both on `Audio`:

- `audio.volume` / `audio.setVolume(0..1)`
- `audio.enabled` / `audio.setEnabled(on)`

Mute is separate from volume zero on purpose: turning sound off and back on returns you to the level
you had chosen rather than to silence.

The level is a field on `Audio` rather than a read of the gain node, because the node does not exist
until the first user gesture unlocks audio, and the setting has to survive being changed before then.

## The same panel exists in lumberwave

Same two pages, same audio API, same rule about the controls constant. The two codebases do not
share code, so a change to one is a change to make twice.

## Clothing shows on the character

`player.wearing` is drawn as a garment over the body, not as a tint of it. Bare
skin is `WORLD.skin`, its own token: it used to borrow `WORLD.sand`, and once
the torso used it too, an unclothed player standing on a beach was the same
colour as the beach.

A hood (the hazmat suit) is drawn **after** the head, in `drawHood`, because
drawing it with the rest of the garment put it underneath.

You start with nothing on: a rock, a building plan, and an empty worn slot.

## The worn slot is a container

`player.worn` is a one-slot `Container`, not a bare id, so the inventory screen
treats it like every other slot: drag something in to put it on, drag it out to
take it off. Read it with `wornId(player)`.

The slot refuses anything without a `wear` block. That check lives in the slot
widget (`slotGrid`'s `wearableOnly`) rather than in `moveStack`, which stays dumb
and general.

Taking something off puts it back in the pack, and only on the ground if there
is nowhere for it. Taking a coat off should never be how you lose it.

## The belt only takes what you hold

`BELT_CATEGORIES` decides, via `isBeltItem`. The check sits in the slot widget
(`slotGrid`'s `heldOnly`), alongside the worn slot's `wearableOnly`, so
`moveStack` stays dumb and general.

## The progress arc

Every timed action (a bandage, a syringe, a magazine change) draws the same arc
beside the player. `PROGRESS_ARC` fixes the geometry: **always the same size**,
only the fill rate changes, so a three-second job visibly runs faster than a
five-second one.

`Game.progressJob` is the single place that decides what it shows. Add a case
there rather than a new bar. The old bar above the belt is gone.

Lumberwave has the same thing, with the same rule and no shared code.

## Bare hands

Half a rock, in damage and in yield: `FIST_FRACTION` applied to
`ITEMS.rock.melee`, derived rather than written out so the two cannot drift.
Enough to get a log and a stone together and make a rock again, and slow enough
that you want to.
