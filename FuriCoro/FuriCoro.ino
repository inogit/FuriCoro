/*
 * FuriCoro  -  Raspberry Pi Pico 2 W で作る
 *              手を振るだけでリモート会議のリアクションを送るガジェット
 *              (Zoom / Teams 対応、Windows / macOS 切替可能)
 * ============================================================
 * 手をかざして動かすと、BLEキーボードとして会議アプリのショートカットを送ります。
 * 
 * PCからは普通のBluetoothキーボードとして見えるため、
 * ドライバもソフトのインストールも不要です。
 *
 * すべての操作を手のジェスチャーで完結させています（ボタン不要）。
 *
 * ------------------------------------------------------------
 * 【必要なハードウェア】
 *   - Raspberry Pi Pico 2 W  (ピンヘッダはんだ付け済み)
 *   - GROVE ジェスチャーセンサ (PAJ7620U2)   I2C 0x73
 *   - GROVE 0.96インチ OLED  (SSD1315/SSD1306) I2C 0x3C
 *
 * 【配線】  ※センサの電源は必ず 3V3(OUT) から取ること
 *   Pico 3V3(OUT) 36番ピン --- ジェスチャ VCC / OLED VCC
 *   Pico GND      38番ピン --- ジェスチャ GND / OLED GND
 *   Pico GP4      6番ピン  --- ジェスチャ SDA / OLED SDA
 *   Pico GP5      7番ピン  --- ジェスチャ SCL / OLED SCL
 *
 * ------------------------------------------------------------
 * 【Arduino IDE の設定】
 *   1. ボード      : Raspberry Pi Pico 2 W
 *   2. ★ IP/Bluetooth Stack : "IPv4 + Bluetooth"
 *
 * 【必要ライブラリ】
 *   - DFRobot_PAJ7620U2
 *   - U8g2  (oliver)
 *
 * ------------------------------------------------------------
 * 【操作方法】
 *   ● 通常モード ────────────────────────
 *     上スワイプ      → 拍手
 *     下スワイプ      → サムズアップ
 *     左スワイプ      → ハート
 *     右スワイプ      → 笑顔
 *     手を近づける    → 祝賀        (Forward)
 *     時計回り        → ミュート切替
 *     反時計回り      → 挙手切替
 *     手を引く        → 設定モードへ (Backward)
 *
 *   ● 設定モード ────────────────────────
 *     上/下スワイプ   → OS切替  (Windows <-> macOS)
 *     左/右スワイプ   → アプリ切替 (Zoom <-> Teams)
 *     手を近づける    → 確定して通常モードへ (Forward)
 *     設定はフラッシュに保存され、電源を切っても記憶されます
 *
 * ------------------------------------------------------------
 * 【使い方】
 *   1. 書き込み後、PCのBluetooth設定で「FuriCoro」をペアリング
 *      ※スケッチを書き込み直したら、PC側でデバイスを削除して
 *        再ペアリングしてください（開発中のみ必要な操作です）
 *   2. Zoom → 設定 → キーボードショートカット で
 *      挙手・ミュートの「グローバルショートカットを有効にする」をON
 *   3. センサの5〜15cm上で手を動かす
 *
 * 【Zoomショートカットの制約】
 *   挙手・ミュート → グローバル対応。Zoomが背面でも効く (OLEDに GLOBAL)
 *   リアクション   → Zoomが最前面のときだけ効く (OLEDに NEEDS FOCUS)
 * ============================================================
 */

#include <Wire.h>
#include <KeyboardBLE.h>
#include <U8g2lib.h>
#include <DFRobot_PAJ7620U2.h>
#include <EEPROM.h>

// ============================================================
//  ★ ユーザー設定エリア
// ============================================================

// BLEデバイス名 (PCのBluetooth一覧に表示される名前)
#define BLE_DEVICE_NAME "FuriCoro"

// I2Cピン
#define PIN_SDA 4
#define PIN_SCL 5

// 起動時のデフォルト (初回のみ。以降はフラッシュ保存値を使用)
#define DEFAULT_OS  0   // 0 = Windows / 1 = macOS
#define DEFAULT_APP 0   // 0 = Zoom    / 1 = Teams

