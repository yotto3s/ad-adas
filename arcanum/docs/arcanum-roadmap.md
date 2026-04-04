# Arcanum Toolchain: Roadmap

> **Status:** Draft — v0.1.0
>
> **Date:** 2026-04-03

## Vision

ArcanumはSafe C++サブセット向けの形式検証・テスト自動生成ツールチェインである。ECSL（Extended C Specification Language）による契約記述と、MLIR上のIRを基盤として、演繹的検証・テスト生成・所有権検証を統合的に提供する。

## 中間目標

Frama-Cの演繹的検証機能（WPプラグイン相当）を完全にカバーする。これにより、ACSLアノテーション付きのCコードに対して、Frama-C/WPと同等の機能的性質の証明が可能になる。

## Phase Overview

```
Phase 1: MVP
  │  純粋関数のECSL契約 → テスト生成 + WhyML形式検証
  ▼
Phase 2: 言語サポート拡張
  │  ループ、構造体、配列、ポインタ、固定幅整数
  ▼
Phase 3: WP相当
  │  最弱事前条件計算、メモリモデル、モジュラー検証
  │
  │  ── Frama-C演繹的検証カバー完了 ──
  ▼
Phase 4: 統合
  │  プロパティステータス管理、全機能の結果統合
  ▼
Phase 5: 所有権・ライフタイム
     ECSL独自拡張、Rust的な所有権の静的検証
```

---

## Phase 1: MVP

**目標:** 最小限のE2Eパイプラインを構築する。

**スコープ:**
- 入力: 副作用なし・ポインタなしの純粋関数（スカラー型のみ）
- ECSL契約: `requires`, `ensures`, `assigns \nothing`（算術制約、論理結合子）
- テスト生成: 境界値分析 → Google Test `TEST_P`
- 形式検証: ecsl IR → WhyML → Why3 → Z3/Alt-Ergo
- IR基盤: MLIR（ecsl + arith + scf + testgen方言）
- property_idによるトレーサビリティ基盤

**マイルストーン:**
- M0: MLIR Build Infrastructure（CMake + LLVM 22 + 空の方言登録）
- M1: ecsl Dialect + Parser（契約 + 関数本体のIR生成、early return正規化）
- M2: WhyMLEmitPass + Why3 Integration（形式検証E2E）
- M3: testgen Dialect + BoundaryValuePass（テスト生成IR）
- M4: Google Test CodeGen（テストコード出力）
- M5: End-to-End Integration（統合CLI）

**完了条件:** `arcanum testgen` と `arcanum verify` が単一の純粋関数に対してE2Eで動作する。

**詳細:** [arcanum-mvp-plan.md](./arcanum-mvp-plan.md)

---

## Phase 2: 言語サポート拡張

**目標:** 実用的なCコードを扱えるように対応言語構文を拡張する。

**スコープ:**

### Phase 2a: ループ対応
- パーサーでのループ→再帰関数変換（`ecsl.func` + `ecsl.call`）
- WhyMLでの再帰関数出力（`let rec` + `variant`）
- テスト生成: ループ含む関数の境界値分析
- variant（停止性証明）の自動導出（単純なカウンタ型ループ）

### Phase 2b: 構造体・列挙型
- ecsl方言への構造体型の追加
- 構造体メンバアクセスのIR表現
- WhyMLのrecord型への変換
- テスト生成: 構造体のフィールド単位の境界値分析

### Phase 2c: 配列
- ecsl方言への配列型・配列アクセスOpの追加
- ACSLの`\valid(a + (0..n-1))` 等の配列述語サポート
- WhyMLの`array`型への変換

### Phase 2d: ポインタ
- ecsl方言へのポインタ型の追加
- ACSLヒープ述語のサポート（`\valid`, `\valid_read`, `\separated`, `\freeable`, `\allocable`）
- `assigns`節の完全サポート（フレーム条件）
- テスト生成: ポインタ引数に対するテストファクトリ生成

### Phase 2e: 固定幅整数意味論
- WhyMLEmitPassでBV理論（`use bv.BV32`等）への切り替え
- RTEPass: 算術Opごとのオーバーフロー不在アサーション自動生成（Frama-C RTEプラグイン相当）
- 符号なし型の正確なモデリング
- 負数除算のC意味論との整合性

**完了条件:** ポインタ・構造体・配列・ループを含むCコードに対してテスト生成と形式検証が動作する。ACSLのヒープ述語と固定幅整数が正確に扱われる。

---

## Phase 3: WP相当（Frama-C演繹的検証カバー）

**目標:** Frama-C WPプラグインと同等の演繹的検証機能を提供する。

**スコープ:**

### Phase 3a: 最弱事前条件計算
- ecsl IR上でのWP計算エンジン（現在のWhyML直接変換からの発展）
- Qed相当の内部簡約器（検証条件の簡約化）
- 複数のSMTソルバへの同時送信（Alt-Ergo, Z3, CVC5）

### Phase 3b: メモリモデル
- Hoareモデル: 変数のアドレスが取られない場合の効率的なモデル
- Typedモデル: ヒープ値を型ごとの配列で表現
- 混合モデル: HoareとTypedの自動切り替え（Frama-C WPのデフォルト）

