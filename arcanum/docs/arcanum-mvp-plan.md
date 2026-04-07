# Arcanum Toolchain: MVP Implementation Plan

> **Status:** Draft — v0.4.0
>
> **Date:** 2026-04-03

## Goal

Arcanumツールチェインの最小限の動作するパイプラインを構築する。Safe C++サブセットのソースコードに付与されたECSL契約から:

1. **テスト自動生成** — 人が読みやすいパラメータ化テスト（Google Test）を生成する
2. **形式検証** — WhyML（Why3）へ変換し、SMTソルバ（Z3/Alt-Ergo）で契約の充足を証明する

この2つの出力パスがecsl MLIR方言を共有入力とすることで、MLIRを採用する意味が生まれ、テストと証明のトレーサビリティが統一される。

最初のMVPでは、対象を極めて限定し、エンドツーエンドで動作することを最優先する。

## Scope

### In Scope (MVP)

- **入力言語**: 副作用なし・ポインタなしの純粋関数（標準ライブラリ不使用）
- **対応する型**: スカラー型のみ（`int`, `unsigned int`, `bool`, `char`）
- **ECSL契約**: `requires`（算術制約）、`ensures`（事後条件）、`assigns \nothing`
- **制約の種類**: 等式、不等式、論理結合子（`&&`, `||`, `!`）
- **関数本体**: ローカル変数宣言、代入、if/else、return、算術式・比較式
- **テスト生成**: 境界値分析（Boundary Value Analysis）→ Google Test `TEST_P`
- **形式検証**: ecsl IR → WhyML生成 → Why3 → Z3/Alt-Ergo
- **IR基盤**: MLIR（`ecsl` dialect + `arith` dialect + `scf` dialect + `testgen` dialect）
- **ビルドシステム**: CMake

### Out of Scope (MVP)

- 構造体、配列、ポインタ、列挙型
- 所有権・ライフタイム関連のECSL拡張構文
- ループ（`for`, `while`, `do-while`）
- ゴーストコード
- SMTソルバ連携によるテスト入力探索
- 標準ライブラリのサポート
- Google Test以外のテストフレームワーク出力
- C++固有の構文（テンプレート、参照、クラス等）

## Architecture

### パイプライン概要

```
Source (.c/.cpp + ECSL)
  │
  ▼
Parser (Clang + ECSL parser)
  │  - early returnをif/elseネストに正規化
  │  - (将来) ループを再帰関数に変換
  │
  ▼
ecsl dialect + arith + scf (MLIR)  ← 契約 + 関数本体
  │
  ├──→ BoundaryValuePass            ← テスト生成パス
  │       │
  │       ▼
  │    testgen dialect (MLIR)
  │       │
  │       ▼
  │    TestCodeGen                   → test.cpp (Google Test)
  │
  └──→ WhyMLEmitPass                ← 形式検証パス
          │
          ▼
       .mlw (WhyML file)            → Why3 → Z3 / Alt-Ergo
```

ecsl方言が2つの出力パスの共通入力となる。すべてのIRがMLIR上に乗るため、`mlir-opt`でパイプラインの途中結果をダンプ・検査できる。

### MLIR方言の役割分担（ハイブリッド設計）

| 関心事 | 方言 | 理由 |
|--------|------|------|
| 契約（requires/ensures） | `ecsl` | ECSL固有、他に代替なし |
| 関数定義 + 契約リージョン | `ecsl`（`ecsl.func`） | 契約リージョンを持つため独自が必要 |
| 算術演算（+, -, *, /, %） | `arith` | 標準方言、十分にテスト済み、定数畳み込み等のパスが利用可能 |
| 比較演算（==, <, >= 等） | `arith`（`arith.cmpi`） | 同上 |
| 構造化制御フロー（if/else） | `scf`（`scf.if` + `scf.yield`） | リージョンベースでWhyMLのif/else式と直接対応 |
| 変数（宣言・代入・読出） | `ecsl`（`ecsl.var`, `ecsl.store`, `ecsl.load`） | 将来の所有権/ライフタイム拡張の土台 |
| 関数呼び出し（将来の再帰） | `ecsl`（`ecsl.call`） | 再帰変換されたループの表現に使用 |

