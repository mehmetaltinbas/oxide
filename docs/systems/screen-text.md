# Text on the screen

The game does not tell you things in sentences. Read this before adding any
text to the screen at all, and before reaching for a message as the answer to
"how will they know?".

## No prose during play

### Rule

Nothing that happens during play is announced in words. There is no notice
area, no toast, no banner, no line over the belt, and no sentence floating over
your head. Not for a refusal, not for a warning, not for a confirmation, not
for a change of state.

If you are about to write any of these, you are solving the wrong problem:

- "Empty. Reload with R."
- "Crafting queue is full."
- "Need 20 wood to repair."
- "Too far away."
- "Pack full."
- "Woke up in your bag."

These are all real messages this game used to print, and every one of them has
been removed.

### Why

A sentence on the screen is the interface admitting it did not manage to show
you something. Every one of them is a thing the player has to stop and read, in
the middle of doing something else, about a fact that was already on the screen
somewhere else: the belt already says nought rounds, the queue already shows
eight chips, the pack already has no empty slot. Saying it again in English is
noise on top of information.

They also pile up. Three of them at once is a paragraph over the middle of the
island, and the moment one of them is drawn every frame by mistake it becomes a
permanent banner, which is exactly what happened here twice.

### How to apply

Say it with the thing itself, or with a sound:

| Instead of                | Do this                                                 |
| ------------------------- | ------------------------------------------------------- |
| "Empty. Reload with R."   | The belt's ammunition reads 0, and the trigger clicks.  |
| "Crafting queue is full." | The queue reads 8/8 and the Craft button greys out.     |
| "Need 20 wood to repair." | The cost line reads in red, and the click does nothing. |
| "Too far away."           | The prompt does not appear, because it is out of reach. |
| "Pack full."              | The stack falls on the ground, where you can see it.    |

A refusal that produces no visible change at all should produce a sound. That
is what `Audio::deny` is for.

### Exceptions

Three kinds of text stay, and they are not prose:

- **Numbers and gains floating off the thing they happened to.** `+12 Wood` off
  the top of a tree, `-22` off a player, `Killed a Wolf` off a body. These are
  the record of a blow landing, they are attached to where it landed, and they
  are gone in a second.
- **The name over a living thing**, which is how you tell a wolf from a bear at
  a distance.
- **The one prompt above the belt**, which says what the key under your finger
  would do right now: `E   Pick up 3 Wood`. It is not a message about something
  that happened, it is a label on an action available this instant, and it
  disappears the moment the action does. One line, never two.

Menus, the pack screen, the crafting screen and the death screen are reading
surfaces and are not covered by this at all. Write as much as they need.
