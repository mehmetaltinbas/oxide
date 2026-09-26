# File conventions

One artifact per file, a predictable place for each kind, and names that say
what a thing is. Read this before creating a file.

## One artifact per file

### Rule

A file holds one thing: one system, one definition table, one drawing routine
family, one screen. The file is named after that thing, in `snake_case`, and
the header and the source share the name.

```
sim/include/sim/npc.hpp     what an animal is         sim/src/npc.cpp     the table
sim/include/sim/npcs.hpp    the system that runs them sim/src/npcs.cpp
client/src/animal.hpp       how one is drawn          client/src/animal.cpp
```

### Why

Where a thing lives should be derivable from its name without searching. A file
that holds two unrelated things has no name that fits it, which is how
`util.cpp` happens.

### How to apply

- A header declares; a source defines. A header that needs no source (a
  definition struct, a constant, a tiny inline) has none.
- `sim/include/sim/<name>.hpp` is public. `client/src/<name>.hpp` is private.
- The singular is the thing, the plural is the system that runs many of them:
  `npc.hpp` is what an animal is, `npcs.hpp` is the system that steps them.

### Exceptions

Small families that are only ever used together stay in one file: every item
glyph lives in `client/src/held.cpp`, because splitting fifty of them into
fifty files would make the sheet impossible to read as a sheet.

## Names say what a thing is

### Rule

| Kind | Style | Example |
| --- | --- | --- |
| File | `snake_case` | `world_life.cpp` |
| Type, struct, enum | `PascalCase` | `ResourceNode`, `NpcKind` |
| Function, method, variable | `camelCase` | `keepOutOfSolids`, `dropLifetime` |
| Member | `camelCase_` with a trailing underscore | `nodes_`, `worldScale_` |
| Constant, enum value | `kPascalCase` | `kDropLifetime`, `NpcKind::Wolf` |
| Namespace | `lowercase` | `sim`, `client`, `sim::net` |

### Why

The trailing underscore is the one that earns its keep: it says at a glance
whether a name in a method body is state that outlives the call or a local that
does not.

### How to apply

A definition struct is `<Kind>Def` and its table is `k<Kind>Defs`, reached
through a `<kind>Def(kind)` accessor. Never index the table directly from
outside the file that owns it.

## Comments say why, not what

### Rule

A comment explains the reason a thing is the way it is, the measurement behind
a number, or the mistake the code is avoiding. It never restates the code.

### Why

The code already says what it does. What it cannot say is why 176 and not 184,
or that this loop is written the long way because the short way walked an
animal through a wall.

### How to apply

```cpp
// No: says what the line says.
// Set the wolf's speed to 176.

// Yes: says what the number is pinned between, and what broke.
// 176 against a sprint of 185. Under a sprint or a wolf is a death sentence;
// over an elk's 165 or it never eats. It was 184, a sprint to within a
// rounding error, and you could not get away from one at all.
```

Every balance number carries the measurement that justifies it, beside it. See
[no-hardcoded-values.md](no-hardcoded-values.md).