### Phase 3c: モジュラー検証
- 関数呼び出し先の契約を仮定として使用（呼び出し先の本体を展開しない）
- 複数ファイル・翻訳単位にまたがる検証
- WhyMLのmodule間importの生成

### Phase 3d: 高度なACSL構文サポート
- 量化子（`\forall`, `\exists`）
- 帰納述語（inductive predicates）
- 論理関数・述語定義
- ゴーストコード
- ステートメント契約
- behaviors（`assumes`, `complete behaviors`, `disjoint behaviors`）
- `admit`/`check` 修飾子
- `\at` ラベル（プログラムポイント間の状態参照）

**完了条件:** ACSL by Example（BSD-2-Clause、Fraunhofer FOKUS）の主要例題（equal, find, mismatch, sort等の標準アルゴリズム）がArcanumで検証可能。Frama-C WPと同等のプロパティ証明率を達成。

**ベンチマーク:**
- ACSL by Example（主要ベンチマーク、BSD-2-Clause）: https://github.com/fraunhoferfokus/acsl-by-example
- Frama-C open-source-case-studies（参考、各ケーススタディのライセンスに従う）: https://github.com/Frama-C/open-source-case-studies
- Arcanum独自のテスト入力（各Phaseで蓄積）

**← Frama-C演繹的検証カバー完了 →**

---

## Phase 4: 統合

**目標:** テスト生成と形式検証の結果を統一的に管理し、ASIL-D安全性論証を支援する。

**スコープ:**
- プロパティステータス管理（property_id基盤の上に構築）
  - テスト生成ステータス: Covered / Skipped
  - テスト実行ステータス: Passed / Failed
  - 形式検証ステータス: Proved / Not Proved / Not Attempted
  - 統合ステータスの自動計算
- 警告カテゴリの階層化（ASIL-D厳密運用 vs 探索的運用の切り替え）
- トレーサビリティレポート生成（ISO 26262準拠の要件追跡）
- CI/CD統合（テスト結果のインポート、増分検証）
- GUI/レポートでの可視化

**完了条件:** 各プロパティに対して「テストでカバー済み & 形式証明済み」の統合ステータスがレポートとして出力される。

---

## Phase 5: 所有権・ライフタイム（ECSL独自拡張）

**目標:** Frama-Cを超えた機能として、Rust的な所有権の静的検証をECSL + WPで実現する。

**スコープ:**

### Phase 5a: 所有権の基本サポート
- ECSL所有権構文の確定（`\owns`, `\borrows`, `\moves`）
- 所有権述語からACSLヒープ述語へのdesugaring変換パス
- 単純なケース（単一レベルの所有権移転・借用）のWP検証
- エイリアシング制約（`\separated`）の自動生成

### Phase 5b: ライフタイム
- ECSL `\lifetime` 構文の設計と確定
- 借用の有効期間の検証
- 戻り値のライフタイムが引数に依存するケースの表現
- 入れ子の借用パターンのサポート

**完了条件:** Rust所有権モデルの主要パターン（所有権移転、不変借用、可変借用、ライフタイムパラメータ）がECSLで記述・検証可能。

---

## Backlog

以下は必要に応じて将来のPhaseとして実装する。現時点ではロードマップに含めない。

### E-ACSL相当（ランタイムアサーション変換）
- ECSL契約を実行時アサーション付きCコードに変換
- メモリ述語のシャドウメモリによる実行時検査
- 数学的整数のGMP変換
- 有界量化子のループ展開
- WPで証明できなかったプロパティの実行時検証
- テスト実行との統合（計装コードへのテスト実行）

### PathCrawler相当（パスベーステスト生成）
- 関数本体の分岐構造を解析したホワイトボックステスト生成
- 制約ソルバによるパスカバレッジ入力の自動生成
- branch coverage, MC/DCカバレッジ基準のサポート
- ISO 26262 Part 6 Table 9のカバレッジ基準達成支援

### その他の候補
- 並行プログラム解析（Frama-C Mthread相当）
- 時相論理検証（Frama-C Aoraï相当）
- プログラムスライシング（Frama-C Slicing相当）
- コードメトリクス（Frama-C Metrics相当）
- 対話的証明支援（Coq/Lean連携）

---

## Timeline（目安）

具体的なスケジュールは未定。以下は相対的な規模感の目安:

| Phase | 規模感 | 主な技術的課題 |
|-------|--------|----------------|
| Phase 1 (MVP) | 小 | MLIR方言設計、Clang統合、Why3連携 |
| Phase 2 (言語拡張) | 大 | ループの再帰変換、ポインタのIR表現、BV理論 |
| Phase 3 (WP) | 最大 | WP計算エンジン、メモリモデル、モジュラー検証 |
| Phase 4 (統合) | 中 | ステータス管理、レポート生成 |
| Phase 5 (所有権) | 大 | 所有権desugaring、ライフタイム検証 |

Phase 3が最も技術的に困難であり、最も時間を要する見込み。

## References

- [ECSL Specification](./ECSL.md)
- [MVP Implementation Plan](./arcanum-mvp-plan.md)
- [Supported Source Language Subset](./arcanum-mvp-cpp-subset.md)
- [Future Design Considerations](./arcanum-future-considerations.md)
- Frama-C: https://frama-c.com/
- ACSL: https://frama-c.com/html/acsl.html
- Why3: https://why3.lri.fr/
- ISO 26262: Road vehicles — Functional safety
