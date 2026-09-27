# Channelled actions

Anything the player does that takes time rather than happening at once is a
**channelled action**: a reload, a bandage, a syringe, and whatever is added
next. They all behave the same way, and they behave that way because of where
the rule is written, not because each one remembered to.

## The rule

**A channelled action is bound to the item in your hand, and stops the moment
that item is not in your hand.**

Putting the rifle away stops the magazine change. Putting the bandage back in
your pack stops the dressing. You cannot finish shoving a magazine into a gun
you have slung over your shoulder.

What does **not** stop one:

- Being hit. See [the reason](#what-does-not-interrupt-one).
- Walking. Only a sprint stops an apply, and only a rocket cannot be loaded at
  a run.

## How the rule is enforced

Not by the code that changes the slot. **By the action itself, every tick.**

```cpp
if (!stillInHand(inventory, player.applying)) {
    // stop
}
```

`stillInHand` is in `sim/features/survival/utils/channelled.util.hpp`.

This matters more than it looks. The alternative is cancelling the action
wherever the held slot changes, and the held slot changes in a lot of places:
the number keys, the scroll wheel, dropping the item, a swap in the pack
screen, a trade out of a container, death, a save being loaded. Every one of
them is a place to forget, and the one that gets forgotten is found by a
player, not by a reader.

Asking the question from inside the action inverts that. A new way of changing
slots gets the behaviour free. A new channelled action gets it by asking the
same question, in the same shape, in its own tick.

## Adding a channelled action

1. Put what is being done, and how long is left of it, on `Player`. Name the
   "what" after the item it is being done with, as `applying` and `loaded` are.
2. In its tick routine, first line: `if (!stillInHand(inventory, <what>))`,
   then clear it and return.
3. Decide, and write down beside the code, what else stops it: a sprint, water,
   a particular item. Being hit is not on that list, by default.

## What does not interrupt one

Being hit does not cancel a reload, a bandage or a syringe. It used to, and it
made the moment you most needed one of them the moment you could not have it: a
wolf on you cancelled the dressing with every bite, so the dressing existed
only for fights you were already winning. The `interrupt` check in
`tools/play_check.cpp` asserts it stays that way.