// --- 誤検知対策のパラメータ ---
const unsigned long COOLDOWN_ALL_MS  = 1500;  // 発火後、全ジェスチャー無視
const unsigned long COOLDOWN_SAME_MS = 3000;  // 同一ジェスチャー再受付まで
const unsigned long TOAST_MS         = 2000;  // 送信結果の表示時間

// ============================================================
//  キーコンボ定義
// ============================================================
struct KeyCombo {
  uint8_t mod1;
  uint8_t mod2;
  uint8_t key;
};

// アクションの種類 (OS×アプリで実際のキーが変わる)
enum ActionId {
  ACT_CLAP = 0, ACT_THUMBS, ACT_HEART, ACT_SMILE,
  ACT_PARTY, ACT_HAND, ACT_MUTE,
  ACT_COUNT
};

// ------------------------------------------------------------
//  ショートカットテーブル  [OS][APP][アクション]
//  OS:  0=Windows 1=macOS
//  APP: 0=Zoom    1=Teams
//
//  ⚠️ Zoom(Windows) 以外は実機で未検証の値を含みます。
//     各アプリの設定画面で確認して修正してください。
//     特に Teams のリアクション系は要確認です。
// ------------------------------------------------------------
const KeyCombo SHORTCUTS[2][2][ACT_COUNT] = {
  // ===== Windows =====
  {
    // --- Windows / Zoom ---  (実測済み)
    {
      { KEY_LEFT_ALT, KEY_LEFT_SHIFT, '4' }, // CLAP
      { KEY_LEFT_ALT, KEY_LEFT_SHIFT, '5' }, // THUMBS
      { KEY_LEFT_ALT, KEY_LEFT_SHIFT, '6' }, // HEART
      { KEY_LEFT_ALT, KEY_LEFT_SHIFT, '7' }, // SMILE
      { KEY_LEFT_ALT, KEY_LEFT_SHIFT, '9' }, // PARTY
      { KEY_LEFT_ALT, 0,              'y' }, // HAND  (Alt+Y)
      { KEY_LEFT_ALT, 0,              'a' }, // MUTE  (Alt+A)
    },
    // --- Windows / Teams ---  ⚠️要確認
    {
      { KEY_LEFT_CTRL, KEY_LEFT_SHIFT, '1' }, // CLAP   ?
      { KEY_LEFT_CTRL, KEY_LEFT_SHIFT, '2' }, // THUMBS ?
      { KEY_LEFT_CTRL, KEY_LEFT_SHIFT, '3' }, // HEART  ?
      { KEY_LEFT_CTRL, KEY_LEFT_SHIFT, '4' }, // SMILE  ?
      { KEY_LEFT_CTRL, KEY_LEFT_SHIFT, '5' }, // PARTY  ?
      { KEY_LEFT_CTRL, KEY_LEFT_SHIFT, 'k' }, // HAND  (Ctrl+Shift+K)
      { KEY_LEFT_CTRL, KEY_LEFT_SHIFT, 'm' }, // MUTE  (Ctrl+Shift+M)
    },
  },
  // ===== macOS =====
  {
    // --- macOS / Zoom ---  ⚠️要確認
    {
      { KEY_LEFT_ALT, KEY_LEFT_SHIFT, '4' }, // CLAP   ?
      { KEY_LEFT_ALT, KEY_LEFT_SHIFT, '5' }, // THUMBS ?
      { KEY_LEFT_ALT, KEY_LEFT_SHIFT, '6' }, // HEART  ?
      { KEY_LEFT_ALT, KEY_LEFT_SHIFT, '7' }, // SMILE  ?
      { KEY_LEFT_ALT, KEY_LEFT_SHIFT, '9' }, // PARTY  ?
      { KEY_LEFT_ALT, 0,              'y' }, // HAND   ?
      { KEY_LEFT_GUI, KEY_LEFT_SHIFT, 'a' }, // MUTE  (Cmd+Shift+A)
    },
    // --- macOS / Teams ---  ⚠️要確認
    {
      { KEY_LEFT_GUI, KEY_LEFT_SHIFT, '1' }, // CLAP   ?
      { KEY_LEFT_GUI, KEY_LEFT_SHIFT, '2' }, // THUMBS ?
      { KEY_LEFT_GUI, KEY_LEFT_SHIFT, '3' }, // HEART  ?
      { KEY_LEFT_GUI, KEY_LEFT_SHIFT, '4' }, // SMILE  ?
      { KEY_LEFT_GUI, KEY_LEFT_SHIFT, '5' }, // PARTY  ?
      { KEY_LEFT_GUI, KEY_LEFT_SHIFT, 'k' }, // HAND  (Cmd+Shift+K)
      { KEY_LEFT_GUI, KEY_LEFT_SHIFT, 'm' }, // MUTE  (Cmd+Shift+M)
    },
  },
};

