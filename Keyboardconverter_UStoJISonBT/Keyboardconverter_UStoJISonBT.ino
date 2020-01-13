#include <BTHID.h>
//#include "KeyboardParser.h"

#include <HID.h>
#include <SPI.h>

/* ================= 定数 ================= */

// US配列=>JIS配列変換
static const uint8_t UsToJisTable[] =
  {
   //  0x2c,0x00,  0x2c,0x00,  // (space)
   //  0x1e,0x01,  0x1e,0x01,  // !
   0x34,0x02,  0x1f,0x02,  // "
   //  0x20,0x01,  0x20,0x01,  // #
   //  0x21,0x01,  0x21,0x01,  // $
   //  0x22,0x01,  0x22,0x01,  // %
   0x24,0x02,  0x23,0x02,  // &
   0x34,0x00,  0x24,0x02,  // '
   0x26,0x02,  0x25,0x02,  // (
   0x27,0x02,  0x26,0x02,  // )
   0x25,0x02,  0x34,0x02,  // *
   0x2e,0x02,  0x33,0x02,  // +
   //  0x36,0x00,  0x36,0x00,  // ,
   //  0x2d,0x00,  0x2d,0x00,  // -
   //  0x37,0x00,  0x37,0x00,  // .
   //  0x38,0x00,  0x38,0x00,  // /
   0x33,0x02,  0x34,0x00,  // :
   //  0x33,0x00,  0x33,0x00,  // ;
   //  0x36,0x01,  0x36,0x01,  // <
   0x2e,0x00,  0x2d,0x02,  // =
   //  0x37,0x01,  0x37,0x01,  // >
   //  0x38,0x01,  0x38,0x01,  // ?
   0x1f,0x02,  0x2f,0x00,  // @
   0x2f,0x00,  0x30,0x00,  // [
   0x31,0x00,  0x89,0x00,  // BackSlash
   0x30,0x00,  0x32,0x00,  // ]
   0x23,0x02,  0x2e,0x00,  // ^
   0x2d,0x02,  0x87,0x02,  // _
   0x35,0x00,  0x2f,0x02,  // `
   0x2f,0x02,  0x30,0x02,  // {
   0x31,0x02,  0x89,0x02,  // |
   0x30,0x02,  0x32,0x02,  // }
   0x35,0x02,  0x2e,0x02,  // ~
   0x2a,0x02,  0x4c,0x00,  // Shift+BackSpace -> Delete
   0x2c,0x04,  0x35,0x04,  // Alt+Space -> Alt+Hankaku/Zenkaku
   0
  };

// HID 追加定義
static const uint8_t _hidReportDescriptor[] PROGMEM =
  {
   0x05, 0x01, // USAGE_PAGE (Generic Desktop)
   0x09, 0x06, // USAGE (Keyboard)
   0xa1, 0x01, // COLLECTION (Application)
   0x85, 0x02, //   REPORT_ID (2) --- Mouse.cppがID==1。

   0x05, 0x07, //   USAGE_PAGE (usage = keyboard page)
   // モデファイヤキー(修飾キー)
   0x19, 0xe0, //     USAGE_MINIMUM (左CTRLが0xe0)
   0x29, 0xe7, //     USAGE_MAXIMUM (右GUIが0xe7)
   0x15, 0x00, //   LOGICAL_MINIMUM (0)
   0x25, 0x01, //   LOGICAL_MAXIMUM (1)
   0x95, 0x08, //   REPORT_COUNT (8) 全部で8つ(左右4つずつ)。
   0x75, 0x01, //   REPORT_SIZE (1) 各修飾キーにつき1ビット
   0x81, 0x02, //   INPUT (Data,Var,Abs) 8ビット長のInputフィールド(変数)が1つ。
   // 予約フィールド
   0x95, 0x01, //   REPORT_COUNT (1)
   0x75, 0x08, //   REPORT_SIZE (8) 1ビットが8つ。

   0x81, 0x01, //   INPUT (Cnst,Var,Abs)
   // LED状態のアウトプット
   0x95, 0x05, //   REPORT_COUNT (5) 全部で5つ。
   0x75, 0x01, //   REPORT_SIZE (1)  各LEDにつき1ビット
   0x05, 0x08, //   USAGE_PAGE (LEDs)
   0x19, 0x01, //     USAGE_MINIMUM (1) (NumLock LEDが1)
   0x29, 0x05, //     USAGE_MAXIMUM (5) (KANA LEDが5)
   0x91, 0x02, //   OUTPUT (Data,Var,Abs) // LED report
   // LEDレポートのパディング
   0x95, 0x01, //   REPORT_COUNT (1)
   0x75, 0x03, //   REPORT_SIZE (3)　　残りの3ビットを埋める。
   0x91, 0x01, //   OUTPUT (Cnst,Var,Abs) // padding
   // 押下情報のインプット
   0x95, 0x06, //   REPORT_COUNT (6) 全部で6つ。
   0x75, 0x08, //   REPORT_SIZE (8) おのおの8ビットで表現
   0x15, 0x00, //   LOGICAL_MINIMUM (0) キーコードの範囲は、
   0x25, 0xdd, //   LOGICAL_MAXIMUM (221) 0～221(0xdd)まで

   0x05, 0x07, //   USAGE_PAGE (Keyboard)
   0x19, 0x00, //     USAGE_MINIMUM (0はキーコードではない)
   0x29, 0xe7, //     USAGE_MAXIMUM (Keypad Hexadecimalまで)
   0x81, 0x00, //   INPUT (Data,Ary,Abs)
   0xc0    // END_COLLECTION
  };

