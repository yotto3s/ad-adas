# Arcanum Milestones

> **Date:** 2026-04-12
>
> **Tracks:** arcanum-syntax-tracker.xlsx

## Overview

Arcanum development is organized into 6 milestones. Each milestone delivers an end-to-end capability increment — new language features, new ECSL specification constructs, and the verification/test-generation passes to support them.

| Milestone | Theme | Scope |
|-----------|-------|-------|
| M1 | Pure Functions | Scalar C, basic ECSL contracts → test gen + WhyML |
| M2 | Full C + ECSL Predicates | Loops, structs, arrays, pointers, float, bitwise, ECSL memory predicates |
| M3 | Frama-C WP Equivalent | WP engine, memory model, all ACSL constructs |
| M4 | Basic C++ | Classes, methods, namespaces, OOP basics |
| M5 | C++ References + Casts + Memory | References, move, casts, new/delete |
| M6 | Advanced C++ | Lambdas, exceptions (TBD) |

---

## M1: Pure Functions

**Theme:** Minimal end-to-end pipeline for side-effect-free scalar functions.

**Goal:** Given a C/C++ pure function with ECSL contracts (`requires`, `ensures`, `assigns \nothing`), generate boundary-value Google Test code and WhyML for Why3 verification.

**Completion criteria:**
- `arcanum testgen` and `arcanum verify` work E2E on a single pure function
- Contract violation causes both test failure and verification failure

### Features (49)

**ECSL (11):**
- `requires`, `ensures`, `assigns \nothing`, `\result`, `\nothing`
- Predicate expressions: `==`, `!=`, `<`, `<=`, `>`, `>=`, `&&`, `||`, `!`, `+`, `-`, `*`, `/`, `%`
- Logic types: `integer`, `boolean`, integer promotion

**C Syntax (37):**
- Scalar types: `bool`, `char`, `int`, `short`, `long`, `long long`, `unsigned` variants
- Declarations: function definition/declaration, parameters, local variables
- Statements: compound, declaration, expression, null, `if`/`else`, `return`
- Expressions: arithmetic (`+`, `-`, `*`, `/`, `%`, unary `-`), comparison, logical, ternary `?:`, simple assignment `=`, integer/character literals, variable reference
- Types: `FunctionProtoType`

**C++ (1):**
- `CXXBoolLiteralExpr` (true/false literals)

### Implementation components
- M0: Build infrastructure, Clang plugin setup
- M1: ECSL parser + annotation store
- M2: WP engine (Clang AST walker + predicate transformer)
- M3: WhyML emitter + Why3 integration
- M4: Boundary value test generator
- M5: Google Test code generator
- M6: CLI integration (`arcanum testgen`, `arcanum verify`)

---

## M2: Full C + ECSL Predicates

**Theme:** Support real-world C codebases — loops, composite types, pointers, floating point, bitwise.

**Goal:** Arcanum can process typical MISRA C:2025 compliant automotive C code. ECSL pointer predicates (`\valid`, `\separated`, etc.) enable memory safety verification.

**Completion criteria:**
- Loops are verified with `loop invariant` / `loop variant` annotations
- Struct/array/pointer types are supported in both test generation and WhyML
- ACSL pointer predicates verify memory safety properties
- Float/double are handled (mathematical real in WhyML, with BV theory option)

### Features (73)

**ECSL (29):**
- Frame conditions: `assigns <locations>`, `frees`, `allocates`, `\old(e)`
- Loop annotations: `loop invariant`, `loop assigns`, `loop variant`, `decreases`
- Pointer predicates: `\valid`, `\valid_read`, `\valid(p + (a..b))`, `\separated`, `\freeable`, `\allocable`
- Memory locations: `\everything`, `t1 .. t2`
- Assertions: `assert`
- Logic types: `real`, `Enum in logic`, integer casts
- Float: `\round_float`/`\round_double`, `rounding_mode`, `\is_finite`, `\is_NaN`, `\is_infinite`, `\sign`, `\exact`/`\round_error`, math builtins

