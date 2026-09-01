# Oxide docs

Architecture and design conventions for Oxide, a top-down survival island built on a plain HTML5 canvas and
TypeScript, with no engine. These docs are the source of truth for how the codebase is built. Read
the relevant one before proposing a structural change in its area.

The docs are split into buckets:

- **Architecture**: how the code is shaped: principles, the source tree, and the file/code conventions.
- **Design system**: the single source for every visual value.
- **Testing**: what gets verified, and how.
- **Systems**: how individual parts of the game work.

## Placeholders

Examples use **slots in angle brackets** that you replace with real names from the codebase. A slot
is never a real identifier, it marks where one of yours goes. The casing of the slot tells you the
casing of the replacement.

| Slot                                                             | Replace with                                                             | Example replacement                            |
| ---------------------------------------------------------------- | ------------------------------------------------------------------------ | ---------------------------------------------- |
| `<feature>`                                                      | a feature folder under `src/features/`                                   | `building`, `crafting`                         |
| `<System>`                                                       | a system class                                                           | `BuildSystem`, `Renderer`                      |
| `<kind>` / `<Kind>`                                              | a variant key and its type                                               | `revolver` / `ItemId`                          |
| `<Def>`                                                          | a definition interface for a kind                                        | `ItemDef`, `NpcDef`                            |
| `<name>` / `<Name>` / `<NAME>`                                   | a generic identifier (kebab file / PascalCase type / SCREAMING constant) | `bleed-out` / `BleedOut` / `BLEED_OUT_SECONDS` |
| `<token>`                                                        | a design token                                                           | `UI.surface`, `SPACE.md`                       | **Fixed, non-slot names** are only the platform primitives you don't own: `CanvasRenderingContext2D`, |
| `requestAnimationFrame`, DOM events, and the token sets named in |
| [design-tokens.md](design-system/design-tokens.md).              |

## Document structure

Convention docs here follow a standard skeleton so they read consistently and are easy to extend.
When adding or editing one, keep to it:

1. **Title + one-paragraph intro**, what the doc covers and when to read it.
2. One **`## <Topic>`** section per rule the doc establishes. Inside each, in this order (omit a part
   only when it genuinely doesn't apply):
    - **`### Rule`**: the convention itself, stated imperatively.
    - **`### Why`**: the reasoning; what breaks without it.
    - **`### How to apply`**: concrete steps or a code example.
    - **`### Exceptions`**: where the rule legitimately doesn't hold.

Short single-rule docs may use the four `###` headings directly under the title without a
`## <Topic>` wrapper. Principle and overview docs are lists by nature and don't use this skeleton.

## Architecture

- [design-principles.md](architecture/design-principles.md): SOLID and the architectural principles every change is measured against, in the idioms of a fixed-timestep game loop.
- [project-structure.md](architecture/project-structure.md): Feature-first source tree: one folder per feature under `src/features/`, `src/shared/` for what belongs to nobody, `src/app/` for the orchestrator.
- [file-conventions.md](architecture/file-conventions.md): One artifact per file, where each kind lives, and how files are named.
- [code-conventions.md](architecture/code-conventions.md): Absolute imports rooted at `src/`, no barrels, systems own their state, hooks instead of back-references.
- [no-hardcoded-values.md](architecture/no-hardcoded-values.md): Tuning numbers live in `constants/`, never in the code that reads them.
- [data-driven-definitions.md](architecture/data-driven-definitions.md): A definition table per variant family instead of a `switch` on a kind. This codebase's answer to the strategy pattern.

## Design system

- [design-tokens.md](design-system/design-tokens.md): Tokens as the single source for every visual value, the draw-order scale, and the one exception the rule allows.
- [theming.md](design-system/theming.md): A theme is a set of token values; changing the look never touches a draw call.

## Testing

- [testing.md](testing.md): Critical-operations-only: drive the real game in a real browser, assert on real state.

## Features

`src/features/` is a flat list. Each folder owns everything that feature needs.

| Feature    | Owns                                                                                                                                   |
| ---------- | -------------------------------------------------------------------------------------------------------------------------------------- |
| `building` | Foundations, walls, doorways, doors, deployables, tiers, the room/enclosure flood fill, and what happens when a piece takes damage.    |
| `clans`    | The AI clans: names, skills, tech, raid pacing, where they settle, what they build, and when they raid you.                            |
| `combat`   | Projectiles, thrown explosives, and the weapon and ammunition definitions.                                                             |
| `crafting` | Recipes, workbench levels, and the crafting queue.                                                                                     |
| `items`    | The item table, containers, stack limits, the belt rules, the glyph icons, using what you hold, and reaching for the world around you. |
| `net`      | The multiplayer client, the wire protocol, and the server URL.                                                                         |
| `npcs`     | Wildlife, scientists, clan members, the survivor brain, and A* navigation.                                                             |
| `render`   | The world renderer and its camera-facing constants.                                                                                    |
| `session`  | Saving, loading, and autosave.                                                                                                         |
| `survival` | The player staying alive: vitals, temperature, radiation, healing, and the use and reload channels.                                    |
| `time`     | Day length, night fraction, darkness, twilight.                                                                                        |
| `ui`       | The HUD, the panels, and the drawing primitives they share.                                                                            |
| `world`    | Worldgen, biomes, beaches, resource nodes, monuments, loot crates.                                                                     |

## Systems

How particular parts of the game actually work, as opposed to how the code is shaped.

- [systems/ai-survivor.md](systems/ai-survivor.md): How a clan member decides what to do, and the rule that they may only do things a player could do.
- [systems/inventory.md](systems/inventory.md): The two containers, stack limits, where a picked-up item goes, magazines, the crafting queue.
- [systems/networking.md](systems/networking.md): The authoritative server, client prediction, reconciliation, and seed-based world sync.
- [systems/performance.md](systems/performance.md): The frame budget, what is allowed to cost what, and the measurements behind the caps.
- [systems/ui.md](systems/ui.md): The two UI contexts, the tabbed menu screen, the channel bar, and the sandbox.
