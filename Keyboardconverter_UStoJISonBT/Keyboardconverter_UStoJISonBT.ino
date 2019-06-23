#include <BTHID.h>
#include "KeyboardParser.h"

#include <SPI.h>

USB Usb;
BTD Btd(&Usb); // You have to create the Bluetooth Dongle instance like so

// 初回のペアリング時み
BTHID bthid(&Btd, PAIR, "0000");

// 2回目以降はこちら
//BTHID bthid(&Btd);

KbdRptParser keyboardPrs;

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
  if (Usb.Init() == -1) {
    Serial.print(F("\r\nOSC did not start"));
    while (1); // Halt
  }

  bthid.SetReportParser(KEYBOARD_PARSER_ID, &keyboardPrs);

  // If "Boot Protocol Mode" does not work, then try "Report Protocol Mode"
  // If that does not work either, then uncomment PRINTREPORT in BTHID.cpp to see the raw report
  //bthid.setProtocolMode(USB_HID_BOOT_PROTOCOL); // Boot Protocol Mode
  bthid.setProtocolMode(HID_RPT_PROTOCOL); // Report Protocol Mode

  Serial.print(F("\r\nHID Bluetooth Library Started"));
}

// USB Host Shield の状態と合わせる為に必要
void loop() {
  Usb.Task();
}
