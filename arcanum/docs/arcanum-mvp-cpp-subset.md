# Arcanum MVP: Supported Source Language Subset

> **Status:** Draft — v0.1.0
>
> **Date:** 2026-04-03

## Overview

Arcanum MVPが入力として受け付けるソース言語のサブセットを定義する。本ドキュメントでは、対応する構文のBNF文法と、それに対応するClang ASTノードを明示する。

MVPでは副作用なし・ポインタなしの純粋関数のみを対象とする。さらに、ヘッダファイル（`.h` / `.hpp`）で宣言されている関数のみがテスト生成・形式検証の対象となる。ヘッダに宣言がない関数（static関数、内部実装関数等）は対象外。

## Language Modes

ArcanumはC99とC++の両方のソースコードを入力として受け付ける。`--std` フラグまたはファイル拡張子によってモードが決定され、Clangフロントエンドに `-std` オプションとして渡される。

| モード | `--std` 値 | 拡張子 | Clangオプション |
|--------|-----------|--------|----------------|
| C99 | `c99` | `.c` | `-std=c99` |
| C++17 | `c++17` | `.cpp`, `.cc`, `.cxx` | `-std=c++17` |

### バリデーション方針

Arcanumは独自の言語バリデーションを行わない。ホスト言語の構文・意味の検査はすべてClangに委ねる。Clangがwarning付きで受け入れる構文（例: C++モードでのcompound literal）はArcanumも受け入れ、Clangがエラーとする構文はArcanumもエラーとして報告する。

これにより、ユーザーの既存ビルド環境と挙動が一致し、Arcanum固有の受理基準による混乱を避ける。

### モード別の構文差異（将来の拡張参考）

MVPのサポート構文はどちらのモードでもほぼ同一（スカラー型の純粋関数）である。以下はMVPスコープ外だが、将来の拡張時にClangのAST上でモード依存の差異が現れる構文を参考として列挙する。Arcanumがこれらを受け入れるかどうかはClangの判断に従う。

**C99由来の構文:**

| 構文 | 例 | Clang AST Node | 備考 |
|------|-----|----------------|------|
| Compound literal | `(int){42}` | `CompoundLiteralExpr` | C++モードでもClang拡張として受理されることが多い |
| Designated initializer (配列) | `{[0] = 1, [2] = 3}` | `DesignatedInitExpr` | C++20で構造体のみ標準化。配列はClang拡張 |
| `restrict` ポインタ | `int * restrict p` | `QualType` with `restrict` | C++では `__restrict` として拡張サポート |
| `_Bool` 型 | `_Bool flag` | `BuiltinType::Bool` | C++では `bool`。Clangは両方受理 |

**C++固有の構文:**

| 構文 | 例 | Clang AST Node | 備考 |
|------|-----|----------------|------|
| 参照型 | `const int &x` | `ReferenceType` | C99モードではエラー |
| `constexpr` | `constexpr int f()` | `FunctionDecl` with `isConstexpr()` | C99モードではエラー |
| `enum class` | `enum class Color { R, G, B }` | `EnumDecl` with `isScoped()` | C99モードではエラー |
| `auto` 戻り値型 | `auto f() -> int` | `AutoType` | C99モードではエラー |

### テスト出力への影響

生成されるテストコードは常にC++（Google Test）である。C99モードで入力された場合、CodeGenは対象関数のヘッダを `extern "C"` ブロック内で include する:

```cpp
// C99入力の場合
extern "C" {
#include "compute.h"
}

// C++入力の場合
#include "compute.h"
```

## BNF Grammar

### Top-Level

```bnf
translation-unit ::= { ecsl-annotated-function }*

ecsl-annotated-function ::= ecsl-contract function-definition
                          | ecsl-contract function-declaration

function-declaration ::= type-specifier declarator '(' parameter-list ')' ';'

function-definition ::= type-specifier declarator '(' parameter-list ')' compound-statement
```

### ECSL Contract (コメント内)

```bnf
ecsl-contract ::= '/*@' { contract-clause }+ '*/'

contract-clause ::= requires-clause
                  | ensures-clause
                  | assigns-clause

requires-clause ::= 'requires' predicate ';'
ensures-clause  ::= 'ensures' predicate ';'
assigns-clause  ::= 'assigns' '\nothing' ';'
```