// アクションのメタ情報 (表示名とグローバル可否)
struct ActionMeta {
  const char* label;
  const char* sub;
  bool        isGlobal;   // Zoomが背面でも効くか (挙手・ミュートのみ true)
};
const ActionMeta ACT_META[ACT_COUNT] = {
  { "CLAP",  "applause",    false },
  { "GOOD",  "thumbs up",   false },
  { "HEART", "heart",       false },
  { "SMILE", "smile",       false },
  { "PARTY", "celebrate",   false },
  { "HAND",  "raise hand",  true  },
  { "MUTE",  "mute toggle", true  },
};

// ジェスチャー → アクション の割り当て (通常モード)
typedef DFRobot_PAJ7620U2::eGesture_t Gesture;
struct GestureMap { Gesture g; ActionId act; };
const GestureMap GESTURE_MAP[] = {
  { DFRobot_PAJ7620U2::eGestureUp,            ACT_CLAP   },
  { DFRobot_PAJ7620U2::eGestureDown,          ACT_THUMBS },
  { DFRobot_PAJ7620U2::eGestureLeft,          ACT_HEART  },
  { DFRobot_PAJ7620U2::eGestureRight,         ACT_SMILE  },
  { DFRobot_PAJ7620U2::eGestureForward,       ACT_PARTY  },
  { DFRobot_PAJ7620U2::eGestureClockwise,     ACT_MUTE   },
  { DFRobot_PAJ7620U2::eGestureAntiClockwise, ACT_HAND   },
};
const int GESTURE_MAP_COUNT = sizeof(GESTURE_MAP) / sizeof(GESTURE_MAP[0]);

// ============================================================
//  グローバル
// ============================================================
DFRobot_PAJ7620U2 paj;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

bool  gSensorOK  = false;
int   gOS        = DEFAULT_OS;   // 0=Win 1=Mac
int   gApp       = DEFAULT_APP;  // 0=Zoom 1=Teams
bool  gSetupMode = false;        // 設定モード中か

unsigned long gLastFireMs  = 0;
Gesture       gLastGesture = DFRobot_PAJ7620U2::eGestureNone;
unsigned long gLastSameMs  = 0;

const char*   gToastLabel  = nullptr;
const char*   gToastSub    = nullptr;
bool          gToastGlobal = false;
unsigned long gToastUntil  = 0;

uint16_t      gSendCount   = 0;

// EEPROM(フラッシュ)保存
const int EE_ADDR_MAGIC = 0;
const int EE_ADDR_OS    = 1;
const int EE_ADDR_APP   = 2;
const uint8_t EE_MAGIC  = 0xF3;   // 初期化判定用

// ============================================================
//  設定の保存 / 読み込み
// ============================================================
void saveSettings() {
  EEPROM.write(EE_ADDR_MAGIC, EE_MAGIC);
  EEPROM.write(EE_ADDR_OS,    (uint8_t)gOS);
  EEPROM.write(EE_ADDR_APP,   (uint8_t)gApp);
  EEPROM.commit();
}