// 修飾キー定義
const uint8_t altkey = 0x04;
const uint8_t commandkey = 0x08;
const uint8_t nomod = 0x00;


// スイッチ用PIN
const int SWITCH_PIN = 7;
const uint8_t OUTPUT_BLUETOOTH = LOW;
const uint8_t OUTPUT_USB = HIGH;


/* ================= グローバル変数 ================= */

// HIDバッファ送信用
typedef struct {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
} KeyReport;

// キーレポート用
KeyReport keyReport;

// フラグ
bool shiftmodify;
bool lock_flag;
uint8_t last_switch_status;

// 修飾キー
uint8_t modif_key;
uint8_t main_key = 0x00;
uint8_t fn_key = false;

// 時間関連
unsigned long time;
unsigned long lasttime;
unsigned long last_switch_status_change_time;

/* ================= BTHID ================= */

USB Usb;
BTD Btd(&Usb); // You have to create the Bluetooth Dongle instance like so

// 初回のペアリング時み
//BTHID bthid(&Btd, PAIR, "0000");

// 2回目以降はこちら
BTHID bthid(&Btd);


/* ================= 関数部 ================= */


// USBにキーコード送信
void sendReport() {

  // スイッチが USB モードのときのみ、USB経由でキーコード送信
  if (digitalRead(SWITCH_PIN) == OUTPUT_USB) {
    // USB HID としてキーコードを送信
    HID().SendReport(2, &keyReport ,sizeof(KeyReport));
  }
}

// 全てのキーを解除
void releaseAll() {
  memset(&keyReport, 0, sizeof(KeyReport));
  sendReport();
}

// キー変換
uint8_t convertkey(uint8_t key, uint8_t mod) {
  int i = 0;

  for (i=0; i< sizeof(UsToJisTable)/sizeof(UsToJisTable[0]); i=i+4 ){
    if ((key == UsToJisTable[i]) && (mod == UsToJisTable[i+1])) {
      if (mod != UsToJisTable[i+3]) {
        shiftmodify = true;
        keyReport.modifiers = UsToJisTable[i+3];
        modif_key = UsToJisTable[i+3];
      }
      else {
        modif_key = mod;
      }
      return UsToJisTable[i+2];
    }

  }

  return key;

}