使用しない方言: `memref`（構造体に対応できない）、`cf`（gotoモデルはWhyML変換と相性が悪い）

### MLIR Dialect: `ecsl`

ECSL契約と変数操作を表現する方言。関数本体の算術・制御フローは`arith`/`scf`に委ねる。

#### 統合表現の例

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

#### MVP Op 一覧

**ecsl 契約 Operations:**

| Op | 説明 |
|----|------|
| `ecsl.func` | 契約付き関数定義。requires/ensures（block argument付き）/bodyリージョンを持つ |
| `ecsl.constraint.cmp` | 比較制約（`eq`, `ne`, `lt`, `le`, `gt`, `ge`） |
| `ecsl.constraint.and` | 論理積（リージョン内の制約すべて） |
| `ecsl.constraint.or` | 論理和（リージョン内の制約のいずれか） |
| `ecsl.constraint.not` | 論理否定 |

**ecsl 本体 Operations:**

| Op | 説明 |
|----|------|
| `ecsl.var` | ローカル変数の宣言と初期化 |
| `ecsl.store` | 変数への代入 |
| `ecsl.load` | 変数の値読み出し |
| `ecsl.return` | 関数からの返却 |
| `ecsl.call` | 関数呼び出し（将来の再帰変換で使用） |

**arith Operations（再利用）:**

| Op | 説明 |
|----|------|
| `arith.constant` | 定数値リテラル |
| `arith.addi`, `arith.subi`, `arith.muli`, `arith.divsi`, `arith.remsi` | 符号付き算術演算 |
| `arith.divui`, `arith.remui` | 符号なし算術演算 |
| `arith.cmpi` | 整数比較（sgt, sge, slt, sle, eq, ne 等） |
| `arith.negf`相当 | 単項マイナス（`arith.constant 0` + `arith.subi`で表現） |

**scf Operations（再利用）:**

| Op | 説明 |
|----|------|
| `scf.if` | 構造化条件分岐（then/elseリージョン、値をyield可能） |
| `scf.yield` | リージョンからの値の返却 |

**設計上の注意:**

- `ecsl.func`は3つのリージョンを持つ: requires, ensures, body
- ensuresリージョンはblock argumentとして`%result`を受け取る。bodyリージョンの`ecsl.return`の値がensuresの`%result`に束縛される。これによりMLIRのSSAモデル内で`\result`の型安全な参照が実現される
- 複数のrequires節はrequiresリージョン内で暗黙の論理積として結合される（ACSLセマンティクス）。同様に複数のensures節も論理積
- early returnはパーサーの段階で`scf.if`/`scf.yield`のネストに正規化される。bodyリージョンの最後に単一の`ecsl.return`がある形に統一
- すべてのOpはMLIRの`loc`を保持し、元のソースコード位置にトレース可能

**Types:**
- MVPではMLIR組み込みの`IntegerType`を使用（`i32`, `i16`, `i64`, `i8`, `i1`）
- 符号の区別はarith Opの選択（`divsi` vs `divui`等）とAttribute（`signedness`）で保持
- 将来: `ecsl.owned_ptr`, `ecsl.borrowed_ref` 等を追加

**Attributes:**
- `ecsl.property_id` — 各契約節に付与される一意なプロパティ識別子（例: `"compute.requires.0"`）。テスト生成・形式検証の結果を同一プロパティに紐付けるためのトレーサビリティ基盤。パーサーが `<関数名>.<契約種別>.<連番>` の形式で自動生成する
- `ecsl.source_loc` — 元のECSLアノテーションの位置（トレーサビリティ）
- `ecsl.signedness` — 符号付き/符号なしの区別

#### 拡張性

方言の将来拡張はOp追加として行い、既存定義に影響しない:

