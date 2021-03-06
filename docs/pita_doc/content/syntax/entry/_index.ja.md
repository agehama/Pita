---
title: "初級編"
weight: 10
---

### Pitaプログラム
Pitaのプログラムは複数の式が並んだものです。
処理系は上から式を順番に評価していき、最後に評価した値を結果として出力します。

---

### 基本的な演算子
通常のプログラミング言語と同じような演算子を使うことができます。
剰余はMod関数を使って求めることができます。

#### 四則演算

| 演算子 | 意味 |
| ------ | ----------- |
| +a | a の値 |
| -a | -a の値 |
| a + b | a ＋ b |
| a - b | a － b |
| a * b | a × b |
| a / b | a ÷ b |
| a ^ b | a の b 乗 |

#### 代入演算

| 演算子 | 意味 |
| ------ | ----------- |
| a = b  | a に b を代入 |

#### 論理演算

| 演算子 | 意味 |
| ------ | ----------- |
| !a  | a の否定 |
| a \| b | a または b |
| a & b | a かつ b |
| a < b | a が b より小さい |
| a <= b | a が b 以下である |
| a > b | a が b より大きい |
| a >= b | a が b 以上である |

---

### 変数の定義
`変数名 = 値`と書くことで変数を定義することができます。
```
a = 4
b = 5
a + b
```

新しい変数の定義と、既に存在する変数への代入はどちらも=演算子によって行われます。
したがって、=の左辺の変数がすでに存在したら再代入、存在しなかったら変数の定義とみなされます。
```
a = 4 //変数定義
a = 5 //再代入
a
```

---

### リスト
[]で囲むことで値のリストを作ることができます。
```
xs = [1, 2, 3]
xs[0]
```

```
xs = [1, 2, 3]
xs[0]
```

---

### レコード

---

### 図形を呼び出す

---

### ループ

---

### 条件分岐

---

### コメント