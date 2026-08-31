# FuriCoro 🖐️

**Raspberry Pi Pico 2 W で作る、手を振るだけでリモート会議のリアクションを送るガジェット**

手をかざしてスワイプするだけで、Zoom や Teams のリアクション（拍手・いいね・ハートなど）を送れます。
カメラOFFでも使えて、ミュート切り替えや挙手もジェスチャーひとつ。
BLEキーボードとしてPCに繋がるので、ドライバもアプリのインストールも不要です。
なお、開発は生成AIを活用して行なっています。
「Coro」シリーズ第2弾。第1弾は [PomoCoro](https://github.com/inogit/PomoCoro) です。

---

## 📺 デモ



https://github.com/user-attachments/assets/48af5c12-4d48-47eb-b89f-51a5c0a4fe04



---

## ✨ 特徴

- **手をかざすだけ** — センサーの上で手をスワイプ・回転させるとリアクションが飛びます
- **カメラ不要** — 自分の姿を映さずにリアクションできる（Zoom内蔵ジェスチャーはカメラ必須）
- **7種類のリアクション** — Zoom内蔵は2種類のみ。FuriCoroは拍手・いいね・ハート・喜び・祝賀＋ミュート・挙手
- **ミュート・挙手も操作可能** — 手が塞がっていても手をかざすだけ
- **Zoom / Teams 対応、Windows / macOS 切替可能** — 設定モードでOS・アプリを切り替え、電源を切っても記憶
- **すべてジェスチャーで完結** — ボタン不要。誤操作が気になるときはセンサーを伏せて置くだけ

---

## 🕹️ 操作方法

### 通常モード

| ジェスチャー | Zoom | Teams |
|------------|------|-------|
| 上スワイプ | 拍手 👏 | 拍手 |
| 下スワイプ | いいね 👍 | いいね |
| 左スワイプ | ハート ❤️ | 驚き |
| 右スワイプ | 喜び 😂 | 笑顔 |
| 手を近づける | 祝賀 🎉 | 悲しい |
| 時計回り | ミュート切替 🔇 | ミュート切替 |
| 反時計回り | 挙手切替 ✋ | 挙手切替 |
| 手を引く | 設定モードへ | 設定モードへ |

### 設定モード（手を引くと入る）

| ジェスチャー | 動作 |
|------------|------|
| 上/下スワイプ | OS切替（Windows ⇄ macOS）|
| 左/右スワイプ | アプリ切替（Zoom ⇄ Teams）|
| 手を近づける | 確定して通常モードへ戻る |

設定はフラッシュに保存され、電源を切っても記憶されます。

---

## 🛒 必要なパーツ

| パーツ | 購入先 |
|-------|-------|
| Raspberry Pi Pico 2 W | [スイッチサイエンス](https://www.switch-science.com/products/10053) |
| GROVE - ジェスチャー（PAJ7620U2） | [スイッチサイエンス](https://www.switch-science.com/products/2645) |
| Grove - 0.96インチ OLED（SSD1315） | [スイッチサイエンス](https://www.switch-science.com/products/7002) |
| Raspberry Pi Pico ピンヘッダキット | [スイッチサイエンス](https://www.switch-science.com/products/6991) |
| GROVE - 4ピン-ジャンパオスケーブル | [スイッチサイエンス](https://www.switch-science.com/products/6245) |
| GROVE用 4ピン変換コネクタ（M5STACK-A099） | [スイッチサイエンス](https://www.switch-science.com/products/7093) |
| ブレッドボード | 市販の汎用品でOK |
| microUSBケーブル（データ通信対応） | 手持ちでOK（⚠️ Pico 2 は micro USB）|

詳細は [docs/パーツリスト.md](docs/パーツリスト.md) を参照。

---

## 🔌 配線

⚠️ **センサーの電源は必ず 3V3(OUT) から取ってください**（5VだとI2Cバスに5Vが乗り、RP2350を痛める可能性があります）

| 信号 | Pico | → センサー / OLED |
|------|------|------------------|
| 3.3V | 36番ピン（3V3 OUT） | VCC |
| GND  | 38番ピン | GND |
| SDA  | 6番ピン（GP4） | SDA |
| SCL  | 7番ピン（GP5） | SCL |

I2Cアドレス: センサー `0x73` / OLED `0x3C`

各信号はPicoの1ピンからセンサーとOLEDの2つに分岐します。
詳しい分岐方法は [docs/FuriCoro配線表.md](docs/FuriCoro配線表.md) を参照。

---

## 🛠️ 開発環境のセットアップ

### 1. Arduino IDE に arduino-pico を追加

`ファイル → 基本設定 → 追加のボードマネージャのURL` に以下を追加：

```
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
```

ボードマネージャで **Raspberry Pi Pico/RP2040/RP2350**（作者: Earle F. Philhower, III）をインストール。
⚠️ Arduino公式の Mbed 版ではなく、**Philhower版**を使ってください（BLE HIDが使えます）。

### 2. ボードと設定

- ボード: **Raspberry Pi Pico 2 W**
- ★ **Tools → IP/Bluetooth Stack → IPv4 + Bluetooth**（これを忘れると `KeyboardBLE.h` が見つからずコンパイルエラーになります）

### 3. ライブラリ

ライブラリマネージャで以下をインストール：

- DFRobot_PAJ7620U2
- U8g2（作者: oliver）

---

## 📝 書き込み手順

1. Pico の白いボタン（BOOTSEL）を押しながらUSB接続 → `RPI-RP2` ドライブが現れる
2. スケッチを書き込む
3. **PCのBluetooth設定で「FuriCoro」をペアリング**

---

## 📌 作り方のコツ

- **★ IP/Bluetooth Stack を「IPv4 + Bluetooth」に設定** — これを忘れるとコンパイルできません
- **センサーの電源は 3V3(OUT) から** — 5Vは厳禁
- **書き込み後は再ペアリング** — スケッチを書き込むとPicoのBLE識別情報が変わるため、PC側で「FuriCoro」を一度削除して再ペアリングしてください。OLEDにジェスチャーが表示されるのにZoomが反応しないときは、まずこれを試します。完成後、普段使う分には最初の1回だけでOKです
- **回転ジェスチャー（ミュート・挙手）は人差し指1本で** — 手のひら全体だと回転前の動きがスワイプと誤認識されやすいです
- **ミュートと挙手はグローバル対応** — Zoomの設定で「グローバルショートカットを有効にする」をONにすると、Zoomが背面でも効きます。リアクションはZoomが最前面のときだけ効きます

---

## 🎛️ Zoomショートカットの制約

| 機能 | グローバル | 動作条件 |
|------|:---------:|---------|
| ミュート・挙手 | ✅ 可 | Zoomが背面でもOK（OLEDに `GLOBAL` 表示）|
| リアクション5種 | ❌ 不可 | Zoomが最前面のときだけ（OLEDに `NEEDS FOCUS` 表示）|

---

## 📂 ファイル構成

```
FuriCoro/
├── README.md
├── FuriCoro/
│   └── FuriCoro.ino          # メインスケッチ
└── docs/
    ├── パーツリスト.md
    └── FuriCoro配線表.md
```

※ 専用パネル（3Dプリント）とケースは現在設計中です。完成次第このリポジトリに追加します。

---

## 📄 ライセンス

MIT License

---

## 🙏 使用しているライブラリ

| 名称 | 用途 |
|------|------|
| [arduino-pico](https://github.com/earlephilhower/arduino-pico)（KeyboardBLE） | BLE HIDキーボード |
| [DFRobot_PAJ7620U2](https://github.com/DFRobot/DFRobot_PAJ7620U2) | ジェスチャー検出 |
| [U8g2](https://github.com/olikraus/u8g2) | OLED表示 |