### Predicate (ECSL式)

```bnf
predicate ::= predicate '&&' predicate
            | predicate '||' predicate
            | '!' predicate
            | '(' predicate ')'
            | comparison

comparison ::= expression rel-op expression

rel-op ::= '==' | '!=' | '<' | '<=' | '>' | '>='

expression ::= expression add-op expression
             | expression mul-op expression
             | unary-expression

add-op ::= '+' | '-'
mul-op ::= '*' | '/' | '%'

unary-expression ::= '-' unary-expression
                   | primary-expression

primary-expression ::= identifier
                     | integer-literal
                     | '\result'
                     | '(' expression ')'
```

### Types

```bnf
type-specifier ::= 'int'
                 | 'unsigned' 'int'
                 | 'unsigned'
                 | 'bool'
                 | 'char'
                 | 'signed' 'char'
                 | 'unsigned' 'char'
                 | 'short'
                 | 'long'
                 | 'long' 'long'
                 | 'unsigned' 'short'
                 | 'unsigned' 'long'
                 | 'unsigned' 'long' 'long'
```

### Parameters

```bnf
parameter-list ::= ε
                 | parameter { ',' parameter }*

parameter ::= type-specifier declarator

declarator ::= identifier
```

### Statements (関数本体)

```bnf
compound-statement ::= '{' { statement }* '}'

statement ::= declaration-statement
            | expression-statement
            | return-statement
            | if-statement
            | compound-statement

declaration-statement ::= type-specifier declarator [ '=' expression ] ';'

expression-statement ::= expression ';'

return-statement ::= 'return' expression ';'

if-statement ::= 'if' '(' expression ')' statement
               | 'if' '(' expression ')' statement 'else' statement
```

### Expressions (関数本体内)

```bnf
expression ::= assignment-expression
             | expression ',' expression

assignment-expression ::= identifier '=' expression
                        | conditional-expression

conditional-expression ::= logical-or-expression
                         | logical-or-expression '?' expression ':' expression

logical-or-expression  ::= logical-and-expression { '||' logical-and-expression }*
logical-and-expression ::= equality-expression { '&&' equality-expression }*
equality-expression    ::= relational-expression { ( '==' | '!=' ) relational-expression }*
relational-expression  ::= additive-expression { ( '<' | '<=' | '>' | '>=' ) additive-expression }*
additive-expression    ::= multiplicative-expression { ( '+' | '-' ) multiplicative-expression }*
multiplicative-expression ::= unary-expression { ( '*' | '/' | '%' ) unary-expression }*

unary-expression ::= '-' unary-expression
                   | '!' unary-expression
                   | primary-expression

primary-expression ::= identifier
                     | integer-literal
                     | bool-literal
                     | char-literal
                     | '(' expression ')'

bool-literal ::= 'true' | 'false'
integer-literal ::= digit { digit }*
char-literal ::= '\'' character '\''
```

## Explicitly Excluded (MVP)

以下の構文はMVPで明示的に非サポートとする:

| カテゴリ | 除外される構文 |
|----------|----------------|
| ポインタ・参照 | `*`, `&`, `->`, `[]`（配列添字） |
| 構造体・クラス | `struct`, `class`, `union`, メンバアクセス`.` |
| テンプレート | `template`, 型パラメータ |
| 動的メモリ | `new`, `delete`, `malloc`, `free` |
| ループ | `for`, `while`, `do-while` |
| switch | `switch`, `case`, `default` |
| 列挙型 | `enum`, `enum class` |
| typedef / using | `typedef`, `using` |
| キャスト | `static_cast`, `reinterpret_cast`, Cスタイルキャスト |
| ビット演算 | `&`, `|`, `^`, `~`, `<<`, `>>` |
| インクリメント | `++`, `--` |
| 複合代入 | `+=`, `-=`, `*=`, `/=`, `%=` |
| 標準ライブラリ | `#include`（テスト対象のヘッダ以外） |
| プリプロセッサ | `#define`, `#ifdef`等（ECSL契約を除く） |
| 可変長引数 | `...` |
| 例外 | `try`, `catch`, `throw` |
| 名前空間 | `namespace` |

