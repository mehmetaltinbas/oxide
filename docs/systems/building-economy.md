# What building, mending and salvage cost

One rule, and every piece obeys it.

## Mending costs the share you mend

A swing of the hammer puts back some fraction of a thing's health, and costs
**that same fraction of what the thing cost to build**, rounded to the nearest
whole number.

A wall that cost 50 wood and 20 stone, mended by 10% of its maximum health,
charges 5 wood and 2 stone. Mended by 25%, it charges 13 and 5.

```
cost[k] = round(buildCost[k] * (healthRestored / maxHealth))
```

There used to be a `costFraction` discount, so a building could be repaired for
less than it cost. That made knocking your own walls down and rebuilding them a
way of making timber out of nothing.

Rounded to nearest, never up. Ceiling meant every small top-up cost a full unit
of everything, so tapping a barely-scratched wall was the most expensive way to
spend materials in the game.

## Salvage returns half, scaled by condition

Pulling down something you built returns **half its build cost, times how much
of it is still standing**.

```
back[k] = round(buildCost[k] * 0.5 * (health / maxHealth))
```

A wall at full health returns half. The same wall at a quarter health returns an
eighth. Half rather than all on purpose: at full return a wall becomes a free
scaffold you can move around for ever, and the loss is what makes placing one a
decision.

The camp shelter cannot be salvaged. It is the contract.

## Both are shown where they happen

The materials taken or returned float over the building itself, not in a corner
of the screen. You are looking at the thing you are working on.

## This applies to any piece added later

A new building is a new row in the cost table and nothing else. Do not write a
special case: if a piece needs different repair or salvage economics, the reason
belongs in this document first.
