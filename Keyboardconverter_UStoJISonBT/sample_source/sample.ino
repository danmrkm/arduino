#include "HID.h"

#include <hidboot.h>

#include <SPI.h>

static const uint8_t _hidReportDescriptor[] PROGMEM = {
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
  0x29, 0xdd, //     USAGE_MAXIMUM (Keypad Hexadecimalまで)
  0x81, 0x00, //   INPUT (Data,Ary,Abs)
  0xc0    // END_COLLECTION
};

#define REPORT_KEYS 6
typedef struct {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[REPORT_KEYS];
} KeyReport;

KeyReport keyReport;

void sendReport() {
  HID().SendReport(2, &keyReport ,sizeof(KeyReport));
}

void releaseAll() {
  memset(&keyReport, 0, sizeof(KeyReport));
  sendReport();
}

void report_press(uint8_t key, uint8_t mod) {
  if (key != 0) {
    bool already = false;
    int empty_slot = -1;
    for(int i = 0; i < REPORT_KEYS; i++) {
      if (keyReport.keys[i] == key)
        already = true;
      if (keyReport.keys[i] == 0 && empty_slot < 0)
        empty_slot = i;
    }
    if (empty_slot < 0)  // error condition.
      return;
    if (!already)
      keyReport.keys[empty_slot] = key;
  }
  keyReport.modifiers = mod;
  sendReport();
}

void report_release(uint8_t key, uint8_t mod) {
  if (key != 0) {
    for(int i = 0; i < REPORT_KEYS; i++) {
      if (keyReport.keys[i] == key) {
        keyReport.keys[i] = 0;
        break;
      }
    }
  }
  keyReport.modifiers = mod;
  sendReport();
}

class KeyboardEvent : public KeyboardReportParser {
  protected:
    void OnControlKeysChanged(uint8_t before, uint8_t after);
    void OnKeyPressed(uint8_t key) {};
    void OnKeyDown  (uint8_t mod, uint8_t key) { report_press(key, mod);  };
    void OnKeyUp  (uint8_t mod, uint8_t key) { report_release(key, mod); };
};

void KeyboardEvent::OnControlKeysChanged(uint8_t before, uint8_t after) {
  uint8_t change = before ^ after;
  if (change != 0) {
    if (change & after)
      report_press(0, after);
    else
      report_release(0, after);
  }
}

USB Usb;
HIDBoot<USB_HID_PROTOCOL_KEYBOARD>    HidKeyboard(&Usb);
KeyboardEvent kbd;

void error_blink(int period) {
  for(;;) {
    TXLED1; delay(period); TXLED0; delay(period);
    RXLED1; delay(period); RXLED0; delay(period);
  }
}

void setup() {
  static HIDSubDescriptor node(_hidReportDescriptor, sizeof(_hidReportDescriptor));
  HID().AppendDescriptor(&node);
  delay(200);
  releaseAll();
  if (Usb.Init() == -1)
    error_blink(400);
  delay( 200 );
  HidKeyboard.SetReportParser(0, &kbd);
}

void loop() {
  Usb.Task();
}