// キー押下
void report_press(uint8_t key, uint8_t mod) {
  shiftmodify = false;

  //  time = millis();

  if (mod == altkey) {
    mod = commandkey;
  }
  else if(mod == commandkey) {
    mod  = altkey;
  }

  if (key != 0) {
    bool already = false;
    int empty_slot = -1;
    for(int i = 0; i < 6; i++) {
      if (keyReport.keys[i] == convertkey(key,mod)) {
        already = true;
      }
      if (keyReport.keys[i] == 0 && empty_slot < 0) {
        empty_slot = i;
      }
    }
    if (empty_slot < 0) {  // error condition.
      return;
    }
    if (!already) {
      keyReport.keys[empty_slot] = convertkey(key,mod);
      //keyReport.keys[empty_slot] = key;
      if (!shiftmodify) {
  keyReport.modifiers = mod;
      }
    }
    else {
      keyReport.modifiers = mod;
    }

  }
  else {
    keyReport.modifiers = mod;
  }
  sendReport();
}

// キーリリース
void report_release(uint8_t key, uint8_t mod) {
  if (mod == altkey) {
    mod = commandkey;
  }
  else if (mod == commandkey) {
    mod  = altkey;
  }

  if (key != 0) {
    for(int i = 0; i < 6; i++) {
      if (keyReport.keys[i] == convertkey(key,mod)) {
  //      if (keyReport.keys[i] == key) {
        keyReport.keys[i] = 0;
        break;
      }
    }
  }
  if (shiftmodify) {
    keyReport.modifiers = modif_key;
  } else {
    keyReport.modifiers = mod;
  }
  sendReport();
}

// RN-42 にシリアル経由で HID コマンド送信
void sendKeyCodesBySerial(uint8_t modifiers,
        uint8_t keycode0,
        uint8_t keycode1,
        uint8_t keycode2,
        uint8_t keycode3,
        uint8_t keycode4,
        uint8_t keycode5
                          )
{
  // スイッチが BLUETOOTH モードのときのみ、RN-42 に送信
  if (digitalRead(SWITCH_PIN) == OUTPUT_BLUETOOTH) {

    Serial1.write(0xFD); // Raw Report Mode

    // 通常のキー

    Serial1.write(0x09); // Length
    Serial1.write(0x01); // Descriptor 0x01=Keyboard

    /* send key codes(8 bytes all) */
    Serial1.write(modifiers); // modifier keys
    Serial1.write(0x00);   // reserved
    Serial1.write(keycode0);  // keycode0
    Serial1.write(keycode1);  // keycode1
    Serial1.write(keycode2);  // keycode2
    Serial1.write(keycode3);  // keycode3
    Serial1.write(keycode4);  // keycode4
    Serial1.write(keycode5);  // keycode5


  }
}

// エラー時、LED 点灯
void error_blink(int period) {
  for(;;) {
    TXLED1; delay(period); TXLED0; delay(period);
    RXLED1; delay(period); RXLED0; delay(period);
  }
}

// KeyboardReportParser をオーバーライド
class KeyboardEvent : public KeyboardReportParser {
protected:
  void OnControlKeysChanged(uint8_t before, uint8_t after);
  void OnKeyPressed(uint8_t key) {};
  void OnKeyDown  (uint8_t mod, uint8_t key) { report_press(key, mod);  };
  void OnKeyUp  (uint8_t mod, uint8_t key) { report_release(key, mod); };
  void Parse(USBHID *hid, bool is_rpt_id __attribute__((unused)), uint8_t len __attribute__((unused)), uint8_t *buf);
};

// キーボードイベント
KeyboardEvent kbd;

// Controlキーの変更
void KeyboardEvent::OnControlKeysChanged(uint8_t before, uint8_t after) {
  uint8_t change = before ^ after;
  if (change != 0) {
    if (change & after)
      report_press(0, after);
    else
      report_release(0, after);
  }
}