- 所有権: `ecsl.constraint.owns`, `ecsl.constraint.borrows`, `ecsl.constraint.moves`
- 量化子: `ecsl.constraint.forall`, `ecsl.constraint.exists`（リージョン付きOp）
- ヒープ述語: `ecsl.constraint.valid`, `ecsl.constraint.separated`
- ループ: 再帰関数に変換（`ecsl.func` + `ecsl.call`）。ループ専用Opは不要
- 配列: `ecsl.array_access`, `ecsl.array_length`

### Early Returnの正規化

パーサー（ClangBridge）がClang ASTを走査する際、複数のreturn文を持つ関数本体を`scf.if`のネストに変換する。

**変換例:**
```c
int f(int x, int y) {
    if (x > 50) {
        return x + y;    // early return
    }
    int z = x * 2;
    if (z > y) {
        return z;         // early return
    }
    return y - x;
}
```

→ MLIR上では単一の`ecsl.return`に正規化:
```mlir
ecsl.func @f(%x: i32, %y: i32) -> i32 ... {
    %c50 = arith.constant 50 : i32
    %cond1 = arith.cmpi sgt, %x, %c50 : i32
    %result = scf.if %cond1 -> i32 {
        %sum = arith.addi %x, %y : i32
        scf.yield %sum : i32
    } else {
        %c2 = arith.constant 2 : i32
        %z = arith.muli %x, %c2 : i32
        %cond2 = arith.cmpi sgt, %z, %y : i32
        %inner = scf.if %cond2 -> i32 {
            scf.yield %z : i32
        } else {
            %diff = arith.subi %y, %x : i32
            scf.yield %diff : i32
        }
        scf.yield %inner : i32
    }
    ecsl.return %result : i32
}
```

### MLIR Dialect: `testgen`

テスト生成結果をMLIR上で表現する方言。テストフレームワーク非依存。（v0.3.0から変更なし）

#### MVP Op/Attr 一覧

**Operations:**

| Op | 説明 |
|----|------|
| `testgen.suite` | テストスイート。対象関数への参照と子テストケースを持つ |
| `testgen.case` | 個別のテストケース。ラベル・入力・期待条件・導出情報を持つ |
| `testgen.input` | テスト入力値（パラメータ名と具体値） |
| `testgen.expect.cmp` | 事後条件の検証（比較アサーション） |

**Attributes:**

| Attr | 説明 |
|------|------|
| `property_id` | 導出元のecsl契約節の`ecsl.property_id`（トレーサビリティ） |
| `strategy` | 導出に使われたパーティション戦略 |
| `rationale` | 人間可読な導出理由 |
| `source_loc` | 元の契約節のソース位置 |

`property_id`により、テスト結果を元の契約節に紐付けられる。将来的にWhyML検証結果と統合し「このプロパティはテストでカバー済み & 形式証明済み」のような統合ステータスを構築する基盤となる。

### MLIR Pass: BoundaryValuePass (ecsl → testgen)

事前条件の制約から、テスト入力のパーティションと境界値を導出する。

**処理フロー:**

1. `ecsl.func` を走査する
2. requiresリージョン内の `ecsl.constraint.cmp` から各パラメータの値域を抽出する
3. 値域から境界値セットを生成する（`{min, min+1, typical, max-1, max}`）
4. 複数パラメータは各パラメータの境界値を独立に変化させ、他は代表値を使用する
5. ensuresリージョン内の制約をそのまま `testgen.expect.*` に変換する
6. 各`testgen.case`に導出元の`ecsl.property_id`を記録する
7. 結果を `testgen.suite` / `testgen.case` として出力する

**代表値（typical）の選定:**
- 明示的な値域 `[min, max]` がある場合: `(min + max) / 2`（中央値）
- 下限のみ指定（`y > 0`）の場合: `min + 1`（下限のすぐ上）。型の最大値付近は避ける
- 上限のみ指定の場合: `max - 1`
- 制約なしの場合: `0`（符号付き）または `1`（符号なし）

