# Design principles

The principles every change in `src/` is measured against. SOLID first, then the guidance that is
specific to a fixed-timestep canvas game with no engine under it. When a design decision is
contentious, point at the principle that resolves it.

## 1. Single Responsibility Principle (SRP)

Each system, class or function has one reason to change. `BuildSystem` owns what is built;
`Renderer` owns how it looks. A bug in wall placement has exactly one file to open.

## 2. Open-Closed Principle (OCP)

Adding a new enemy, item, building or biome is adding a row to a definition table, not adding a
branch to a `switch`. See [data-driven-definitions.md](data-driven-definitions.md).

## 3. Liskov Substitution Principle (LSP)

Anything that satisfies a definition interface, `<Def>`, must be usable everywhere that interface
is accepted. A definition that needs the calling code to special-case it is a definition that needs
another field, not another `if`.

## 4. Interface Segregation Principle (ISP)

A system asks for the narrow set of callbacks it needs, not for the whole game. Systems take a
`<System>Hooks` object listing exactly the operations they invoke upward. Nothing takes a reference
to `Game`.

## 5. Dependency Inversion Principle (DIP)

Systems depend on the hooks interface they declare, not on the orchestrator that satisfies it. The
orchestrator wires the concrete implementations at construction time and is the only thing that
knows about all of them.

## 6. Composition over inheritance

There is no entity class hierarchy. An actor is a plain record plus the systems that act on it.
Behaviour is combined by running several systems over the same data, not by extending a base class.

## 7. Don't Repeat Yourself (DRY)

Every piece of knowledge has one home. A tuning number lives in one `*.constant.ts`; a colour lives
in one token; a shape lives in one `*.interface.ts`. A helper duplicated across two files becomes a
`*.util.ts` that both import.

## 8. Separation of Concerns (SoC)

Simulation never draws, and drawing never mutates. `update(dt)` advances the world; `draw()` reads
it. This is what lets the simulation run at a fixed 60Hz while rendering runs at whatever the display
gives us, and it is why a rendering change can never alter the outcome of a fight.

## 9. Law of Demeter (LoD)

A system reaches for what it was handed, not for the whole object graph behind it. If a system finds
itself walking `game.world.something.other`, the thing it actually needed belongs in its hooks.

## 10. Keep It Simple, Stupid (KISS)

There is no engine here on purpose. Prefer a plain array and a loop over a clever abstraction; add
the abstraction when the second caller arrives, not in anticipation of it. Measure before optimising

- every performance rule in these docs came from a measurement, and the measurement is written down
  next to the rule.

## 11. Measure, don't guess

Balance numbers and performance budgets are derived from measurements taken in the running game, and
the measurement is recorded in a comment beside the value it justifies. "It felt slow" is not a
reason to change a number; "0.38ms became 7.2ms" is.
