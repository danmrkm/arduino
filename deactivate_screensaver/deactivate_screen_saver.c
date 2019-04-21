#include "Mouse.h"

// ピン設定
// Switch用信号出力ピン（出力固定）
const int SWITCH_OUTPUT_PIN = 9;
// Switch用信号入力ピン
const int SWITCH_INPUT_PIN = 6;

// インターバル設定
// マウス動作間隔（秒数）
const int MOUSE_MOVE_INTERVAL = 270;
// マウス動作ピクセル数
const int MOUSE_MOVE_PIXEL = 3;
// マウス動作戻し間隔（ミリ秒）
const int MOUSE_RETURN_INTERVAL = 500;

// Initialize
void setup() {

  // ピンの初期設定
  pinMode(SWITCH_OUTPUT_PIN, OUTPUT);
  pinMode(SWITCH_INPUT_PIN, INPUT);

  // 出力用ピンを設定
  digitalWrite(SWITCH_OUTPUT_PIN, HIGH);
  // マウス用設定初期化
  Mouse.begin();

}

// ループ関数
void loop() {
  // スイッチがONの時のみ実行
  if (digitalRead(SWITCH_INPUT_PIN) == HIGH) {
    // マウスを指定したピクセル分だけ動かす
    Mouse.move(0, -MOUSE_MOVE_PIXEL);
    delay(MOUSE_RETURN_INTERVAL);
    // 動かしたマウスを元に戻す
    Mouse.move(0, MOUSE_MOVE_PIXEL);
  }

  // 指定時間だけ待つ
  delay(MOUSE_MOVE_INTERVAL * 1000);
}