void loadSettings() {
  if (EEPROM.read(EE_ADDR_MAGIC) == EE_MAGIC) {
    gOS  = EEPROM.read(EE_ADDR_OS)  & 0x01;
    gApp = EEPROM.read(EE_ADDR_APP) & 0x01;
  } else {
    gOS  = DEFAULT_OS;
    gApp = DEFAULT_APP;
  }
}

const char* osName()  { return gOS  ? "MAC"  : "WIN";  }
const char* appName() { return gApp ? "TEAMS": "ZOOM"; }

// ============================================================
//  キー送信
// ============================================================
void sendCombo(const KeyCombo& kc) {
  if (kc.mod1) KeyboardBLE.press(kc.mod1);
  if (kc.mod2) KeyboardBLE.press(kc.mod2);
  KeyboardBLE.press(kc.key);
  delay(30);
  KeyboardBLE.releaseAll();
}

void fireAction(ActionId act) {
  sendCombo(SHORTCUTS[gOS][gApp][act]);
  gSendCount++;

  const ActionMeta& m = ACT_META[act];
  gToastLabel  = m.label;
  gToastSub    = m.sub;
  gToastGlobal = m.isGlobal;
  gToastUntil  = millis() + TOAST_MS;

  digitalWrite(LED_BUILTIN, HIGH);
  delay(40);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.print("sent: ");
  Serial.print(m.label);
  Serial.print("  [");
  Serial.print(osName()); Serial.print("/"); Serial.print(appName());
  Serial.println("]");
}

// ============================================================
//  OLED 描画
// ============================================================
void drawSetup() {
  u8g2.clearBuffer();

  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(30, 8, "-- SETUP --");
  u8g2.drawHLine(0, 11, 128);

  // OS 行
  u8g2.setFont(u8g2_font_ncenB14_tr);
  char line1[20];
  snprintf(line1, sizeof(line1), "OS:  [%s]", osName());
  u8g2.drawStr(6, 32, line1);

  // APP 行
  char line2[20];
  snprintf(line2, sizeof(line2), "APP: [%s]", appName());
  u8g2.drawStr(6, 50, line2);

  // フッター
  u8g2.setFont(u8g2_font_5x8_tf);
  u8g2.drawStr(2, 62, "U/D:OS  L/R:APP  Fwd:OK");

  u8g2.sendBuffer();
}

void drawNormal() {
  u8g2.clearBuffer();

  // --- 送信直後のトースト ---
  if (gToastLabel && millis() < gToastUntil) {
    u8g2.setFont(u8g2_font_ncenB18_tr);
    int w = u8g2.getStrWidth(gToastLabel);
    u8g2.drawStr((128 - w) / 2, 22, gToastLabel);

    if (gToastSub) {
      u8g2.setFont(u8g2_font_6x10_tf);
      int sw = u8g2.getStrWidth(gToastSub);
      u8g2.drawStr((128 - sw) / 2, 42, gToastSub);

      // グローバル可否マーカー
      if (gToastGlobal) {
        const char* m = "GLOBAL";
        int mw = u8g2.getStrWidth(m);
        int bx = (128 - mw) / 2 - 4;
        u8g2.drawBox(bx, 46, mw + 8, 11);
        u8g2.setDrawColor(0);
        u8g2.drawStr((128 - mw) / 2, 48, m);
        u8g2.setDrawColor(1);
      } else {
        const char* m = "NEEDS FOCUS";
        int mw = u8g2.getStrWidth(m);
        u8g2.drawStr((128 - mw) / 2, 48, m);
      }
    }

    // クールダウンバー
    unsigned long elapsed = millis() - gLastFireMs;
    if (elapsed < COOLDOWN_ALL_MS) {
      int bw = 120 - (int)(120L * elapsed / COOLDOWN_ALL_MS);
      u8g2.drawBox(4, 60, bw, 3);
    }
    u8g2.sendBuffer();
    return;
  }

  // --- 待機画面 ---
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 8, "FuriCoro");
  u8g2.drawHLine(0, 11, 128);

  u8g2.setFont(u8g2_font_ncenB14_tr);
  if (gSensorOK) u8g2.drawStr(30, 34, "READY");
  else           u8g2.drawStr(12, 34, "NO SENSOR");

  // 現在のモードと送信数
  u8g2.setFont(u8g2_font_6x10_tf);
  char buf[24];
  snprintf(buf, sizeof(buf), "%s / %s", osName(), appName());
  u8g2.drawStr(0, 52, buf);
  snprintf(buf, sizeof(buf), "sent: %u", gSendCount);
  u8g2.drawStr(0, 62, buf);

  u8g2.sendBuffer();
}