**整数オーバーフローの扱い:**
- 型の最大値/最小値（`INT_MAX`, `INT_MIN`等）は、明示的に制約で参照されない限り境界値に含めない
- 暗黙の上下限は型のビット幅から導出するが、テスト値としては使用しない。将来のRTEPass/SMTベース戦略でオーバーフロー検証を行う

**拡張:**
- `EquivalenceClassPass` — 等価クラス分割
- `SMTPartitionPass` — Z3連携による入力探索
- `PathAnalysisPass` — パスベースのホワイトボックステスト生成（PathCrawler的アプローチ）
- `PropertyBasedPass` — ランダムテスト生成

### MLIR Pass: WhyMLEmitPass (ecsl + arith + scf → .mlw)

ecsl IRからWhyML（Why3の入力言語）を生成する。ecsl、arith、scfの3方言を横断して変換する。

**変換規則（MVP）:**

| MLIR Op | WhyML |
|---------|-------|
| `ecsl.func @f(...)` | `let f (x: int) (y: int) : int` |
| requires リージョン | `requires { ... }` |
| ensures リージョン | `ensures { ... }` |
| `ecsl.constraint.cmp ge, %x, %c0` | `x >= 0` |
| `ecsl.constraint.and { ... }` | `... /\ ...` |
| `ecsl.constraint.or { ... }` | `... \/ ...` |
| `ecsl.constraint.not { ... }` | `not (...)` |
| `arith.addi %x, %y` | `x + y` |
| `arith.subi %x, %y` | `x - y` |
| `arith.muli %x, %y` | `x * y` |
| `arith.divsi %x, %y` | `div x y` |
| `arith.remsi %x, %y` | `mod x y` |
| `arith.cmpi sgt, %x, %y` | `x > y` |
| `arith.constant 42` | `42` |
| `scf.if %cond -> T { ... } else { ... }` | `if cond then ... else ...` |
| `scf.yield %v` | 式の値として展開 |
| `ecsl.var` / `ecsl.store` / `ecsl.load` | `let ref x = ... in` / `x := ...` / `!x` |
| `ecsl.return %v` | 関数本体の最終式 |
| ensures block argument `%result` | `result` |

**MVP出力例:**

```whyml
module Compute
  use int.Int

  let compute (x: int) (y: int) : int
    requires { 0 <= x /\ x <= 100 }  (* compute.requires.0, compute.requires.1 *)
    requires { y > 0 }               (* compute.requires.2 *)
    ensures  { result >= x }         (* compute.ensures.0 *)
  =
    if x > 50 then
      x + y
    else
      x
end
```

WhyML出力にproperty_idをコメントとして埋め込む。Why3の検証結果（proved/not proved）をproperty_idと対応付けて報告することで、「どの契約節が証明済みか」をソースコードレベルで追跡可能にする。

**整数意味論の制限事項（MVP）:**

| 性質 | C (int) | WhyML (int) | 影響 |
|------|---------|-------------|------|
| 精度 | 32bit固定 | 任意精度 | オーバーフロー未検出 |
| 除算 | 0方向切り捨て | ユークリッド除算 | 負数の除算で差異 |
| 符号なし型 | `unsigned int` | なし（int >= 0で代替） | requiresで範囲制約を追加 |

### TestCodeGen: testgen → Google Test

`testgen` dialect からC++ソースコードを生成する。

**実装方針:**
- `testgen.suite` をトラバースし、テンプレートベースでC++コードを出力する
- CodeGenはインターフェースとして抽象化し、Google Test以外のバックエンド追加に備える
- 出力コードにはトレーサビリティ用のコメント（`property_id`, `source_loc`, `strategy`, `rationale`）を埋め込む
- C99モード（`--std=c99`）で入力された場合、テストコードは `extern "C"` で対象ヘッダを include する
- `--split-tests`指定時は関数ごとにファイルを分割する

### エラーハンドリング

パイプライン全体で統一的なエラー報告を行う。

**方針:** MLIRのDiagnosticsインフラ（`mlir::emitError`, `mlir::emitWarning`）を使用する。これにより、すべてのエラーにソース位置情報が付与され、`mlir-opt`のデバッグ出力と一貫性が保たれる。