## Clang AST Mapping

MVPでサポートする各構文要素と、対応するClang ASTノードの対応表。パーサー（ClangBridge）はこれらのノードのみを走査する。

### Declarations

| 構文 | Clang AST Node | 備考 |
|------|----------------|------|
| 関数定義 | `FunctionDecl` | `hasBody()` が true |
| 関数宣言 | `FunctionDecl` | `hasBody()` が false |
| パラメータ | `ParmVarDecl` | `FunctionDecl::parameters()` で取得 |
| ローカル変数宣言 | `VarDecl` | `isLocalVarDecl()` が true |
| 戻り値型 | `QualType` | `FunctionDecl::getReturnType()` |

### Types

| 構文 | Clang AST Type | MLIR型への変換 |
|------|----------------|----------------|
| `int` | `BuiltinType::Int` | `i32` |
| `unsigned int` | `BuiltinType::UInt` | `ui32`（またはi32 + unsigned attr） |
| `bool` | `BuiltinType::Bool` | `i1` |
| `char` | `BuiltinType::Char_S` / `Char_U` | `i8` |
| `signed char` | `BuiltinType::SChar` | `si8` |
| `unsigned char` | `BuiltinType::UChar` | `ui8` |
| `short` | `BuiltinType::Short` | `i16` |
| `long` | `BuiltinType::Long` | `i64`（プラットフォーム依存） |
| `long long` | `BuiltinType::LongLong` | `i64` |
| `unsigned short` | `BuiltinType::UShort` | `ui16` |
| `unsigned long` | `BuiltinType::ULong` | `ui64` |
| `unsigned long long` | `BuiltinType::ULongLong` | `ui64` |

### Statements

| 構文 | Clang AST Node | 備考 |
|------|----------------|------|
| `{ ... }` | `CompoundStmt` | |
| `return expr;` | `ReturnStmt` | `getRetValue()` で式取得 |
| `type x = expr;` | `DeclStmt` → `VarDecl` | `getInit()` で初期化式 |
| `if (...) ...` | `IfStmt` | `getCond()`, `getThen()`, `getElse()` |
| `expr;` | `ExprStmt` | MVP内では代入式のみ想定 |

### Expressions

| 構文 | Clang AST Node | 備考 |
|------|----------------|------|
| `x + y` | `BinaryOperator` | `getOpcode()` == `BO_Add` |
| `x - y` | `BinaryOperator` | `BO_Sub` |
| `x * y` | `BinaryOperator` | `BO_Mul` |
| `x / y` | `BinaryOperator` | `BO_Div` |
| `x % y` | `BinaryOperator` | `BO_Rem` |
| `x == y` | `BinaryOperator` | `BO_EQ` |
| `x != y` | `BinaryOperator` | `BO_NE` |
| `x < y` | `BinaryOperator` | `BO_LT` |
| `x <= y` | `BinaryOperator` | `BO_LE` |
| `x > y` | `BinaryOperator` | `BO_GT` |
| `x >= y` | `BinaryOperator` | `BO_GE` |
| `x && y` | `BinaryOperator` | `BO_LAnd` |
| `x \|\| y` | `BinaryOperator` | `BO_LOr` |
| `x = y` | `BinaryOperator` | `BO_Assign` |
| `-x` | `UnaryOperator` | `UO_Minus` |
| `!x` | `UnaryOperator` | `UO_LNot` |
| `x ? a : b` | `ConditionalOperator` | `getCond()`, `getTrueExpr()`, `getFalseExpr()` |
| `x` (変数参照) | `DeclRefExpr` | `getDecl()` で `VarDecl`/`ParmVarDecl` 取得 |
| `42` | `IntegerLiteral` | `getValue()` で `APInt` 取得 |
| `true` / `false` | `CXXBoolLiteralExpr` | |
| `'a'` | `CharacterLiteral` | |
| `(expr)` | `ParenExpr` | `getSubExpr()` で内部式取得 |

### Implicit Nodes

