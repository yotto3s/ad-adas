# Doxygen Style Guide

This document defines the rules for writing Doxygen documentation
comments in Arcanum. Rules are derived from the [LLVM Coding
Standards](https://llvm.org/docs/CodingStandards.html#doxygen-use-in-documentation-comments);
deviations from LLVM are called out explicitly.

Arcanum's `docs` CMake target runs Doxygen with `WARN_AS_ERROR =
FAIL_ON_WARNINGS`, so any violation of the *machine-checked* rules
below fails the build (and CI).

## Machine-checked rules

These are enforced by Doxygen itself:

1. **Every public symbol must have a documentation comment.**
   Triggered by `WARN_IF_UNDOCUMENTED = YES` + `EXTRACT_ALL = NO`.
   Classes, structs, enums, non-static free functions, public/protected
   members, typedefs/using aliases, and namespaces must carry at least
   a `\brief` or first-sentence summary. Private members are exempt
   (`EXTRACT_PRIVATE = NO`).

2. **`\param` names must match actual parameter names.** Triggered by
   `WARN_IF_DOC_ERROR = YES`. Stale `\param` entries after a rename are
   build failures.

3. **Cross-references (`\ref`, `\p`, `\see`) must resolve.**

4. **No unbalanced inline HTML.** (Markdown with stray `</tt>` or
   similar fails.)

## Style rules (convention — enforced via code review)

Doxygen does not mechanically check these, but they are required:

### Comment syntax

Use C++-style `///` for documentation comments. Do **not** use
`/** ... */` blocks.

```cpp
/// Brief description of the function.
///
/// Longer description here.
void foo();
```

### Command prefix

Use backslash (`\brief`, `\param`, `\returns`, `\p`, `\file`, `\code`,
`\ref`, `\see`). Do **not** use the at-sign form (`@brief`).

### File header

Every `.h` and `.cpp` file begins with a `\file` doc block:

```cpp
/// \file
/// Declares the ECSL MLIR dialect class.
```

The first sentence becomes the file's brief. Keep it to one line.

### Brief and detailed description

With `JAVADOC_AUTOBRIEF = YES`, the first sentence (ending at the first
period followed by whitespace) becomes the brief. An explicit `\brief`
is only needed when the first sentence is not a suitable summary.

```cpp
/// Returns the verifier pass for this dialect.
///
/// The pass runs ownership analysis before emitting errors.
std::unique_ptr<Pass> createVerifier();
```

- Do not repeat the function/class name.
- Keep the brief to one line.

### Parameters and returns

Use `\param name` (or `\param [out] name`, `\param [in,out] name`).
Use `\p name` when referring to a parameter from prose:

```cpp
/// Sets the verbosity to \p level.
/// \param level Integer in [0, 3]; higher is more verbose.
void setVerbosity(int level);
```

Use `\returns` on a new paragraph to document return values. Omit
`\returns` when the brief already describes the return (e.g. "Returns
the verifier pass...").

### Code examples

Wrap non-inline examples in `\code` / `\endcode`:

```cpp
/// Loads the dialect into \p ctx.
///
/// \code
/// mlir::MLIRContext ctx;
/// ctx.loadDialect<ECSLDialect>();
/// \endcode
void loadInto(mlir::MLIRContext &ctx);
```

### Where docs live

Public-API documentation belongs in **headers**, not .cpp files.
Doxygen's `FILE_PATTERNS` only includes `*.h`, `*.hpp`, `*.td`, and
`*.md`; .cpp files are not processed. Use plain `//` comments in .cpp
files for implementation notes.

## Deviations from LLVM

- **Namespaces:** Arcanum uses flat per-dialect C++ namespaces (for
  example `ecsl` and `testgen`, each at the top level) rather than an
  umbrella project namespace such as `arcanum::ecsl`.
- **EXTRACT_ALL:** We use `NO` (LLVM uses `YES`) because our tree is
  greenfield and we can afford strict undocumented-symbol enforcement
  from day one.