| ステージ | エラーの例 | 重大度 |
|----------|-----------|--------|
| Parser | ECSL構文エラー、`/*@`コメント未検出 | Error |
| Parser | 非サポート構文の検出（ループ等） | Error |
| Parser | Clangのwarning | Warning（透過的に報告） |
| Parser | ヘッダに宣言がない関数のスキップ | Info |
| BoundaryValuePass | 値域抽出の失敗（複雑すぎる制約） | Warning + スキップ |
| BoundaryValuePass | パラメータに制約なし | Warning（型の全域を使用） |
| WhyMLEmitPass | 式への変換不能（複雑な制御フロー） | Error |
| WhyMLEmitPass | 未知のOp検出 | Error |
| TestCodeGen | 出力ファイル書き込み失敗 | Error |

### 複数関数の処理

Frama-Cの方針に従い、入力ファイル全体をパースしすべてのECSLアノテーション付き関数を処理する。

**入力処理（Frama-C準拠）:**
- 入力ファイル全体を単一の正規化ASTに変換する
- ecsl IRにはすべてのアノテーション付き関数が `ecsl.func` として含まれる
- `--function <name>` オプションで特定関数のみを処理対象にできる（Frama-Cの`-wp-fct`に相当）
- デフォルトではすべてのアノテーション付き関数を処理する

**テスト出力（Arcanum固有）:**
- 内部的には関数ごとに独立した`testgen.suite`として保持する
- デフォルトでは入力ファイル1つにつきテスト出力1ファイル（`compute.c` → `compute_test.cpp`）
- `--split-tests` オプションで関数ごとにテストファイルを分割できる（`foo_test.cpp`, `bar_test.cpp`）
- 分割モードは増分ビルドに有利

**WhyML出力（Frama-C準拠）:**
- 1つの `.mlw` ファイルに1つの `module` としてすべての関数を出力する
- Why3の検証結果はproperty_idごとに報告する

### ヘッダファイルとテスト対象関数の決定

**テスト対象の決定:** ヘッダファイル（`.h` / `.hpp`）で宣言されている関数のみをテスト生成・形式検証の対象とする。ヘッダに宣言がない関数（static関数、内部実装関数等）は対象外。これにより「公開APIのみをテストする」という一貫したルールが成立する。

**プリプロセッサオプションの取得:** ソースファイルのコンパイル時に渡されたプリプロセッサオプションをそのまま使用する。includeパスに影響するすべてのオプションを受け付ける:

- `-I <dir>` — ユーザーinclude検索パス
- `-isystem <dir>` — システムヘッダ検索パス
- `-iquote <dir>` — `#include "..."` のみの検索パス
- `-isysroot <dir>` — システムルートの変更
- `-D <macro>` / `-U <macro>` — マクロ定義/解除（ヘッダの条件分岐に影響）
- `-include <file>` — 強制インクルード
- `-nostdinc` / `-nostdinc++` — 標準ヘッダ検索パスの除外
- その他Clangが受け付けるプリプロセッサ関連オプション

これらはClangフロントエンドにそのまま渡されるため、Arcanum側で独自の解釈は行わない（Frama-CのClang委任方針と同じ）。

**オプションの指定方法:**

- `--compilation-db <path>` — `compile_commands.json`からファイルごとのコンパイルオプションを自動取得（CMakeの`CMAKE_EXPORT_COMPILE_COMMANDS=ON`で生成）。Clangの`CompilationDatabase` APIで直接パース
- `--` — 以降のオプションをClangに直接渡す（例: `arcanum verify --input foo.c -- -I./include -isystem /usr/local/include -DNDEBUG`）
- 両方指定された場合、`--`以降のオプションが`compile_commands.json`の内容に追加される

**生成されるテストコードのinclude:**
- テストコードの`#include`にはヘッダファイルのパスを使用
- テストのビルド時にもソースと同じプリプロセッサオプションを使用する必要がある。生成されたテストファイルのヘッダコメントに必要なincludeパスを記録する

