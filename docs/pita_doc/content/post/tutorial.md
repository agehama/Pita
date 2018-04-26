---
title: "チュートリアル"
slug: "tutorial"
date: 2018-04-26T17:20:39+09:00
---

### 1. 図形を描く
Pitaでは、次のようにして図形を描くことができます。

```ruby
Shape{
    s1: Square{}
}
```

### 2. 複数の図形を描く
図形を呼び出した後の{}の中に位置や大きさを指定することができます。

また、Pitaでは式と式はカンマ(,)で区切りますが、カンマの代わりに改行を入れて省略することもできます。

```ruby
Shape{
    a: Square{pos: Vec2(100, 0), angle: 45}
    b: Triangle{}
}
```

### 3. 関数を定義する
関数は(引数->定義)の形で記述します。
関数は最後に評価した式の値を戻り値として返します。

```ruby
min = (a, b -> 
    if a < b
    then a
    else b
)
```

### 4. 図形を定義する
Shape{}で作った図形を使って、さらに別の図形を作ることもできます。

```ruby
x = Shape{
    a: Square{scale: Vec2(100, 10), angle: +45}
    b: Square{scale: Vec2(100, 10), angle: -45}
}

Shape{
    a: x{pos: Vec2(-100, 0)}
    b: x{pos: Vec2(+100, 0)}
}
```

### 5. 図形を返す関数を定義する

### 6. 再帰的な図形を定義する

```ruby
stick = Square{scale: Vec2(0.1, 1)}

shapes = (f, depth -> Shape{
    s: stick{}
    child: if 0 < depth then f(f, depth-1){pos: Vec2(0, 100)}
})
```

### 7. 制約を使って図形を定義する

