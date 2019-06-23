#include <BTHID.h>
//#include "KeyboardParser.h"

#include <HID.h>
#include <SPI.h>

#define REPORT_KEYS 6
typedef struct {
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[REPORT_KEYS];
} KeyReport;

USB Usb;
BTD Btd(&Usb); // You have to create the Bluetooth Dongle instance like so

// 初回のペアリング時み
//BTHID bthid(&Btd, PAIR, "0000");

// 2回目以降はこちら
BTHID bthid(&Btd);

static const uint8_t UsToJisTable[] = {
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
  0x2a,0x02,  0x4c,0x00,  // +
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
  0
};

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
  0x29, 0xe7, //     USAGE_MAXIMUM (Keypad Hexadecimalまで)
  0x81, 0x00, //   INPUT (Data,Ary,Abs)
  0xc0    // END_COLLECTION
};

KeyReport keyReport;
bool shiftmodify;

void sendReport() {
  HID().SendReport(2, &keyReport ,sizeof(KeyReport));
}

void releaseAll() {
  memset(&keyReport, 0, sizeof(KeyReport));
  sendReport();
}

uint8_t convertkey(uint8_t key, uint8_t mod) {
   int i = 0;


   for (i=0; i< sizeof(UsToJisTable)/sizeof(UsToJisTable[0]); i=i+4 ){
     if ((key == UsToJisTable[i]) && (mod == UsToJisTable[i+1])) {
       if (mod != UsToJisTable[i+3]) {
	 shiftmodify = true;
	 keyReport.modifiers = UsToJisTable[i+3];
       }
       return UsToJisTable[i+2];
     }

   }

  return key;

}

void report_press(uint8_t key, uint8_t mod) {
  shiftmodify = false;
  PrintHex<uint8_t>(key, 0x80);
  PrintHex<uint8_t>(mod, 0x80);

  if (key != 0) {
    bool already = false;
    int empty_slot = -1;
    for(int i = 0; i < REPORT_KEYS; i++) {
      if (keyReport.keys[i] == convertkey(key,mod)) {
     // if (keyReport.keys[i] == key) {
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

void report_release(uint8_t key, uint8_t mod) {
  if (key != 0) {
    for(int i = 0; i < REPORT_KEYS; i++) {
      if (keyReport.keys[i] == convertkey(key,mod)) {
//      if (keyReport.keys[i] == key) {
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

KeyboardEvent kbd;

void error_blink(int period) {
  for(;;) {
    TXLED1; delay(period); TXLED0; delay(period);
    RXLED1; delay(period); RXLED0; delay(period);
  }
}


void setup() {

  // If "Boot Protocol Mode" does not work, then try "Report Protocol Mode"
  // If that does not work either, then uncomment PRINTREPORT in BTHID.cpp to see the raw report
  //bthid.setProtocolMode(USB_HID_BOOT_PROTOCOL); // Boot Protocol Mode
  bthid.setProtocolMode(HID_RPT_PROTOCOL); // Report Protocol Mode

  static HIDSubDescriptor node(_hidReportDescriptor, sizeof(_hidReportDescriptor));
  HID().AppendDescriptor(&node);
  delay(200);
  releaseAll();
  if (Usb.Init() == -1)
    error_blink(400);
  delay( 200 );
  bthid.SetReportParser(KEYBOARD_PARSER_ID, &kbd);

}

// USB Host Shield の状態と合わせる為に必要
void loop() {
  Usb.Task();
}
