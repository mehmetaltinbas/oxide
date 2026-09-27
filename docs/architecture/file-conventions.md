# File conventions

## One artifact per file, and nothing else in the file

A file holds **one** thing: one struct, one enum, one class, one free function
(or one small family of them that is meaningless apart), one constant table.

The name of the file is the name of the thing, in `kebab-case`, with a suffix
that says what kind of thing it is:

| Suffix            | Holds                                       | Example                          |
|-------------------|---------------------------------------------|----------------------------------|
| `*.struct.hpp`    | one struct                                  | `npc-def.struct.hpp`             |
| `*.enum.hpp`      | one enum, plus its count if it has one      | `item-id.enum.hpp`               |
| `*.constant.hpp`  | named numbers, or a definition table        | `wildlife-tuning.constant.hpp`   |
| `*.util.hpp`      | free functions with no state                | `collide.util.hpp`               |
| no suffix         | a class that holds state                    | `npc-system.hpp`, `paint.hpp`    |

A `.cpp` takes the name of the header it implements. A `.cpp` with no header of
its own is named for what it does and sits beside its siblings:
`world-nodes.cpp` and `world-life.cpp` are both parts of `world.hpp`, split
because one file of two thousand lines is not a file anybody reads.

## Why one thing per file

Because the alternative is what this codebase had: a header called `npc.hpp`
holding an enum, three structs, four free functions and three constants. Every
file that wanted the enum got all of it, a change to a constant rebuilt
everything, and nothing in the tree said what depended on what. Splitting it
was mechanical and the include graph told the truth afterwards.

The cost is more files. That is the point: a file is a unit you can move,
delete, or find by name.

## Where the name comes from

The file is named after the artifact, not after the feature it is in.
`sim/features/wildlife/types/npc-def.struct.hpp` holds `NpcDef`. Not
`wildlife-npc-def`: the folder already said `wildlife`, and repeating it in
every name is noise you read a hundred times to learn nothing.

## The two names a thing has

C++ uses `PascalCase` for types and `camelCase` for functions; files use
`kebab-case`. `NpcDef` lives in `npc-def.struct.hpp`, `stepPlayer` lives in
`step-player.util.hpp`. The translation is mechanical, so either name gets you
to the other.

## Headers

- `#pragma once` at the top. No include guards.
- Standard headers first, then project headers, each group sorted, a blank line
  between them.
- Include what you use. If you use `ItemId`, include `item-id.enum.hpp`, even
  if something else you include would have brought it in.
- Forward declare a class you only hold a pointer or reference to, rather than
  including its header.

## What does not exist

- **No barrel headers.** Nothing whose job is to include other headers.
- **No `common.hpp`, `util.hpp`, `helpers.hpp` or `misc.cpp`.** A file with a
  name like that is a place things go to be forgotten. If a helper has no
  feature, it belongs in `shared/` under its own name.
- **No relative includes.** See
  [project-structure.md](project-structure.md).
