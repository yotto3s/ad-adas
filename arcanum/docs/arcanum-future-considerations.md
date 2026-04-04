# Arcanum: Future Design Considerations

> **Status:** Living document
>
> **Date:** 2026-04-03

MVP計画の議論で浮上した、将来の拡張時に検討すべき事項をまとめる。MVPスコープ外だが、設計判断に影響するため記録しておく。

## F1: ループの再帰関数への変換

### 決定事項

ループ（for/while/do-while）はecsl IRではループ専用Opを持たず、パーサーの段階で再帰関数に変換する。

### 変換パターン

```c
// 元のCコード
int sum(int *a, int n) {
    int s = 0;
    for (int i = 0; i < n; i++) {
        s += a[i];
    }
    return s;
}
```

```whyml
(* 変換後のWhyML *)
let rec sum_aux (a: array int) (n: int) (i: int) (s: int) : int
  requires { 0 <= i <= n }
  variant  { n - i }
  ensures  { result >= 0 }
=
  if i >= n then s
  else sum_aux a n (i + 1) (s + a[i])
```

break → 再帰を呼ばずに値を返す。continue → 次の再帰呼び出しに直行。ループ内のearly return → 再帰関数からの値の返却。

### 未決定事項

- **補助関数の命名規則**: `sum_aux`, `sum_loop`, `sum_rec` 等。ユーザーがテスト/デバッグ時に読みやすい命名が必要
- **補助関数の契約の自動生成**: variant（停止性証明）は自動導出が必要。ループカウンタ型の単純なケースは `n - i` 等で自動生成可能だが、複雑なケースはユーザーアノテーションが必要になる可能性がある
- **元のループとのトレーサビリティ**: 生成された補助関数がどのループから由来するかのメタデータ保持。ASIL-Dの要件追跡に必要
- **ネストされたループ**: 二重ループは2段の再帰関数に変換するか、内側ループを独立関数に抽出するか
- **SMTソルバの性能評価**: 再帰関数の検証がループ不変条件ベースの検証と比較してソルバ負荷がどの程度異なるか、実際のベンチマークで評価する

## F2: ecsl.var/assign/loadのSSA整合性

### 背景

MLIRはSSA（静的単一代入）形式が基本だが、Cのミュータブル変数は同一変数への再代入が発生する。ecslの`ecsl.var`/`ecsl.assign`/`ecsl.load`でこれを表現する必要がある。

### 選択肢

**(A) SSA値として扱う**: 再代入のたびに新しいSSA値を生成し、後続の参照を更新する。ミュータブル変数を使わないCコード（関数型スタイル）には自然だが、一般的なCコードでは複雑になる。

**(B) 参照セマンティクス**: `ecsl.var`がメモリスロットを確保し、`ecsl.assign`/`ecsl.load`でread/writeする。memrefに似ているが独自型。WhyMLの`let ref`と直接対応する利点がある。

**(C) scf.if内のblock argumentで伝搬**: scf.ifがyieldする値としてミュータブル変数の「最新値」を渡す。SSA的にクリーンだが、変数が増えるとyieldの引数が煩雑になる。

### M1での方針

M1で実装しやすいものを採用する。MVPの対象コードはミュータブル変数の使用が最小限のため、(A)または(C)で十分な可能性が高い。ループ対応時に(B)への移行が必要になるかを評価する。

## F3: Safe C++サブセットにおけるbreak/continueの扱い

### 背景

再帰変換によりbreak/continueはMLIR/WhyMLレベルでは問題にならないが、Safe C++サブセットの言語仕様としてbreak/continueを許可するかは別の設計判断。

### 選択肢

**(A) 許可する**: パーサーがbreak → 「再帰を呼ばずに返す」、continue → 「次の再帰呼び出し」に変換する。既存Cコードとの互換性が高い。

**(B) 禁止する**: Safe C++サブセットの制約としてbreak/continueを禁止し、ループの脱出条件をwhile条件式で表現させる。形式検証の観点では脱出条件が明示的になり、ASIL-D安全性議論が強くなる。

### Frama-Cの参考情報

Frama-Cはbreak/continue/early returnをすべてgoto + ラベルに正規化する（oneret正規化）。WPプラグインはこの正規化されたCIL ASTから直接検証条件を生成する。Arcanumはgotoベースではなく再帰ベースのアプローチを取るため、Frama-Cとは異なる設計になる。