**C Syntax (44):**
- Control flow: `for`, `while`, `do-while`, `break`/`continue`, `switch`/`case`, function calls
- Composite types: `struct`, `union`, `enum`, `typedef`, anonymous struct/union, field declarations
- Arrays: `[]` subscript, `ConstantArrayType`, `IncompleteArrayType`
- Pointers: `*` (type/deref), `&` (address-of), `->`, `malloc`/`free`
- Float: `float`/`double` types, floating literals
- Bitwise: `&`, `|`, `^`, `~`, `<<`, `>>`
- Other: compound assignment (`+=` etc.), `++`/`--`, string literals, comma operator, C-style cast, `sizeof`/`alignof`, variadic args, `va_arg`, compound literals, designated initializers, `_Alignas`, `_Noreturn`, `extern "C"`

---

## M3: Frama-C WP Equivalent

**Theme:** Achieve feature parity with Frama-C's WP plugin for deductive verification.

**Goal:** All ACSL v1.23 constructs are supported. The WP computation engine, memory model (Hoare/Typed/mixed), and modular verification enable proving the ACSL by Example benchmark suite.

**Completion criteria:**
- ACSL by Example main algorithms (equal, find, sort, etc.) verified with Arcanum
- Quantifiers, logic definitions, ghost code, behaviors all functional
- Modular verification: callee contracts used as assumptions
- Property status tracking: each property has Proved / Not Proved / Test Covered status

### Features (76)

All remaining ACSL v1.23 constructs:
- Behaviors: `behavior`, `assumes`, `complete`/`disjoint behaviors`
- Clause modifiers: `check`/`admit` for `requires`/`ensures`/`assert`
- Statement annotations: statement contracts, `loop allocates`/`loop frees`, general loop variants
- Program point labels: `\at(e, L)`, `Pre`/`Post`/`Old`/`Here`/`LoopEntry`/`LoopCurrent`
- Termination: `terminates`
- Abrupt termination: `exits`/`breaks`/`continues`/`returns` clauses
- Advanced predicates: `==>`, `<==>`, `^^`, consecutive comparisons, syntactic naming
- Quantifiers: `\forall`, `\exists`
- Logic definitions: functions, predicates, lemmas, axiomatics, inductive predicates, polymorphic types, recursive definitions, concrete types (ADTs), `reads` clause, specification modules
- Higher-order: `\lambda`, `\sum`, `\product`, `\max`/`\min`, `\numof`, `\let`
- Memory: `\base_addr`, `\offset`, `\block_length`, `\allocation`, `\dangling`, `\object_pointer`, `\initialized`, `\specified`
- Sets/lists: comprehension, `\union`/`\inter`, `\empty`, `\subset`, `\Nil`/`\Cons`/`\nth`/`\length`/`\repeat`/`\concat`
- Functional update: `{ s \with .field = v }`, `{ t \with [i] = v }`
- Bitwise in logic: `&`, `|`, `^`, `~`, `<<`, `>>`, `-->`, `<-->`
- Data invariants: global invariant, type invariant, model field
- Ghost code: ghost variables, ghost statements, volatile specification

---

## M4: Basic C++

**Theme:** C++ class-based OOP support — the foundation for C++ verification.

**Goal:** Arcanum can process C++ code with classes, methods, constructors/destructors, namespaces, and basic C++17 features.

**Completion criteria:**
- C++ classes with methods verified (contracts on methods)
- Constructors/destructors handled in control flow
- Namespaces resolved
- Range-based for loops converted like C loops

### Features (20)

