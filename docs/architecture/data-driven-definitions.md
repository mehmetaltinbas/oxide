# Data-driven definitions

How this codebase replaces `switch` and `if/else` chains on a kind with one definition table per
family. Read this before adding a new enemy, item, building, biome or any other variant.

## Rule

A family of variants is expressed as a **kind union**, a **definition interface**, and a **table
mapping every kind to its definition**. Code that behaves differently per kind reads fields off the
definition; it does not branch on the kind.

```
features/<feature>/types/<kind>.type.ts        export type <Kind> = 'a' | 'b' | 'c';
features/<feature>/types/<def>.interface.ts    export interface <Def> { … }
features/<feature>/constants/<defs>.constant.ts  export const <DEFS>: Record<<Kind>, <Def>> = { … };
```

### Why

- **Adding a variant is adding a row.** The compiler then tells you about every table that must gain
  an entry, because `Record<<Kind>, <Def>>` is exhaustive by construction. A `switch` gives you a
  silent fallthrough instead.
- **All of a variant's behaviour is in one place**: one row, rather than scattered across the nine
  `switch` statements that mention it.
- It is Open-Closed made concrete: new behaviour extends the data, not the code.

### How to apply

Adding a kind:

1. Add the string to the `<Kind>` union.
2. Add the row to `<DEFS>`. TypeScript will refuse to compile until every exhaustive table has it.
3. If the new kind needs behaviour no existing field expresses, **add a field to `<Def>`** and give
   every existing row a value for it. Do not add an `if (kind === …)` at the call site.

The last step is the one that matters. A field with a sensible default for everything else is how a
family stays open to extension; a special case at the call site is how it stops being one.

### Exceptions

- **Drawing.** A renderer genuinely does draw a bear differently from a wall, and no data table
  expresses "which pixels". Rendering switches on kind, and that is where the special-casing is
  allowed to live.
- **A field that only one kind can ever have** may be optional on `<Def>` and read behind a
  presence check. `shootThrough?: boolean` is a property of the piece, not a branch on its name.
- **One-off orchestration**: a single place in the game flow that treats one kind specially because
  of the story, not because of the mechanic.
