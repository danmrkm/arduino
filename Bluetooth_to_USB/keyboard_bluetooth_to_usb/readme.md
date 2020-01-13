Apple Wireless Keyboard を利用する場合は、USB Host Shield 2.0 内の BTHID.h において、以下の修正を行うこと

`switch(l2capinbuf[9]) {` から始まる L2CAP のレポートタイプの Switch 文において、Keyboard (0x01) の case 文をコピーし、0x11 の case 文を作成する。

Sample
```
switch(l2capinbuf[9]) {
        case 0x01: // Keyboard or Joystick events
                if(pRptParser[KEYBOARD_PARSER_ID])
                        pRptParser[KEYBOARD_PARSER_ID]->Parse(reinterpret_cast<USBHID *>(this), 0, (uint8_t)(length - 2), &l2capinbuf[10]); // Use reinterpret_cast again to extract the instance
                break;

        case 0x11: // Mac Keyboard or Joystick events
                if(pRptParser[KEYBOARD_PARSER_ID])
                        pRptParser[KEYBOARD_PARSER_ID]->Parse(reinterpret_cast<USBHID *>(this), 0, (uint8_t)(length - 2), &l2capinbuf[10]); // Use reinterpret_cast again to extract the instance
                break;

        case 0x02: // Mouse events
                if(pRptParser[MOUSE_PARSER_ID])
                        pRptParser[MOUSE_PARSER_ID]->Parse(reinterpret_cast<USBHID *>(this), 0, (uint8_t)(length - 2), &l2capinbuf[10]); // Use reinterpret_cast again to extract the instance
                break;
#ifdef EXTRADEBUG
        default:
                Notify(PSTR("\r\nUnknown Report type: "), 0x80);
                D_PrintHex<uint8_t > (l2capinbuf[9], 0x80);
                break;
#endif
```