**C++ Syntax:**
- Declarations: `CXXMethodDecl`, `CXXConstructorDecl`, `CXXDestructorDecl`, `CXXConversionDecl`, `NamespaceDecl`
- Types: `class` (`CXXRecordDecl`), `enum class`, `DecompositionDecl` (structured bindings)
- Statements: `CXXForRangeStmt` (range-based for)
- Expressions: `this`, `CXXMemberCallExpr`, `CXXOperatorCallExpr`, `CXXConstructExpr`, `CXXTemporaryObjectExpr`, `CXXScalarValueInitExpr`, `CXXStdInitializerListExpr`, `typeid`
- Literals: `CXXBoolLiteralExpr`, `CXXNullPtrLiteralExpr`, `UserDefinedLiteral`

---

## M5: C++ References + Casts + Memory

**Theme:** C++ reference semantics, type casting, and heap memory management.

**Goal:** Full C++ pointer/reference model. Reference parameters, all named casts, and `new`/`delete` are supported in verification and test generation.

**Completion criteria:**
- Reference parameters (`const T&`, `T&&`) correctly modeled in WP engine and WhyML
- `new`/`delete` tracked with `\allocable`/`\freeable` from M2
- All named casts (`static_cast`, `dynamic_cast`, `const_cast`, `reinterpret_cast`) supported
- Move semantics (`T&&`) handled in control flow

### Features (12)

**C++ Syntax (12):**
- References: `LValueReferenceType` (`const T&`), `RValueReferenceType` (`T&&`), `MemberPointerType`
- Casts: `static_cast`, `dynamic_cast`, `const_cast`, `reinterpret_cast`, `CXXFunctionalCastExpr`
- Memory: `CXXNewExpr`, `CXXDeleteExpr`, smart pointers
- Other: `CXXInheritedCtorInitExpr`

---

## M6: Advanced C++

**Theme:** Advanced C++ features — lambdas and potentially exceptions.

**Goal:** Lambda support with capture semantics modeled for verification. Exception support based on TBD decision.

**Completion criteria:**
- Lambdas can be verified (capture semantics modeled)
- Exception handling support (if decided)

### Features (1+)

**C++ Syntax (1+):**
- `LambdaExpr` — anonymous functions with captures
- Exception handling (`CXXTryStmt`, `CXXCatchStmt`, `CXXThrowExpr`) — TBD

---

## TBD Features (not assigned to milestones)

These features have undecided support status:

**C Syntax:**
- `_Thread_local` storage (C11 threading)
- `_Atomic` type / `AtomicExpr` (C11 atomics)
- `_Complex` type / imaginary literals (C99 complex numbers)

**C++ Syntax:**
- `CXXPseudoDestructorExpr` (pseudo-destructor call)
- Exception handling (`try`/`catch`/`throw`) — may be added to M6



---

## Epic ↔ Milestone Mapping

| Epic | M1 | M2 | M3 | M4 | M5 | M6 |
|------|:--:|:--:|:--:|:--:|:--:|:--:|
| Scalar Types & Arithmetic | ● | | | | | |
| Control Flow | ● | ● | | | | |
| Structs & Enums | | ● | | | | |
| Arrays | | ● | | | | |
| Pointers & Memory | | ● | | | | |
| Floating Point | | ● | | | | |
| Bitwise Operations | | ● | | | | |
| ECSL Core Contracts | ● | ● | | | | |
| ECSL Pointer Predicates | | ● | | | | |
| ECSL Advanced (WP) | | | ● | | | |
| C++ Classes & Methods | | | | ● | | |
| C++ References & Move | | | | | ● | |
| C++ Casts & Memory | | | | | ● | |
| C++ Lambdas | | | | | | ● |

## References

- [Arcanum Roadmap](./arcanum-roadmap.md)
- [MVP Implementation Plan](./arcanum-mvp-plan.md)
- [ECSL Specification](./ECSL.md)
- [Supported Source Language Subset](./arcanum-mvp-cpp-subset.md)
- [Future Design Considerations](./arcanum-future-considerations.md)
- ACSL v1.23: https://frama-c.com/download/acsl.pdf
- LLVM/Clang 22.1.1: https://github.com/llvm/llvm-project/releases/tag/llvmorg-22.1.1
