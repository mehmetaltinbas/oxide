# File conventions

How every artifact, constants, types, interfaces, enums, utility functions, is split into files and
where those files live. These rules cover everything that is not a system class.

## Granularity

### Rule

**One artifact per file, and nothing else in the file.** Each file holds exactly one of: one system
class, one constant, one utility function, one type alias, one interface, one enum.

- `*.constant.ts`: one constant
- `*.util.ts`: one utility function
- `*.type.ts`: one type alias
- `*.interface.ts`: one interface
- `*.enum.ts`: one enum

There are **no barrel files.** Nothing re-exports anything.

### Why

- A search for the symbol lands on the file that owns it, not on a 700-line aggregate.
- Renaming, deleting or moving one artifact doesn't touch unrelated neighbours.
- Skipping barrels keeps import paths explicit, so the path itself says which feature a symbol comes
  from, and one item changing doesn't invalidate an "import everything" module.
- It removes the constant judgment call about whether something deserves its own file. The answer is
  always yes.

### Counter-examples (do not do this)

```ts
// BAD: one config module holding the whole game's tuning
export const PLAYER = { ... };
export const ENEMIES = { ... };
export type BuildKind = 'wall' | 'door';
export const BUILDINGS = { ... };
```

Split by owner and by kind:

```
features/survival/constants/player.constant.ts
features/enemies/constants/enemies.constant.ts
features/building/types/build-kind.type.ts
features/building/constants/buildings.constant.ts
```

A helper duplicated across two system files is NOT a reason to leave it in both, extract it to its
own `*.util.ts` and import it from each.

### How to apply

```
features/<feature>/
  <feature>.ts                    the system class, if the feature has one
  constants/
    <name>.constant.ts            one constant: <NAME>
  utils/
    <name>.util.ts                one function
  types/
    <name>.interface.ts           one interface
    <name>.type.ts                one type alias
  enums/
    <name>.enum.ts                one enum
```

## Naming

### Rule

The filename is kebab-case plus the kind suffix. The slug describes the artifact's **identity**, not
its kind: `BLEED_OUT_SECONDS` lives in `bleed-out-seconds.constant.ts`, never
`bleed-out-constant.constant.ts`, the suffix already says what it is.

Constants are `SCREAMING_SNAKE_CASE`; types, interfaces and classes are `PascalCase`; utilities are
`camelCase`.

### Why

Given a symbol you can derive its filename, and given a filename you can derive its symbol. Neither
requires a search.

## When to extract vs. when to inline

### Rule

The rule is _one export per file_, not _every helper must be exported_. A helper used inside a single
file, and which is purely a private implementation detail of it, may stay in that file, but it must
not be exported, so the file still has exactly one export. The moment it is needed from a sibling, it
moves to its own `*.util.ts`.

### Exceptions

- A **system class file** keeps the private helpers and module-private types that only it uses. It
  still has one export: the class.
- A definition table's private row-builder (a `const R = (…) => ({ … })` used to write forty rows
  compactly) stays with the table it builds. It is a way of writing that one constant, not an
  artifact of its own.
