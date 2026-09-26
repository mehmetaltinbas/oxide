# Oxide docs

Architecture and design conventions for Oxide, a top-down survival island in
C++ with SDL3 and no engine. These docs are the source of truth for how the
codebase is built. Read the relevant one before proposing a structural change
in its area.

The docs are split into four buckets:

- **Architecture** — how the code is shaped: principles, the source tree, and
  the file and code conventions.
- **Design system** — the single source for every visual value.
- **Systems** — how individual parts of the game work, and the rules they hold.
- **Testing** — what gets verified, and how.

## Single source of truth

The whole codebase is built around *one authoritative place per concern*, so
nothing is defined twice and nothing can drift. When two copies of a rule exist,
one of them is wrong and nobody finds out until they watch it happen: that is
not a hypothetical here, it is where the collision bug came from.

| Concern | Single source |
| --- | --- |
| The rules of the game | `sim/`, shared by the client and the server |
| What an item is and does | `sim/src/item.cpp`, one row per item |
| What an animal is and does | `sim/src/npc.cpp`, one row per kind |
| What a resource node is worth | `sim/src/node.cpp`, one row per kind |
| What a recipe costs | `sim/src/craft.cpp`, one row per recipe |
| What is solid, and standing against it | `sim/include/sim/collide.hpp` |
| Colour, ink weight, spacing of a mark | `client/src/palette.hpp` |
| Colour, radius, spacing of the interface | `client/src/ui.hpp` |
| How one item is drawn, anywhere | `client/src/held.cpp`, one glyph per item |
| The wire format | `sim/include/sim/net/protocol.hpp` |

## Placeholders

Examples use **slots in angle brackets** that you replace with real names from
the codebase. A slot is never a real identifier, it marks where one of yours
goes. The casing of the slot tells you the casing of the replacement.

| Slot | Replace with | Example replacement |
| --- | --- | --- |
| `<system>` | a system class in `sim/` | `BuildSystem`, `NpcSystem` |
| `<kind>` / `<Kind>` | a variant and its enum | `Wolf` / `NpcKind` |
| `<Def>` | the definition struct for a kind | `ItemDef`, `NpcDef` |
| `<name>` / `<Name>` / `k<Name>` | an identifier (snake file / PascalCase type / constant) | `world_life` / `ResourceNode` / `kDropLifetime` |
| `<token>` | a design token | `kInkWidth`, `ui::kAccent` |

**Fixed, non-slot names** are only the platform primitives you don't own: the
`SDL_` and `TTF_` functions, and the standard library.

## Document structure

Convention docs here follow a standard skeleton so they read consistently and
are easy to extend. When adding or editing one, keep to it:

1. **Title + one-paragraph intro**: what the doc covers and when to read it.
2. One **`## <Topic>`** section per rule the doc establishes. Inside each, in
   this order (omit a part only when it genuinely doesn't apply):
    - **`### Rule`**: the convention itself, stated imperatively.
    - **`### Why`**: the reasoning; what breaks without it.
    - **`### How to apply`**: concrete steps or a code example.
    - **`### Exceptions`**: where the rule legitimately doesn't hold.

Short single-rule docs may use the four `###` headings directly under the title
without a `## <Topic>` wrapper. Principle and overview docs are lists by nature
and don't use this skeleton.

## Architecture

- [design-principles.md](architecture/design-principles.md): the principles every change is measured against, in the idioms of a fixed-timestep game loop.
- [project-structure.md](architecture/project-structure.md): three binaries over one rules library. What may include what, and why the arrows only point one way.
- [file-conventions.md](architecture/file-conventions.md): one artifact per file, where each kind lives, how files and identifiers are named.
- [code-conventions.md](architecture/code-conventions.md): headers, namespaces, ownership, and the rule that the simulation never draws.
- [indentation.md](architecture/indentation.md): tabs, four wide, and why a block always indents its contents.
- [no-hardcoded-values.md](architecture/no-hardcoded-values.md): tuning numbers live in a definition or a named constant, never inline at the call site.
- [data-driven-definitions.md](architecture/data-driven-definitions.md): a definition table per variant family instead of a `switch` on a kind.

## Design system

- [design-tokens.md](design-system/design-tokens.md): tokens as the single source for every visual value, what each ink width means, and the ones that must move together.
- [theming.md](design-system/theming.md): a theme is a set of token values; changing the look never touches a draw call.

## Systems

- [world-scale.md](systems/world-scale.md): world units versus points. Ink, bars and name tags scale with the view; the interface does not.
- [fair-view.md](systems/fair-view.md): everybody sees the same rectangle of island; a wider monitor gets black bars, not more sight.
- [screen-text.md](systems/screen-text.md): no prose during play. Say it with the thing itself or with a sound, never with a sentence.
- [collision.md](systems/collision.md): one routine decides what is solid, and everything that walks calls it.
- [wildlife.md](systems/wildlife.md): the roster, which country each animal lives in, and the rank-and-diet food chain.
- [regrowth.md](systems/regrowth.md): everything taken from the island comes back, on one clock.
- [ground-items.md](systems/ground-items.md): everything dropped rots at the same rate, with no exceptions.
- [inventory.md](systems/inventory.md): the two containers, stack limits, where a picked-up item goes, magazines, the crafting queue.
- [building-economy.md](systems/building-economy.md): what a piece costs, what it takes to break, and the decay clock.
- [ui.md](systems/ui.md): the two interface contexts, the tabbed screen, and the belt.
- [networking.md](systems/networking.md): the authoritative server, interest management, and seed-based world sync.
- [performance.md](systems/performance.md): the frame budget, what is allowed to cost what, and the measurements behind the caps.
- [settings.md](systems/settings.md): the bindings screen and what is adjustable.

## Testing

- [testing.md](testing.md): critical-operations-only. Headless checks that drive the real rules and assert on real state.
