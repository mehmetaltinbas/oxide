# Inventory rules

**This document is the contract for where items go. Any change to pickup,
crafting output or looting should keep these rules, and any new item category
should be classified here first.**

## The two containers

| Container          | Size     | Purpose                                             |
| ------------------ | -------- | --------------------------------------------------- |
| **Belt**           | 6 slots  | What you can hold. Slot 1-6 select the active item. |
| **Main inventory** | 24 slots | Everything you are carrying.                        |

The belt is not extra storage. It is the set of things your character can have
_in their hands_, which is why a stack of stones has no business there.

## Where a picked-up item goes

Anything that arrives without you placing it, walking over a dropped stack,
crafting output, looting, harvesting a node, is routed by whether you can
_hold_ it:

- **Holdable items go to the belt first**, and only overflow into the main
  inventory when the belt is full. Craft a hatchet and it should be in your
  hand, not buried in a bag.
- **Everything else goes straight to the main inventory** and is never
  auto-placed on the belt.

## Belt-eligible categories

Defined by `BELT_CATEGORIES` in `config.ts`:

`tool`, `weapon`, `consumable`, `deployable`, `explosive`, `clothing`

Everything else, notably `resource` and `ammo`, is **inventory only** and is
never auto-placed on the belt. Ammunition is consumed from wherever it sits, so
it does not need a belt slot.

The player may still **drag** anything they like into a belt slot by hand. This
rule governs automatic placement only; it never overrides a deliberate choice.

## Dragging

Items move by press-and-drag: hold the left mouse button on a stack, drag it,
release over the destination slot. Releasing over an occupied slot merges the
stacks when they match and swaps them when they do not. Releasing outside any
slot returns the stack where it came from.

## Magazines

A gun with `magazine` set in its item definition holds its own rounds. Those rounds live on the
`ItemStack`, in `loaded`, not on the item definition, because two revolvers in a crate are not the
same revolver: one can be full and the other empty. A stack with no `loaded` field has never been
loaded and counts as empty.

`r` reloads what you are holding: it takes up to `magazine - loaded` rounds out of the belt and
pack, and takes `reloadSeconds` to do it. Firing dry starts the reload for you rather than making
you press the key. The reload is a channel like a bandage, so walking through it is fine, breaking
into a run drops it, and so does being hit.

A weapon with no `magazine` (the bow) draws straight from the quiver, which is what nocking an
arrow is. The belt slot shows `loaded/spare` for magazine weapons and just the spare count for the
rest, in red when the thing cannot fire.

## The crafting queue

Crafting is a queue, not one job at a time. `game.craftQueue` holds up to `CRAFT_QUEUE_MAX` runs;
entry 0 is the one being made and the rest wait their turn. `game.craftJob` is a getter for the
head, so nothing that only cares about "what is being made" had to change.

Every run pays its materials **up front**, at the moment it is queued. That is what lets several
runs sit in the queue without racing each other for the same wood, and it is why cancelling has to
refund: `cancelCraft` hands back `cost x left` and drops on the floor whatever will not fit.

`reorderCraft` moves waiting runs around but refuses to touch the head, because shuffling the head
would throw away the seconds already spent on it.

Nothing plays when a craft finishes. The sound was on every single item of a long run, which turned
a stack of bandages into a machine gun.

## Item categories

An item's `category` decides which crafting tab it appears under, so it is not a cosmetic field.
Ammunition is built with the `A()` helper rather than `R()` for exactly this reason: `R()` files
everything under `resource`, which is why arrows and rounds used to turn up in the Resources tab.

## Invariants

- No item is ever created or destroyed by moving it between containers.
- A full destination returns the remainder to the source rather than voiding it.
- Death drops the contents of _both_ containers on the ground, plus worn kit.
