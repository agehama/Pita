---
title: "リサイクルマークを作る"
weight: 3
---

![リサイクルマーク](https://github.com/agehama/Pita/raw/master/docs/pita_doc/images/recycle.svg?classes=shadow)
このチャプターでは、上記のようなリサイクルマークを作ります。

まず最初に、原型となる矢印の形を作りましょう。
矢印の形はPitaに初めから入っており、`Arrow{}`という風に呼び出すこともできますが、ここでは練習のために自分で定義してみます。

```
arrow = Shape{
    body: Square{scale: Vec2(200, 20), pos: Vec2(-100, 0)}
    head: Triangle{scale: X2(50), pos: X2(0), angle: 90}
}
```

一旦ここで`rycycle.cgl`のように名前を付けて保存し、実行してみましょう。
実行の仕方は[ダウンロードとインストール]({{%relref "/start/_index.md" %}})を参照してください。
正しく実行されると、次のような図が出力されるはずです。
![矢印の形](https://github.com/agehama/Pita/raw/master/docs/pita_doc/images/arrow.svg?classes=shadow)

次に、円の弧に沿った形のパスを作ります。
先ほどの後ろに続けて次のように書いてみましょう。
```
arcPath = (r, begin, end, num -> 
{
    delta = 1.0*(end - begin)/num
    line: for i in 0:num list(
        angle = begin + delta*i
        Vec2(r*Cos(angle), r*Sin(angle))
    )
})
```
これは、beginとendで指定された角度の円弧をパスとして返す関数です。
ただ関数を定義しただけでは何も表示されないので、続けて次のように書いて出力を確認してみましょう。

```
Shape{
    path: arcPath(100, 0, 120, 50)
    stroke: Rgb(0, 0, 0)
}
```
ここまでのコードを実行すると、次のような図が出力されるはずです。
![円弧の形](https://github.com/agehama/Pita/raw/master/docs/pita_doc/images/arc.svg?classes=shadow)

最後に矢印を定義した円の弧に沿って曲げることで、リサイクルマークの形を作ります。
先ほどの後ろに続けて次のように書いてみましょう。

```
recycle = Shape{
    a = arrow{}
    arrows: for i in 0:2 list(
        DeformShapeByPath2(a, 
            arcPath(100, 120*i, 120*(i+1), 50),
            a.body.left(), a.head.top())
    )
    fill: Rgb(0, 162, 232)
}
```

ここまで書いて実行し、冒頭に示したリサイクルマークの図が出力されれば成功です。
![リサイクルマーク](https://github.com/agehama/Pita/raw/master/docs/pita_doc/images/recycle.svg?classes=shadow)


以下にこの例で用いたソースコードの全体を記載します。

```
arrow = Shape{
    body: Square{scale: Vec2(200, 20), pos: Vec2(-100, 0)}
    head: Triangle{scale: X2(50), pos: X2(0), angle: 90}
}

arcPath = (r, begin, end, num -> 
{
    delta = 1.0*(end - begin)/num
    line: for i in 0:num list(
        angle = begin + delta*i
        Vec2(r*Cos(angle), r*Sin(angle))
    )
})

Shape{
    path: arcPath(100, 0, 120, 50)
    stroke: Rgb(0, 0, 0)
}

recycle = Shape{
    a = arrow{}
    arrows: for i in 0:2 list(
        DeformShapeByPath2(a, 
            arcPath(100, 120*i, 120*(i+1), 50),
            a.body.left(), a.head.top())
    )
    fill: Rgb(0, 162, 232)
}

```