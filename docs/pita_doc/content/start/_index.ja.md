---
title: "ダウンロードとインストール"
weight: 10
chapter: false
---

---

### Windowsの場合
#### ダウンロード

[Pita(v0.5.0) for win](https://github.com/agehama/Pita/releases/download/v_0.5.0/pita-0.5.0-alpha-x86-win.zip) からダウンロードしてください。

#### サンプルの実行
ダウンロードしたファイルを解凍したら、コマンドプロンプトを起動してpita.exeのあるディレクトリへ移動してください。
そして、以下のコマンドを入力し、SVGファイルが出力されればインストールは成功です。
```
...\pita-0.5.0-alpha-x86-win> pita.exe examples/3rects.cgl -o 3rects.svg
```

---
### MacOSの場合
#### ダウンロード

[Pita 0.5.0](https://github.com/agehama/Pita/releases/download/v_0.5.0/pita-0.5.0-alpha-x86-win.zip) からダウンロードしてください。

#### サンプルの実行
ダウンロードしたファイルを解凍したら、コマンドプロンプトを起動してpita.exeのあるディレクトリへ移動してください。
そして、以下のコマンドを入力し、SVGファイルが出力されればインストールは成功です。
```
...\pita-0.5.0-alpha-x86-win>pita.exe examples/3rects.cgl -o 3rects.svg
```


---
### Ubuntuの場合
#### ダウンロード

[Pita 0.5.0](https://github.com/agehama/Pita/releases/download/v_0.5.0/pita-0.5.0-alpha-x86-win.zip) からダウンロードしてください。

#### サンプルの実行
ダウンロードしたファイルを解凍したら、コマンドプロンプトを起動してpita.exeのあるディレクトリへ移動してください。
そして、以下のコマンドを入力し、SVGファイルが出力されればインストールは成功です。
```
...\pita-0.5.0-alpha-x86-win>pita.exe examples/3rects.cgl -o 3rects.svg
```


---

### 他のOSの場合
上記以外のOSの場合は、ソースコードからPitaをビルドすることができます。
Pita(v0.5.0)のビルドに必要な要件は以下の通りです。

- Boost 1.54
- CMake 3.1
- C++14をサポートするコンパイラ

上記を満たす環境であれば、次のようにしてビルドすることができます。
```
git clone --recursive https://github.com/agehama/Pita.git
mkdir build
cd build
cmake ../Pita
make -j
```