# Theming

A theme is a set of token values. Changing the look never touches a draw call.

### Rule

Everything that decides how the game looks is a value in
`client/design/tokens/world.tokens.hpp` or `client/design/tokens/interface.tokens.hpp`. A different look is a
different set of those values and nothing else.

### Why

It is the test of whether the tokens are actually doing their job. If turning
the palette dark required editing drawing code, then the drawing code contains
visual decisions it should not have.

### How to apply

The game ships one look: comic, inked, warm. To change it, change the values.
Nothing in `client/features/*.cpp` should need editing to do it, and if something
does, that is a colour or a width that escaped into a draw call and belongs in
a token.

### Exceptions

Night is not a theme. It is a veil laid over the finished frame with holes cut
in it by whatever is alight, and it is a system rather than a set of values:
see `client/features/render/systems/night.hpp`.