void drawScreen() {
  if (gSetupMode) drawSetup();
  else            drawNormal();
}

// ============================================================
//  設定モードのジェスチャー処理
// ============================================================
void handleSetupGesture(Gesture g) {
  switch (g) {
    case DFRobot_PAJ7620U2::eGestureUp:
    case DFRobot_PAJ7620U2::eGestureDown:
      gOS = !gOS;                       // OSトグル
      Serial.print("OS -> "); Serial.println(osName());
      break;
    case DFRobot_PAJ7620U2::eGestureLeft:
    case DFRobot_PAJ7620U2::eGestureRight:
      gApp = !gApp;                     // アプリトグル
      Serial.print("APP -> "); Serial.println(appName());
      break;
    case DFRobot_PAJ7620U2::eGestureForward:
      // 確定して通常モードへ
      saveSettings();
      gSetupMode  = false;
      gToastLabel = "SAVED";
      gToastSub   = nullptr;
      gToastGlobal= false;
      gToastUntil = millis() + 1200;
      Serial.println("settings saved");
      break;
    default:
      break;   // 回転・Backwardは設定モードでは無視
  }
}

// ============================================================
//  通常モードのジェスチャー処理
// ============================================================
ActionId lookupAction(Gesture g, bool* found) {
  for (int i = 0; i < GESTURE_MAP_COUNT; i++) {
    if (GESTURE_MAP[i].g == g) { *found = true; return GESTURE_MAP[i].act; }
  }
  *found = false;
  return ACT_CLAP;
}

void handleNormalGesture(Gesture g) {
  unsigned long now = millis();

  // Backward = 設定モードに入る
  if (g == DFRobot_PAJ7620U2::eGestureBackward) {
    gSetupMode  = true;
    gToastLabel = nullptr;
    Serial.println("enter SETUP mode");
    return;
  }

  // クールダウン
  if (now - gLastFireMs < COOLDOWN_ALL_MS) return;

  bool found = false;
  ActionId act = lookupAction(g, &found);
  if (!found) return;

  if (g == gLastGesture && now - gLastSameMs < COOLDOWN_SAME_MS) return;

  fireAction(act);
  gLastFireMs  = now;
  gLastGesture = g;
  gLastSameMs  = now;
}

// ============================================================
//  setup()
// ============================================================
void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  EEPROM.begin(16);
  loadSettings();

  Wire.setSDA(PIN_SDA);
  Wire.setSCL(PIN_SCL);
  Wire.begin();

  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB14_tr);
  u8g2.drawStr(4, 30, "FuriCoro");
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(4, 48, "starting...");
  u8g2.sendBuffer();

  for (int i = 0; i < 10; i++) {
    if (paj.begin() == 0) { gSensorOK = true; break; }
    Serial.println("PAJ7620U2 init retry...");
    delay(500);
  }
  if (gSensorOK) {
    paj.setGestureHighRate(true);
    Serial.println("PAJ7620U2 ready");
  } else {
    Serial.println("PAJ7620U2 NOT FOUND - check wiring / 3V3 power");
  }

  KeyboardBLE.begin(BLE_DEVICE_NAME);
  Serial.println("BLE advertising as \"" BLE_DEVICE_NAME "\" - pair from your PC");

  delay(600);
  drawScreen();
}

// ============================================================
//  loop()
// ============================================================
void loop() {
  if (gSensorOK) {
    Gesture g = paj.getGesture();
    if (g != DFRobot_PAJ7620U2::eGestureNone) {
      if (gSetupMode) handleSetupGesture(g);
      else            handleNormalGesture(g);
    }
  }

  drawScreen();
  delay(50);
}