// キーコードのパース
void KeyboardEvent::Parse(USBHID *hid, bool is_rpt_id __attribute__((unused)), uint8_t len __attribute__((unused)), uint8_t *buf){
  // On error - return
  if (buf[2] == 1)
    return;

  if (fn_key && buf[2] == 0x2a) {
    buf[2] = 0x4c;
  }

  //KBDINFO       *pki = (KBDINFO*)buf;

  // provide event for changed control key state
  if (prevState.bInfo[0x00] != buf[0x00]) {
    OnControlKeysChanged(prevState.bInfo[0x00], buf[0x00]);
  }

  for (uint8_t i = 2; i < 8; i++) {
    bool down = false;
    bool up = false;

    for (uint8_t j = 2; j < 8; j++) {
      if (buf[i] == prevState.bInfo[j] && buf[i] != 1)
  down = true;
      if (buf[j] == prevState.bInfo[i] && prevState.bInfo[i] != 1)
  up = true;
    }
    if (!down) {
      HandleLockingKeys(hid, buf[i]);
      OnKeyDown(*buf, buf[i]);
    }
    if (!up)
      OnKeyUp(prevState.bInfo[0], prevState.bInfo[i]);
  }
  for (uint8_t i = 0; i < 8; i++)
    prevState.bInfo[i] = buf[i];

  // FN Key
  if (len == 0x01) {

    // 押下
    if (buf[0] == 0x10) {
      fn_key = true;
    }
    // リリース
    else {
      fn_key = false;
    }

  }

  sendKeyCodesBySerial(buf[0], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

}


/* ================= 初回実行 ================= */

void setup() {

  //bthid.setProtocolMode(USB_HID_BOOT_PROTOCOL); // Boot Protocol Mode
  bthid.setProtocolMode(HID_RPT_PROTOCOL); // Report Protocol Mode

  // USB 側の HID を 初期化
  static HIDSubDescriptor node(_hidReportDescriptor, sizeof(_hidReportDescriptor));
  HID().AppendDescriptor(&node);
  delay(200);
  releaseAll();

  // USB との接続確認
  if (Usb.Init() == -1)
    error_blink(400);
  delay( 200 );

  // スイッチの設定
  pinMode( SWITCH_PIN, INPUT_PULLUP );

  last_switch_status = digitalRead(SWITCH_PIN);

  // BTHID の設定
  bthid.SetReportParser(KEYBOARD_PARSER_ID, &kbd);

  // 時刻変数の初期化
  time = millis();
  lasttime = millis();
  last_switch_status_change_time = millis();

  // USB 側の Serial (デバッグ用）
  Serial.begin(115200);
  // RN-42 側の Serial
  Serial1.begin(115200);


}

/* ================= ループ実行 ================= */

void loop() {
  // USB Host Shield の状態と合わせる
  Usb.Task();

  // デバック用、定期実行
  // if ((millis() - lasttime) > 5000) {

  //   lasttime = millis();
  // }

  // BTHIDリセット
  if (digitalRead(SWITCH_PIN) != last_switch_status) {

    // スイッチの ON/OFF が一秒以内に行われた場合、BTHID のリセットをかける
    if ((millis() - last_switch_status_change_time) < 1000) {
      Serial.print("BTHID Reset");
      bthid.disconnect();
      delay(500);
      bthid.setProtocolMode(HID_RPT_PROTOCOL);
      bthid.SetReportParser(KEYBOARD_PARSER_ID, &kbd);

    }

    last_switch_status = digitalRead(SWITCH_PIN);
    last_switch_status_change_time = millis();

  }

  // スリープ解除抑止
  if ((millis() - time) > 900000) {

    // ロック済の場合は ESC を送信
    if (lock_flag) {
      main_key = 0x29;
      report_press(main_key,nomod);
      report_release(main_key,nomod);
    }
    else {
      lock_flag = true;

      // Windows を Lockさせる
      // Windows キー
      main_key = 0xe3;
      report_press(main_key,nomod);
      report_release(main_key,nomod);
      delay( 500 );
      // Tab キー
      main_key = 0x2b;
      report_press(main_key,nomod);
      report_release(main_key,nomod);
      delay( 500 );
      // 下矢印 キー
      main_key = 0x51;
      report_press(main_key,nomod);
      report_release(main_key,nomod);
      delay( 500 );
      // Enter キー
      main_key = 0x28;
      report_press(main_key,nomod);
      report_release(main_key,nomod);
      delay( 500 );
      // 下矢印 キー
      main_key = 0x51;
      report_press(main_key,nomod);
      report_release(main_key,nomod);
      delay( 500 );
      // Enter キー
      main_key = 0x28;
      report_press(main_key,nomod);
      report_release(main_key,nomod);

    }

  }
  else {
    // ロックフラグをオフ
    lock_flag = false;

  }
}