## F4: 整数意味論（固定幅 vs 数学的整数）

### 背景

MVPではWhyMLの数学的整数（`int`）を使用するが、Cの固定幅整数（32bit等）とは意味論が異なる。

### Frama-Cの参考アプローチ

Frama-Cは2層構造で整数オーバーフローに対処している:

1. **WPの算術モデル**: Machine Integerモデル（固定幅）とNatural Model（数学的整数）を選択可能。`-wp-rte`オプションでオーバーフロー不在アサーションの自動生成と検証を統合実行
2. **RTEプラグイン**: 各算術式に対してオーバーフロー不在のACSLアサーションを構文的に挿入する前処理パス。例えば `x + 1` に対して `/*@ assert rte: signed_overflow: x + 1 <= 2147483647; */` を自動生成。WPまたはE-ACSLがこれらのアサーションを検証する

この分離により、ユーザーが書く機能仕様（requires/ensures）とは独立に、オーバーフロー安全性を検証できる。

### Arcanumでの将来の対応方針

Frama-CのRTEアプローチをMLIR Passとして導入する:

1. **RTEPass（新規MLIR Pass）**: ecsl IRの各`arith.addi`, `arith.muli`等に対して、型のビット幅に基づくオーバーフロー不在の`ecsl.constraint`を自動生成し、requires/assertリージョンに挿入する
2. **WhyMLEmitPassへの影響**: 生成されたオーバーフロー制約がWhyMLのrequires/assertに変換され、SMTソルバが検証する
3. **テスト生成への影響**: BoundaryValuePassがRTEPassの出力も入力として受け取り、型境界近傍（`INT_MAX`, `INT_MIN`付近）の境界値テストケースを生成する
4. **BV理論への切り替え**: `use bv.BV32` 等のWhy3ビットベクター理論に切り替えることで、数学的整数との意味論差異を根本的に解消する。RTEPassとBV理論は併用可能（BV理論ではオーバーフローがラップアラウンドとして定義されるため、RTEアサーションが「C標準に従い未定義動作がないこと」の検証として機能する）

### 評価すべき事項

- BV理論に切り替えた際のSMTソルバの性能影響
- 既存のrequires/ensures契約がBV理論でも有効かの互換性確認
- 混合精度演算（int + short等）の型昇格ルールのWhyML表現
- RTEPassの粒度: すべての算術式にアサーションを挿入するか、ユーザーが`-rte`相当のオプションで制御するか

## F5: WhyMLEmitPassの複数方言横断

### 背景

ハイブリッド設計により、WhyMLEmitPassはecsl、arith、scfの3方言のOpをハンドルする必要がある。

### 潜在的リスク

- arith方言のバージョンアップでOp名やセマンティクスが変わった場合の追従コスト
- scf方言に新しいOpが追加された場合の未処理Opの検出
- 3方言の組み合わせで発生しうる想定外のOp列

### 対策

- WhyMLEmitPassに「未知のOpに遭遇した場合はエラーを報告する」フォールバックを実装
- LLVM 19にバージョンピニングすることで、arith/scfのAPI変更リスクを限定する
- 統合テストで3方言の組み合わせパターンを網羅する

## F6: 複数ファイル・翻訳単位の処理

### 背景

MVPは単一ファイル入力のみだが、実プロジェクトでは複数ファイルにまたがる関数呼び出しや型定義がある。

### Frama-Cの参考情報

Frama-Cは複数の入力ファイルをマージフェーズで単一の正規化ASTに統合する。WPプラグインはこの統合ASTに対してプロパティ単位で検証条件を生成する。`-wp-fct`オプションで特定関数のみの検証が可能。既に証明済みのプロパティは再検証しない。

### MVPでの決定事項

- 入力処理・WhyML出力はFrama-C方針に準拠（全関数処理、`--function`フィルタ）
- テスト出力はデフォルト1入力1出力、`--split-tests`で関数ごと分割可能
- 内部的には関数ごとに独立した`testgen.suite`として保持する

### 将来の検討事項