## Implementation Language and Build

- **実装言語:** C++（MLIR/Clangとの統合が自然なため）
- **ビルドシステム:** CMake
- **対象LLVMバージョン:** LLVM 22.x（2026年3月リリースの最新安定版。APIの安定性を確保するためピニングする）
- **依存関係:**
  - LLVM/MLIR 22 — IR基盤、方言定義、パスインフラ（arith/scf方言を含む）
  - Clang 22 — 関数シグネチャ・本体のパース
  - Why3 — WhyML検証の実行（外部ツール、ランタイム依存）
  - Z3 / Alt-Ergo — SMTソルバ（Why3経由で呼び出し）
  - Google Test — 生成されたテストの実行（テスト対象コード側の依存）

### プロジェクト構成

```
arcanum/
├── CMakeLists.txt
├── cmake/
│   └── FindMLIR.cmake
├── ecsl/                        # ecsl方言
│   ├── CMakeLists.txt
│   └── IR/
│       ├── ECSLDialect.td
│       ├── ECSLOps.td
│       ├── ECSLDialect.h
│       ├── ECSLDialect.cpp
│       ├── ECSLOps.h
│       └── ECSLOps.cpp
├── testgen/                     # testgen方言 + テスト生成パス
│   ├── CMakeLists.txt
│   ├── IR/
│   │   ├── TestGenDialect.td
│   │   ├── TestGenOps.td
│   │   ├── TestGenDialect.h
│   │   ├── TestGenDialect.cpp
│   │   ├── TestGenOps.h
│   │   └── TestGenOps.cpp
│   ├── Passes/
│   │   ├── BoundaryValuePass.h
│   │   └── BoundaryValuePass.cpp
│   └── CodeGen/
│       ├── TestCodeGen.h
│       └── GoogleTestCodeGen.cpp
├── verify/                      # 形式検証パス
│   ├── CMakeLists.txt
│   ├── WhyMLEmitPass.h
│   ├── WhyMLEmitPass.cpp
│   ├── Why3Runner.h
│   └── Why3Runner.cpp
├── parser/                      # フロントエンド
│   ├── CMakeLists.txt
│   ├── ECSLParser.h
│   ├── ECSLParser.cpp
│   ├── ClangBridge.h
│   └── ClangBridge.cpp
├── tools/
│   ├── CMakeLists.txt
│   ├── arcanum-testgen.cpp
│   ├── arcanum-verify.cpp
│   └── arcanum.cpp
└── test/
    ├── CMakeLists.txt
    ├── examples/
    │   └── compute.c
    ├── ecsl/
    ├── testgen/
    ├── verify/
    └── e2e/
```

### CMake構成方針

- LLVM/MLIR 22はシステムインストールまたは`find_package(MLIR)`で検出する
- 各サブディレクトリは独立した`CMakeLists.txt`を持つ
- TableGen（`.td`）からOp定義のC++コードを自動生成する
- Why3はCMakeの`find_program`で検出し、見つからない場合は検証パスのビルドをスキップする
- テストは`ctest`で実行する

## Milestones

### M0: MLIR Build Infrastructure

CMakeプロジェクトのセットアップ。LLVM 22/MLIRの検出、TableGenの設定、空のecsl/testgen方言の登録確認。arith/scf方言のリンク確認。

**完了条件:** 空のecsl/testgen方言 + arith/scfを含むプロジェクトがビルドでき、カスタム`mlir-opt`相当のツールでMLIRモジュールをロード・ダンプできる。

### M1: ecsl Dialect + Parser

ecsl方言のOp定義（契約 + 変数操作）と、Clang AST + ECSLコメントからecsl + arith + scf MLIR IRを生成するパーサーの実装。early returnの正規化を含む。

**完了条件:** サンプルの `.c` ファイルから、契約と関数本体（arith/scfを含む）のMLIR IRがダンプされる。early returnが`scf.if`ネストに正規化されている。

