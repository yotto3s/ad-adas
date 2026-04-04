# ECSL: Extended C Specification Language

> **Status:** Draft — v0.1.0
>
> **Date:** 2026-04-03

## Overview

ECSL (Extended C Specification Language) is a formal specification language for annotating C/C++ programs with behavioral contracts. ECSL is designed as a strict superset of ACSL (ANSI/ISO C Specification Language), the specification language used by Frama-C.

Any valid ACSL annotation is also a valid ECSL annotation. ECSL extends ACSL with additional constructs to support modern safety-critical software development, particularly in the context of restricted C/C++ subsets targeting functional safety standards such as ISO 26262 (ASIL-D).

## Design Principles

- **ACSL compatibility**: All ACSL constructs are supported with their original semantics. Existing ACSL-annotated code requires no modification.
- **Incremental extension**: New constructs are introduced as additions, never as modifications to existing ACSL semantics.
- **Safety-oriented**: Extensions prioritize expressiveness for safety-critical properties that are cumbersome or error-prone to encode in standard ACSL.
- **Tool-agnostic**: While ECSL is designed for use with the Arcanum verification toolchain, the specification language itself does not assume a particular verification backend.

## Relationship to ACSL

ECSL inherits the full ACSL specification, including but not limited to:

- Function contracts (`requires`, `ensures`, `assigns`, `frees`, `allocates`)
- Assertions and loop annotations (`assert`, `loop invariant`, `loop assigns`, `loop variant`)
- Logic functions and predicates
- Lemmas and axiomatics
- Built-in predicates (`\valid`, `\valid_read`, `\separated`, `\freeable`, `\allocable`)
- Data labels and `\at` expressions
- Ghost code
- Inductive predicates
- Higher-order logic constructs

For the complete ACSL reference, see: https://frama-c.com/html/acsl.html

## Language Modes

ECSL supports annotating both C and C++ source code. The host language mode is determined by the `--std` flag (e.g., `--std=c99`, `--std=c++17`), which is passed directly to the Clang frontend.

### Validation Policy

Arcanum does not perform its own language validation. All syntactic and semantic checking of the host language is delegated to Clang. If Clang accepts a construct (with or without warnings), Arcanum accepts it. If Clang rejects it, Arcanum reports the error.

This means that C99-specific constructs such as compound literals (`(int){42}`) appearing in C++ mode will be accepted if the user's Clang version accepts them (typically as an extension with a warning), and rejected only if Clang rejects them. This matches the behavior of real-world build environments and avoids introducing Arcanum-specific acceptance criteria that diverge from the user's existing toolchain.

The ECSL contract parser similarly does not enforce host language mode restrictions on expressions within contracts. The contract expression language is defined by ECSL independently of the host language (see below), so constructs like compound literals may appear in contracts regardless of mode.

### `--std` Flag

The `--std` flag controls the `-std` option passed to Clang. It does not affect ECSL-specific contract parsing.

If `--std` is not specified, the mode is inferred from the file extension: `.c` → `c99`, `.cpp` / `.cc` / `.cxx` → `c++17`.

### ECSL Contract Expression Language

ECSL contract expressions (`requires`, `ensures`, etc.) are parsed by Arcanum's own parser, not by Clang. The accepted expression syntax within contracts is defined by the ECSL specification and is the same regardless of the host language mode. This ensures that ACSL-compatible contracts work identically whether the host file is C or C++.

The ECSL-specific extensions (ownership, lifetime, etc.) are available in both modes.

## Extensions

### Ownership

ACSL provides low-level heap predicates (`\valid`, `\freeable`, `\separated`) that can express memory safety properties, but it lacks first-class support for ownership semantics. In ACSL, ownership transfer and borrowing must be encoded indirectly through combinations of these predicates, which is verbose and error-prone.

ECSL introduces ownership as a first-class concept in the specification language.

#### Motivation

In safety-critical code, clear ownership of resources is essential for preventing use-after-free, double-free, and memory leaks. While ACSL can express the low-level conditions that correspond to correct ownership, the intent is implicit and the burden of correct encoding falls entirely on the specifier. ECSL makes ownership intent explicit and verifiable.

#### Constructs (Planned)

> **Note:** The concrete syntax for ownership constructs is under design. The following are representative examples subject to change.

- `\owns(p)` — The current function or scope holds unique ownership of the resource pointed to by `p`.
- `\borrows(p)` — The current function holds a temporary, non-owning reference to the resource pointed to by `p`.
- `\moves(p)` — Ownership of the resource pointed to by `p` is transferred from the caller to the callee (or vice versa).
- `\lifetime(p)` — Relates the validity of `p` to a lexical scope or named region.

#### Example (Illustrative)

```c
/*@ requires \owns(p);
    ensures  \owns(\result);
    moves    p;
*/
int *transform(int *p);

/*@ requires \borrows(data);
    assigns  \nothing;
    ensures  \result >= 0;
*/
int read_value(const int *data);
```

#### Relationship to ACSL Heap Predicates

Ownership constructs desugar to combinations of ACSL predicates for verification purposes. For example, `\owns(p)` implies `\valid(p) && \freeable(p)`, and `\moves(p)` implies that `\owns(p)` holds at the `Pre` state but not at the `Post` state. The exact desugaring rules will be defined as the ownership model is formalized.

---

## Versioning and Future Extensions

ECSL uses semantic versioning. Extensions are introduced in minor versions; breaking changes (if ever necessary) require a major version bump.

Planned areas for future extension include:

- Lifetime annotations
- Concurrency contracts
- Temporal properties
- Standard library specifications

## References

- [Arcanum Roadmap](./arcanum-roadmap.md)
- [Arcanum MVP Implementation Plan](./arcanum-mvp-plan.md)
- [Supported Source Language Subset](./arcanum-mvp-cpp-subset.md)
- [Future Design Considerations](./arcanum-future-considerations.md)
- ACSL specification: https://frama-c.com/html/acsl.html
- Frama-C: https://frama-c.com/
- ISO 26262: Road vehicles — Functional safety
