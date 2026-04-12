# ECSL C++ Design: Handling C++ Features

> **Date:** 2026-04-12
> **Status:** Draft
> **Related:** ECSL.md, arcanum-milestones.md, arcanum-syntax-tracker.xlsx
> **References:** ACSL v1.23, ACSL++ v0.1.1, P2900R6 (C++ Contracts)

## Overview

This document describes how ECSL handles C++ language features for formal verification. The design principle is **minimal extension** — reuse existing ACSL constructs wherever possible, and only introduce new ECSL syntax when C++ semantics cannot be expressed with ACSL.

**New ECSL constructs for C++:**
- `class invariant` (M4)
- `throws` clause (TBD, with exception support)

**Everything else** uses existing ACSL constructs applied to C++ functions, with the tool automatically generating proof obligations from C++ language rules.

### Design philosophy

ACSL++ v0.1.1 (Frama-C's C++ specification language) was studied extensively. Many of its features are well-designed and adopted here. However, ACSL++ marks most C++ features as Experimental and Frama-Clang barely implements them. Arcanum works directly on Clang's AST rather than lowering to C first, which is a fundamental advantage over Frama-Clang's approach.

The guiding questions for each C++ feature are:
1. Can the user express the contract using existing ACSL syntax?
2. Can the tool infer facts from the AST without user annotation?
3. Can the tool auto-generate proof obligations from C++ language rules?

If all three are yes, no new ECSL syntax is needed.

---

## 1. References

### Decision: No ECSL extension needed

References are modeled as pointers in the memory model. The tool desugars `T&` to `T*` internally. ACSL memory predicates (`\valid`, `\separated`, `\freeable`) apply directly.

### How it works

In contracts, reference parameters are used by name:

```cpp
/*@ requires x > 0;
    ensures \result == x * 2; */
int double_it(const int& x) { return x * 2; }
```

Reference validity uses `\valid`:

```cpp
/*@ requires \valid(&x);
    assigns x;
    ensures x == \old(x) + 1; */
void increment(int& x) { x++; }
```

Dangling references are caught the same way ACSL catches dangling pointers — `\valid(&r)` becomes unprovable after the referent is invalidated. The user annotates invalidating operations with `assigns` clauses, and WP tracks validity through the memory model.

### What the tool does automatically

- Desugars `T&` to `T*` internally
- Auto-generates `requires \valid(&x)` for reference parameters
- References can't be null — no null check needed (stronger than pointers)
- References can't be reseated — simplifies alias analysis

### What the user writes

- Normal `requires`/`ensures`/`assigns` using reference names directly
- `assigns` on functions that invalidate references (e.g., container operations)

---

## 2. Class Invariants

### Decision: New ECSL construct — `class invariant`

ACSL's `type invariant` is external to the type and has no inheritance semantics. C++ classes need invariants that live inside the class, inherit to derived classes, and are automatically checked at specific program points.

### Syntax

```cpp
class BoundedCounter {
    int value;
    int max_value;

    //@ class invariant counter_valid: 0 <= value <= max_value;

public:
    /*@ ensures value == 0;
        ensures max_value == m; */
    BoundedCounter(int m);

    /*@ ensures value == \old(value) + 1; */
    void increment();
};
```

### Semantics

The class invariant generates automatic proof obligations:
- **After each constructor completes**: invariant must hold
- **Before destructor entry**: invariant is assumed to hold
- **Before and after each public method**: invariant is assumed on entry, must be proved on exit
- **Private/protected methods**: no automatic invariant checks (internal state may temporarily violate invariant)

### Inheritance

If `Derived` extends `Base`, and both have class invariants:
- `Derived` invariant implicitly includes `Base` invariant
- At `Derived` constructor exit, both invariants must hold
- At any `Derived` public method exit, both invariants must hold

The tool auto-generates these compound proof obligations.

### Comparison with ACSL

| Aspect | ACSL `type invariant` | ECSL `class invariant` |
|--------|----------------------|----------------------|
| Placement | External to type | Inside class declaration |
| Inheritance | None | Derived includes base |
| Auto-checking | Global | Constructor/destructor/public methods |
| Virtual | No | Yes (dynamic dispatch possible) |
| Access | Can't see private members | Can see private members (same scope) |

---

## 3. Constructor and Destructor Contracts

### Decision: No ECSL extension needed

Constructors and destructors are functions. Users write normal `requires`/`ensures`/`assigns` contracts on them. The class invariant provides the automatic proof obligations.

### How it works

```cpp
class Socket {
    int fd;

    //@ class invariant valid_fd: fd >= 0;

public:
    /*@ ensures fd >= 0; */  // redundant — class invariant already generates this
    Socket(const char* host, int port);

    /*@ assigns fd; */
    ~Socket();
};
```

### What the tool does automatically

| Program point | Auto-generated obligation |
|--------------|--------------------------|
| Constructor exit | Class invariant must hold |
| Destructor entry | Class invariant assumed |
| Destructor exit | No invariant check (object destroyed) |
| Copy constructor exit | Class invariant must hold for new object |
| Move constructor exit | Class invariant must hold for new object |
| `= default` constructors | Tool verifies invariant holds for default initialization |

### What the user writes

- `ensures` for properties beyond the class invariant
- `assigns` for side effects (especially for destructors that release resources)
- `requires` for constructor preconditions (e.g., valid arguments)

---

## 4. Liskov Substitution (Virtual Methods)

### Decision: No ECSL extension needed

When a derived class overrides a virtual method, the tool automatically generates proof obligations for behavioral subtyping. Following ACSL++ v0.1.1 §2.7.5.

### How it works

```cpp
class Shape {
public:
    /*@ ensures \result >= 0; */
    virtual double area() const = 0;
};

class Circle : public Shape {
    double radius;
public:
    /*@ ensures \result == radius * radius * 3.14159; */
    double area() const override;
    // Auto-generated: prove that (radius * radius * 3.14159) >= 0
    // (derived postcondition implies base postcondition)
};
```

### Auto-generated proof obligations

For `Derived::m()` overriding `Base::m()`:
- **Precondition**: `Base::m` precondition must imply `Derived::m` precondition (derived can be more lenient)
- **Postcondition**: `Derived::m` postcondition must imply `Base::m` postcondition (derived can be more strict)

These are verification conditions, not syntax. The user writes contracts on both base and derived methods; the tool checks compatibility.

---

## 5. Exception Handling

### Decision: New ECSL construct — `throws` clause (TBD)

ACSL's abrupt termination clauses (`exits`, `breaks`, `continues`, `returns`) don't cover C++ exceptions. If exception support is added, ECSL needs a `throws` clause.

### Proposed syntax (following ACSL++ v0.1.1 §2.17)

```cpp
/*@ ensures \result > 0;
    throws \exception != \null; */
int parse(const char* s);
```

`\exception` is a pointer to the exception object (`const void*`). To inspect the exception type, use `static_cast`:

```cpp
/*@ throws static_cast<const std::runtime_error*>(\exception)->what() != \null; */
void risky_operation();
```

### `noexcept` verification

If a function is declared `noexcept`, the tool automatically generates a proof obligation that no exception can be thrown. No user annotation needed.

```cpp
void safe_op() noexcept;
// Auto-generated: prove that no execution path throws
```

### `noexcept` and `terminates`

The C++ `noexcept` keyword is a compiler optimization hint — it tells the compiler that if an exception is thrown, `std::terminate` should be called. ECSL does not generate any proof obligations from `noexcept`. It is not a specification construct.

If the user wants to prove a function never throws, they use the existing ACSL `terminates` clause:

```cpp
/*@ terminates \true;
    ensures \result == a + b; */
int safe_add(int a, int b) noexcept { return a + b; }
```

`terminates \true` proves the function always returns normally — no exceptions, no `exit()`, no divergence. This is stronger than `noexcept` (which allows `std::terminate`).

| What the user writes | Meaning |
|---|---|
| `terminates \true` | Function always returns normally — no throw, no exit, no divergence |
| `throws P` | If function exits via exception, predicate P holds |
| Both | Function either returns normally or throws with P |
| Neither | No termination or exception guarantees specified |

### Status

Exception support is TBD. The `throws` clause and `\exception` keyword will only be implemented if exceptions are supported (M6 decision).

---

## 6. Move Semantics

### Decision: No ECSL extension needed

Move constructors and move assignment operators are just functions with `T&&` parameters. The user writes normal contracts.

### How it works

```cpp
class Buffer {
    int* data;
    int size;

    //@ class invariant valid: size >= 0 && (size > 0 ==> \valid(data + (0 .. size-1)));

public:
    /*@ assigns \nothing;
        ensures data == \old(other.data);
        ensures size == \old(other.size);
        ensures other.data == \null;
        ensures other.size == 0; */
    Buffer(Buffer&& other) noexcept;
};
```

The user explicitly specifies the moved-from state (`other.data == \null`, `other.size == 0`). This is the contract — the tool proves it.

### What the tool does automatically

- Detects move constructor/assignment from Clang AST (`CXXConstructExpr` with rvalue reference)
- Class invariant checked for the newly constructed object
- Class invariant checked for the moved-from object (it must still satisfy the invariant in its new state)

### What the user writes

- `ensures` specifying both the new object state and the moved-from object state
- The moved-from object must still satisfy the class invariant (e.g., `size == 0 && data == \null` satisfies `size >= 0 && (size > 0 ==> ...)`)

---

## 7. Templates

### Decision: No ECSL extension needed

Arcanum only verifies instantiated template code. Template-dependent AST nodes are resolved by Clang's Sema — Clang substitutes concrete types before Arcanum sees the code. Following ACSL++ v0.1.1 §2.9.

### How it works

```cpp
template <typename T>
/*@ requires n > 0;
    ensures \result >= a[0]; */
T max_element(T* a, int n);
```

When instantiated as `max_element<int>`, the contract uses `T = int`. The tool verifies each instantiation separately.

### Behavior-based specs for type-dependent properties

Following ACSL++ v0.1.1 §2.9.3:

```cpp
template <typename M>
/*@ behavior is_int:
      assumes std::is_same<M, int>::value;
      ensures \result == (a > b ? a : b); */
M larger(M a, M b) { return a > b ? a : b; }
```

---

## 8. Lambda Expressions

### Decision: No ECSL extension needed

Lambda contracts are placed before the opening brace. Captures are inferred from the AST. Following ACSL++ v0.1.1 §2.19.3.

### How it works

```cpp
auto doubler = [](int x) /*@ ensures \result == x * 2; */ { return x * 2; };

int factor = 3;
auto multiplier = [factor](int x) /*@ ensures \result == factor * x; */ {
    return factor * x;
};
```

### What the tool does automatically

- Infers captures and capture modes (by value, by reference) from Clang AST
- For by-reference captures, auto-generates `\valid` for the captured reference
- Lambda body is verified like any function body with the contract

---

## 9. Operator Overloading

### Decision: No ECSL extension needed

Operator overloads are functions. Contracts apply normally. Following ACSL++ v0.1.1 §2.7.6.

```cpp
class Vec2 {
    double x, y;
public:
    /*@ ensures \result.x == this->x + other.x;
        ensures \result.y == this->y + other.y; */
    Vec2 operator+(const Vec2& other) const;
};
```

---

## 10. `this` in Contracts

### Decision: No ECSL extension needed

`this` is available in member function contracts as a pointer to the current object. Following ACSL++ v0.1.1 §2.7.3.

```cpp
class Stack {
    int* data;
    int top;
public:
    /*@ requires this->top > 0;
        assigns this->top;
        ensures this->top == \old(this->top) - 1; */
    int pop();
};
```

---

---

## 11. Namespaces

### Decision: No ECSL extension needed

Logic declarations can appear in namespace scope. Qualified names use C++ syntax. Following ACSL++ v0.1.1 §2.13.

```cpp
namespace math {
    //@ logic real pi = 3.14159265358979;

    /*@ ensures \result == 2 * math::pi * r; */
    double circumference(double r);
}
```

---

## 12. Access Control

### Decision: `spec_public` not needed

Arcanum has full source access, so all members are visible in specifications regardless of C++ access level. ACSL++ `spec_public`/`spec_protected` are unnecessary.

---

## Summary

| C++ Feature | New ECSL syntax? | Milestone | Approach |
|-------------|:-:|:-:|-----------|
| References | No | M5 | Pointers in memory model; `\valid(&x)` |
| Class invariants | **Yes** | M4 | `class invariant` with inheritance |
| Constructor/destructor | No | M4 | Normal contracts + auto proof obligations |
| Liskov substitution | No | M4 | Auto proof obligations on override |
| Exceptions | **Yes (TBD)** | M6 | `throws` clause + `\exception` |
| Move semantics | No | M5 | Normal `ensures` on move ctor |
| Templates | No | M4 | Verify instantiated code only |
| Lambdas | No | M6 | Contract before `{`, captures from AST |
| Operator overloading | No | M4 | Normal function contracts |
| `this` | No | M4 | Available as variable in contracts |
| Namespaces | No | M4 | Qualified names in contracts |
| Access control | No | — | Full source access, not needed |
| Ownership | No | — | Removed; ACSL `\valid`/`\separated`/`\freeable` suffices |

---

## Appendix A. ECSL Parser Architecture

### Decision: Recursive descent + Clang expression delegation

The ECSL parser is a hand-written recursive descent parser that handles specification-level grammar (keywords, quantifiers, ECSL predicates) and delegates C/C++ expression fragments to Clang's `Parser::ParseExpression()` with full `Sema` context.

### Why this approach

- **ECSL grammar is small** — ~50-100 productions for specification keywords. Flex/bison overhead is not justified.
- **C/C++ expressions inside contracts are complex** — `this->data[i] + 1`, template-dependent expressions, overloaded operators. Reimplementing C++ expression parsing is impractical.
- **Clang already handles name resolution, type checking, and overload resolution.** Reusing it gives correct semantics for free.
- **ACSL++ / Frama-Clang takes a similar approach** — hand-written parser integrated with Clang.

### Architecture

```
Source file with /*@ ... */ annotations
  │
  ▼
Clang Lexer/Parser (normal C/C++ parsing)
  │
  ├──→ C/C++ AST (Clang's normal output)
  │
  └──→ Annotation comments with source locations
         │
         ▼
       ECSL Parser (our code)
         │
         ├── Tokenizes annotation text
         ├── Parses ECSL keywords: requires, ensures, assigns,
         │   loop invariant, class invariant, throws, ...
         ├── Parses ECSL operators: ==>, <==>, \forall, \exists, ...
         ├── Parses ECSL predicates: \valid, \separated, \old, ...
         │
         └── For C/C++ expression fragments:
               │
               ▼
             Clang Parser::ParseExpression()
               │  (with current Sema context, DeclContext)
               │  (name resolution, type checking, overload resolution)
               │
               ▼
             Clang Expr* (fully typed AST node)
               │
               ▼
             Wrapped into ECSL annotation node
  │
  ▼
Arcanum Core
  │  Walks Clang AST + ECSL annotations
  │  Runs verification passes (WP, test gen)
  ▼
Verification conditions / Test code
```

### How expression delegation works

The ECSL parser handles the specification grammar and identifies where C/C++ expressions appear. At each expression position, it extracts the expression token range and delegates to Clang's parser:

```
ECSL annotation: "requires this->size > 0 && \valid(this->data + (0 .. this->size-1))"

ECSL parser tokenizes:
  [requires] [EXPR: this->size > 0] [&&] [\valid] [(] [EXPR: this->data + (0 .. this->size-1)] [)]

For "this->size > 0":
  → Push tokens into Clang's parser
  → Clang resolves 'this' in current method's DeclContext
  → Clang resolves 'size' as FieldDecl
  → Clang type-checks '>' as comparison
  → Returns: BinaryOperator(MemberExpr(CXXThisExpr, size), IntegerLiteral(0), BO_GT)

For "this->data + (0 .. this->size-1)":
  → ECSL parser recognizes ".." as range operator (not C++)
  → Splits into: Clang parses "this->data", "0", "this->size-1" separately
  → ECSL parser builds the range expression from the parts
```

### What the ECSL parser handles (not delegated to Clang)

| Token/construct | ECSL parser action |
|---|---|
| `requires`, `ensures`, `assigns`, `loop invariant`, `class invariant`, `throws` | Clause keyword → new clause node |
| `\forall`, `\exists` | Quantifier → parse binders, then delegate body to Clang |
| `\valid`, `\valid_read`, `\separated`, `\freeable`, `\allocable` | Built-in predicate → delegate argument expressions to Clang |
| `\old(e)`, `\at(e, L)`, `\result` | Built-in term → delegate inner expression to Clang |
| `\exception` | Exception object reference (in `throws` clause) |
| `==>`, `<==>`, `^^` | Logic connectives → parse left/right operands |
| `..` (range) | Range operator → parse bounds |
| `\nothing`, `\everything` | Location constants |
| `\let x = e1; e2` | Local binding |
| `\lambda` | Logic lambda |
| Behavior blocks (`behavior`, `assumes`, `complete behaviors`) | Clause grouping |
| `check`, `admit` modifiers | Clause modifier |

### What Clang handles (delegated)

Everything that is a valid C/C++ expression:
- Arithmetic, comparison, logical operators
- Variable references, member access (`this->x`, `obj.field`)
- Function calls, operator calls
- Array subscript, pointer dereference
- Casts (`static_cast<T>(x)`)
- Template-dependent expressions (in instantiated context)
- Type expressions (for `sizeof`, casts, quantifier binders)

### Integration point with Clang

The ECSL parser needs access to:
- **`clang::Sema`** — for name lookup, type checking, overload resolution
- **`clang::DeclContext`** — the scope in which the annotation appears (function, class, namespace)
- **`clang::Preprocessor`** — to create token buffers and feed them to the parser

This is implemented as a Clang plugin (`ASTConsumer` + `PPCallbacks`) that:
1. Registers a comment handler to intercept `/*@ ... */` and `//@ ...`
2. Associates annotations with AST nodes by source location
3. Provides `Sema` access to the ECSL parser for expression delegation

### Token boundary detection

The main implementation challenge is determining where a C/C++ expression ends and an ECSL construct begins. For example:

```
requires x + 1 > 0 && \valid(p);
```

The ECSL parser needs to know that `x + 1 > 0` is a C expression, `&&` could be either C or ECSL (in this case C logical AND), and `\valid(p)` is an ECSL predicate.

The rule: **tokens starting with `\` are always ECSL.** Everything else is first attempted as a C expression via Clang. If Clang's parser fails (e.g., encounters `==>` which isn't valid C), the ECSL parser takes over.

In practice, the ECSL parser drives the parse and calls Clang for subexpressions at well-defined points — after `requires`, as operands of `==>`, as arguments to `\valid`, etc. The boundary is always determined by ECSL grammar, not by trial-and-error.