- 複数ファイル入力のマージ（Frama-Cのmergecilに相当する処理）
- 関数呼び出し先の契約の参照（モジュラー検証）
- 型定義のファイル間共有
- WhyMLのmoduleシステムとの対応（関数間呼び出し時のmodule import）
- ビルドシステム統合（CMakeカスタムターゲット、compile_commands.json活用）
- 増分検証（変更された関数のみ再処理。Frama-Cでは証明済みプロパティの再検証をスキップする仕組みがある）

## F7: ACSL Behaviors（assumes/complete/disjoint）

### 背景

ACSLでは関数契約を複数のbehaviorに分割できる。各behaviorは`assumes`節（適用条件）と追加の`ensures`節を持ち、入力条件に応じた異なる事後条件を記述できる。

```c
/*@ behavior positive:
      assumes x >= 0;
      ensures \result == x;
    behavior negative:
      assumes x < 0;
      ensures \result == -x;
    complete behaviors;
    disjoint behaviors;
*/
int abs(int x);
```

### ECSL/Arcanumでの検討事項

- `ecsl.func`にbehaviorリージョンを追加する設計（requires/ensuresと同階層か、入れ子か）
- `complete behaviors` → すべてのassumes節の論理和が真であることの検証条件生成
- `disjoint behaviors` → assumes節の対が排他的であることの検証条件生成
- BoundaryValuePassへの影響: behaviorのassumes境界がテストパーティションの自然な分割点になる。各behaviorに対して独立したテストケース群を生成する戦略が有効
- WhyMLEmitPassへの影響: WhyMLのmatch式やif/else分岐として自然に表現可能

### Frama-Cの参考情報

Frama-C/WPはbehaviorごとに独立した検証条件を生成する。complete/disjoint behaviorsは追加の検証目標（proof obligation）として扱われる。

## F8: admit/check修飾子

### 背景

ACSL（Frama-C 23以降）では`requires`/`ensures`/`assert`に`admit`/`check`修飾子を付与できる:

- `check requires P` — Pを検証するが、後続の推論では仮定しない
- `admit requires P` — Pを検証せず仮定する（外部証拠やツール限界への対処）
- `requires P`（無修飾）— `check requires P` + `admit requires P` と等価

### ECSL/Arcanumでの検討事項

- ASIL-D文脈では`admit`の使用を制限または監査ログに記録すべき。仮定を明示的に追跡することが安全性論証に必要
- ecsl IRではAttributeとして`check`/`admit`/`both`を契約節に付与する形が自然
- WhyMLEmitPassへの影響: `check`のみの節は検証条件として生成するがassume節には含めない。`admit`のみの節はaxiomまたはassumeとして扱う
- テスト生成への影響: `admit requires`はテスト生成の前提条件として使用するが、テスト自体がadmitの妥当性を（部分的に）実行時検証する副次効果がある

## F9: プロパティステータスの統合管理

### 背景

MVPではecsl IRの各契約節に`ecsl.property_id`を付与し、テスト生成と形式検証の結果を同一プロパティに紐付ける基盤を構築する。将来的にはFrama-Cのプロパティステータスシステムに相当する統合管理が必要になる。

### Frama-Cの参考設計

Frama-Cでは各ACSLプロパティに対して複数のプラグインがステータス（Valid / Invalid / Unknown / Not Attempted）を付与でき、カーネルがこれらを統合して最終的な妥当性ステータスを計算する。Emitterシステムにより、どのプラグインがどのステータスを付与したかが追跡可能。

### Arcanumでの将来構想

property_id基盤の上に構築するステータス管理:

- **テスト生成ステータス**: テストケースが生成された（Covered）/ 値域抽出に失敗した（Skipped）
- **テスト実行ステータス**: テストがパスした（Test Passed）/ 失敗した（Test Failed）
- **形式検証ステータス**: Why3で証明された（Proved）/ 証明できなかった（Not Proved）/ 未試行（Not Attempted）
- **統合ステータス**: テストと検証の結果を組み合わせた最終判定

ASIL-D文脈では「このプロパティはどの手段で検証されたか」の完全な記録が安全性論証に必要。property_idがその紐付けの鍵となる。

### 検討事項

- ステータスの永続化（JSON/SQLite等でプロジェクト間で共有）
- CI/CDとの統合（テスト実行結果のインポート）
- GUI/レポートでの可視化
- 増分検証（変更された関数のみ再検証、ステータスのインバリデーション）

