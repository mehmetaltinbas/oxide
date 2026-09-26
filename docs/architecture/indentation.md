# Indentation

Every line of code in this repository is indented with tabs, four columns wide.
Read this before writing a file, and before reaching for a formatter's
defaults.

## Tabs for indentation, four wide

### Rule

One tab per level of nesting. No line of code is ever indented with spaces.

A block always indents its contents. A body written flush against its opening
brace is not acceptable, however short it is:

```cpp
// No.
void useWorldScale(float scale) {
worldScale_ = scale;
}

// Yes.
void useWorldScale(float scale) {
	worldScale_ = scale;
}
```

Four is the width. `.editorconfig` at the root sets `indent_style = tab` and
`tab_width = 4`, so anyone who opens the project sees four columns without
having configured anything.

### Why

Indentation is what makes the shape of a function readable at a glance, and a
line that sits flush against its block hides that shape entirely. Tabs, rather
than spaces, because a tab is one level of indentation by definition: the
reader chooses how wide a level looks, and the file does not have to be
reformatted for them to change their mind.

Four columns because it is wide enough to see the nesting at a glance and
narrow enough that five levels still fit on a line.

### How to apply

Indent with tabs. Align with spaces. A continuation line that sits under an
open bracket is alignment, not nesting, so it gets the enclosing line's tabs
and then spaces up to the column:

```cpp
	const float lean =
		(v(variant, 3) - 0.5f) * r * 0.16f;          // nesting: one more tab
	paint.inkedPoly({{x, t.y - t.r}, {x - t.r, t.y}},
					nodeColor(sim::NodeKind::Tree)); // alignment: tabs then spaces
```

Written that way, the alignment holds at four columns and the nesting holds at
any width.

`.clang-format` at the root says the same thing, so an editor that formats on
save produces it without being asked.

### Exceptions

YAML and JSON are indented with four spaces, because tabs are not valid in YAML
and every tool that writes these files writes spaces. `.editorconfig` says so.

Markdown keeps spaces inside list continuations for the same reason: a tab
changes what a list item means.

A namespace body is **not** indented. A namespace wraps the whole file, so
indenting it distinguishes nothing and costs a level of width on every line;
this is what every C++ project does and `.clang-format` enforces it.