Clang ASTにはソースコードに直接対応しない暗黙ノードが挿入される。パーサーはこれらを透過的に処理する必要がある:

| Clang AST Node | 発生条件 | 処理方針 |
|----------------|----------|----------|
| `ImplicitCastExpr` | 型の暗黙変換（`int` → `long` 等） | 子ノードを再帰的に処理。変換先の型を記録 |
| `ExprWithCleanups` | 一時オブジェクトのクリーンアップ | MVP範囲では発生しないはず。子ノードを透過 |
| `MaterializeTemporaryExpr` | 一時オブジェクトの実体化 | 同上 |

## ECSL Contract Extraction

ECSL契約はClangのASTには現れない（コメントとして扱われる）。抽出方針:

1. `clang::ASTContext::getRawCommentForDeclNoCache()` で関数に関連付けられたコメントを取得する
2. コメントテキストから `/*@ ... */` パターンを検出する
3. 独自のECSLパーサーで契約構文を解析する
4. 解析結果を、Clangから取得した `FunctionDecl` の情報（パラメータ名・型）と結合する

**注意:** Clangの `RawComment` はデフォルトでは `/**` や `///` スタイルのコメントのみ収集する。`/*@` を認識させるには、`CommentOptions` の設定またはソースバッファからの直接抽出が必要になる可能性がある。M1で検証する。

## Example

### Input

```cpp
/*@ requires 0 <= x && x <= 100;
    requires y > 0;
    ensures \result >= x;
    assigns \nothing;
*/
int compute(int x, int y) {
    if (x > 50) {
        return x + y;
    }
    return x;
}
```

### Clang AST (概略)

```
FunctionDecl 'compute' 'int (int, int)'
├── ParmVarDecl 'x' 'int'
├── ParmVarDecl 'y' 'int'
└── CompoundStmt
    ├── IfStmt
    │   ├── BinaryOperator '>' 'bool'
    │   │   ├── ImplicitCastExpr 'int' <LValueToRValue>
    │   │   │   └── DeclRefExpr 'x' 'int'
    │   │   └── IntegerLiteral 50
    │   └── CompoundStmt
    │       └── ReturnStmt
    │           └── BinaryOperator '+' 'int'
    │               ├── ImplicitCastExpr 'int' <LValueToRValue>
    │               │   └── DeclRefExpr 'x' 'int'
    │               └── ImplicitCastExpr 'int' <LValueToRValue>
    │                   └── DeclRefExpr 'y' 'int'
    └── ReturnStmt
        └── ImplicitCastExpr 'int' <LValueToRValue>
            └── DeclRefExpr 'x' 'int'
```

### Generated ecsl MLIR

```mlir
module {
  ecsl.func @compute(%x: i32, %y: i32) -> i32
      requires {
          ecsl.constraint.cmp ge, %x, %c0 : i32
              { ecsl.property_id = "compute.requires.0" }
          ecsl.constraint.cmp le, %x, %c100 : i32
              { ecsl.property_id = "compute.requires.1" }
          ecsl.constraint.cmp gt, %y, %c0 : i32
              { ecsl.property_id = "compute.requires.2" }
      }
      ensures(%result: i32) {
          ecsl.constraint.cmp ge, %result, %x : i32
              { ecsl.property_id = "compute.ensures.0" }
      }
      assigns nothing
  {
      %c50 = arith.constant 50 : i32
      %cond = arith.cmpi sgt, %x, %c50 : i32
      %ret = scf.if %cond -> i32 {
          %sum = arith.addi %x, %y : i32
          scf.yield %sum : i32
      } else {
          scf.yield %x : i32
      }
      ecsl.return %ret : i32
  }
  loc("compute.c":7:1)
}
```

## References

- [Arcanum Roadmap](./arcanum-roadmap.md)
- [Arcanum MVP Implementation Plan](./arcanum-mvp-plan.md)
- [ECSL Specification](./ECSL.md)
- Clang AST Reference: https://clang.llvm.org/docs/IntroductionToTheClangAST.html
- Clang Doxygen (Decl): https://clang.llvm.org/doxygen/classclang_1_1FunctionDecl.html