## F10: 警告カテゴリの階層化

### 背景

Frama-Cでは`-kernel-warn-key`や`-<plugin>-warn-key`でwarningカテゴリごとにアクション（error / warning / inactive等）を制御できる。プラグインごとに独自のカテゴリを定義可能。

### Arcanumでの将来構想

Arcanumの各パスに警告カテゴリを定義し、ユーザーが運用モード（探索的 vs ASIL-D厳密）に応じて制御できるようにする:

- `parser:unsupported-syntax` — 非サポート構文の検出
- `parser:ecsl-syntax-error` — ECSL構文エラー
- `boundary-value:no-constraint` — パラメータに制約なし
- `boundary-value:range-extraction-failed` — 値域抽出失敗
- `whyml:integer-semantics-mismatch` — 整数意味論の差異に起因する潜在的問題
- `whyml:complex-control-flow` — 式への変換不能

ASIL-D運用では全カテゴリをerrorに設定し、探索的運用ではwarningに設定する、といった使い分けが可能になる。

## F11: パスベースのホワイトボックステスト生成（PathCrawler的アプローチ）

### 背景

MVPのBoundaryValuePassは契約（requires/ensures）のみから境界値を導出するブラックボックス的アプローチである。関数本体の分岐構造は考慮しない。

### Frama-C PathCrawlerの参考設計

PathCrawlerはホワイトボックスの構造テスト生成ツールで、以下の特徴を持つ:

- 関数本体の全実行可能パスをカバーするテスト入力を自動生成する
- 制約ソルバ（ECLiPSe CLP）を使い、各実行パスの分岐条件を満たす具体的な入力値を求める
- 「代表値」を事前に選ぶのではなく、ソルバが実行可能なパスを満たす値を生成する
- 入力値の選択は部分的にランダム（`-pc-deter`オプションで決定的モードに切替可能）
- all-path、k-path、branch coverage、MC/DCなど複数のカバレッジ基準をサポートする
- ACSLの事前条件を考慮してテスト対象のドメインを制限できる

### Arcanumでの将来構想

ecsl IRが関数本体（arith + scf）を保持しているため、PathCrawler的なパス解析は自然に実装できる:

- **PathAnalysisPass（新規MLIR Pass）**: ecsl IRの`scf.if`分岐を解析し、実行パスを列挙する。各パスの分岐条件をecsl制約として収集し、SMTソルバ（Z3）で充足可能性を判定する
- **ecsl → testgenへの変換**: 充足可能なパスごとに、ソルバが生成したモデル（具体値）を`testgen.case`に変換する
- **カバレッジ基準**: まずbranch coverageから始め、MC/DC、all-path等に段階的に拡張する
- **BoundaryValuePassとの併用**: ブラックボックス（境界値）とホワイトボックス（パスベース）のテストを両方生成し、統合するワークフロー

### 設計上の検討事項

- パス爆発の制御: ループの再帰変換により、再帰の深さを制限することでパス数を制御できる。PathCrawlerの`k-path`に相当
- ECSL契約との統合: requires節をパス制約の追加条件として使用し、実行不可能パスの早期刈り込みに活用する
- テストケースのラベリング: パスベースで生成されたテストには「どの分岐をカバーするか」のメタデータを付与し、トレーサビリティを確保する（`testgen.case`のstrategy属性に`"path_coverage"`等を記録）
- 代表値問題の解消: ソルバが具体値を生成するため、BoundaryValuePassの代表値選定ヒューリスティクスに依存しなくなる

### ISO 26262との関連

ISO 26262 Part 6 Table 9では、ソフトウェアユニットテストにおいて以下が推奨されている:
- ASIL A/B: branch coverage（高く推奨）
- ASIL C/D: MC/DC（高く推奨）

PathCrawler的なパスベーステスト生成は、これらのカバレッジ基準を自動で達成するための主要な手段となる。

## References

- [Arcanum Roadmap](./arcanum-roadmap.md)
- [Arcanum MVP Implementation Plan](./arcanum-mvp-plan.md)
- [ECSL Specification](./ECSL.md)
- [Supported Source Language Subset](./arcanum-mvp-cpp-subset.md)
