# 1. 概要

FabGL 配下で NEC PC-8001 の画面表示を行う VGA ディスプレイドライバの単体テスト用プログラムです。

# 2. 詳細説明

長い間メンテナンスされている鶏肋電算研究所 [N80の部屋](http://home1.catvmics.ne.jp/~kanemoto/n80/index.html) で公開されている
[n80pi](http://home1.catvmics.ne.jp/~kanemoto/dist/n80pi20210814.tar.gz) を FabGL 活用して移植する際に Canvas レベルは実用では無いので
専用の VGA ディスプレイドライバが必要とわかりました。このドライバは FabGL VGADirectController クラスを継承した CGA なドライバになります。

NEC PC-8001 の画面描画属性情報は以下のようになります。

![描画属性](/img/描画属性.jpg)

n80pi はこれらの情報を以下のようにパックしています。

  char attr
  b76543210
   |||||||+--- B
   ||||||+---- R
   |||||+----- G
   ||||+------ upper line
   |||+------- simple Graphics
   ||+-------- reverse
   |+--------- erase(secret)
   +---------- under line

元のブリンクはビット展開されていません。タイミングを合わせてパック側のリバースが1/0する形式です。
結果、元々の NEC PC-8001 の v-ram より以下のように展開されます。

 +0        +1        +2        +3        ...
 char code char attr char code char attr  
 8bit      8bit      8bit      8bit

この構造はいわゆる CGA フレームバッファです。このフレームバッファを FabGL の VGA としてスキャンラインビットマップ展開すれば画面描画が出来ると言う事になります。

この単体テストプログラムを動作させるためには FabGL のインストールと microSD カードが必要です。microSD カードには以下の構造でフォントファイルを配置します。

 microSD
 /
 +--PC8001
    +-- PC-8001.FON

フォントファイルは [n80pi20210814.tar.gz](http://home1.catvmics.ne.jp/~kanemoto/dist/n80pi20210814.tar.gz)より入手できます。
このファイルは 4096 byte で、簡易グラフックスの bit データを展開済みのデータになります。
PC-8001.FON は手持ちの 2048 byte のファイルを利用しても構いません。2048 byte のときは簡易グラフックスの bit データを生成します。

# 3.その他

レトロ PC を動作させるエミュレータもスマホで十分利用できるような時代になり感無量な時代です(笑) 無償でソースコードを提供されている方々には感謝致します。
NEC PC-8001 も発売から既に 40 年超えと。思えば遠くまで来てしまいました(バロス)