### M2: WhyMLEmitPass + Why3 Integration

ecsl + arith + scf IRからWhyMLを生成するパスと、Why3 CLIを呼び出して検証結果を取得するランナーの実装。

**完了条件:** サンプル関数のIRから `.mlw` が生成され、`why3 prove` で検証が成功する。契約を意図的に破った場合は検証が失敗する。

### M3: testgen Dialect + BoundaryValuePass

testgen方言のOp定義と、ecsl → testgen変換を行うBoundaryValuePassの実装。

**完了条件:** ecsl IRからtestgen IRが生成され、境界値が正しく導出されている。

### M4: Google Test CodeGen

testgen IRからコンパイル・実行可能なGoogle Testコードを出力するCodeGenの実装。

**完了条件:** 生成されたテストコードがコンパイル・実行でき、全テストがパスする。

### M5: End-to-End Integration

M0〜M4を統合CLIとして結合する。

```bash
# テスト生成のみ
arcanum testgen --input compute.c --output compute_test.cpp

# 形式検証のみ
arcanum verify --input compute.c

# 両方実行（compile_commands.jsonから自動取得）
arcanum --input compute.c --compilation-db ./build

# Clangオプション直接指定
arcanum --input compute.c -- -I./include -isystem /usr/local/include -DNDEBUG

# 関数フィルタ + テスト分割
arcanum testgen --input compute.c --function=compute --split-tests --output-dir ./tests
```

**完了条件:**
- 単一コマンドで `.c` / `.cpp` ファイルからテストコードが生成され、ビルド・実行できる
- 同じ入力ファイルに対してWhyML生成・Why3検証が成功する
- 契約を意図的に破った場合、検証は失敗しテストも失敗する

## Open Design Decisions

### O1: ECSL契約の抽出方法

Clangの `RawComment` APIが `/*@` スタイルのコメントを収集するか、M1で検証する。収集しない場合、ソースバッファからの直接抽出にフォールバックする。

### O2: ~~ecsl.func内の`\result`のSSA表現~~ → 決定済み

ensuresリージョンはblock argumentとして`%result`を受け取る方式を採用。bodyの`ecsl.return`値がensuresの`%result`に束縛される。Frama-Cでは`\result`を特殊な論理変数として扱いWP計算時に戻り値と結合するが、Arcanumではblock argumentとしてMLIRのSSAモデル内で型安全に表現する。

### O3: ecsl.var/store/loadのSSA整合性

MLIRはSSA形式が基本だが、ミュータブル変数を`ecsl.var`で表現する場合の整合性。M1で具体化。

## Extensibility Checklist

- [ ] 新しい型（struct, array, pointer, enum）→ ecsl方言にOp/型を追加
- [ ] 新しい制約（量化子、ヒープ述語、所有権）→ ecsl方言にOpを追加
- [ ] ループ対応 → パーサーで再帰関数に変換（`ecsl.func` + `ecsl.call`）。新しいOp不要
- [ ] 固定幅整数 → WhyMLEmitでBV理論への切り替え
- [ ] 新しいパーティション戦略 → 新しいMLIR Passとして追加
- [ ] 新しいテストフレームワーク出力 → CodeGenバックエンドの追加
- [ ] トレーサビリティレポート → ecsl/testgen IRのメタデータから生成
- [ ] カウンターエグザンプルからのテスト生成 → Why3の反例をtestgen IRに変換するパス

## References

- [Arcanum Roadmap](./arcanum-roadmap.md)
- [ECSL Specification](./ECSL.md)
- [Supported Source Language Subset](./arcanum-mvp-cpp-subset.md)
- [Future Design Considerations](./arcanum-future-considerations.md)
- ACSL: https://frama-c.com/html/acsl.html
- MLIR: https://mlir.llvm.org/
- Why3: https://why3.lri.fr/
- WhyML Reference: https://why3.lri.fr/doc/syntaxref.html
- Google Test: https://google.github.io/googletest/
- ISO 26262: Road vehicles — Functional safety
