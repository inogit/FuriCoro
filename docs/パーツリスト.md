# FuriCoro パーツリスト

「Coro」シリーズ第2弾。手を振るだけでリモート会議のリアクションを
送るジェスチャーガジェット「FuriCoro」の部品一覧です。

## 必須パーツ（スイッチサイエンスで購入）

| # | 部品 | 数量 | 用途 | 購入先 |
|---|------|:---:|------|-------|
| 1 | Raspberry Pi Pico 2 W | 1 | メインマイコン（BLE HID） | https://www.switch-science.com/products/10053 |
| 2 | GROVE - ジェスチャー（PAJ7620U2） | 1 | 手のジェスチャー検出 | https://www.switch-science.com/products/2645 |
| 3 | Grove - 0.96インチ OLEDディスプレイ（SSD1315） | 1 | 認識結果の表示 | https://www.switch-science.com/products/7002 |
| 4 | Raspberry Pi Pico ピンヘッダキット | 1 | Picoにはんだ付け | https://www.switch-science.com/products/6991 |
| 5 | GROVE - 4ピン-ジャンパオスケーブル（5本） | 1 |  センサー/OLEDの配線に2本使用（5本入りなので1つでOK） | https://www.switch-science.com/products/6245 |
| 6 | GROVE用 4ピン変換コネクタ（M5STACK-A099、10個入り） | 1 | ブレッドボード・センサー・OLEDのI2Cコネクタに直接挿せる変換、2個使用（10個入りなので1つでOK） | https://www.switch-science.com/products/7093 |
| 7 | ブレッドボード | 1 | 回路の土台 | （下記※1） |

## 手持ちで用意するもの

| 部品 | 用途 | 備考 |
|------|------|------|
| microUSBケーブル（データ通信対応） | スケッチ書き込み・給電 | ⚠️ Pico 2はmicro USB（USB-Cではない） |
| USB電源（ACアダプタ or モバイルバッテリー） | 動作中の給電 | 手持ちの汎用品でOK |

## 補足

### ※1 ブレッドボードについて
一般的なブレッドボード（400穴程度）で動作します。
市販の汎用品をお使いください。

### オリジナル基板・ケースについて
FuriCoro専用に設計したオリジナルブレッドボードとケースは
別途公開予定です（現在設計中）。

### ⚠️ Pico 2 W と WH について
ピンヘッダ未実装の「Pico 2 W」を使う場合、
ピンヘッダキット（#4）のはんだ付けが必要です。
はんだ付けを避けたい場合は、ピンヘッダ実装済みの
「Pico 2 WH」もありますが、コンテスト対象かは要確認です。
