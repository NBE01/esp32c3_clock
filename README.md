# XIAO ESP32-C3 OLED Clock

Seeed Studio XIAO ESP32-C3の内蔵システム時計をI2C OLEDに表示するPlatformIO / Arduinoプロジェクトです。Wi-Fi接続は使用しません。

## 配線

| OLED | XIAO ESP32-C3 |
| --- | --- |
| SDA | D4 / GPIO6 |
| SCL | D5 / GPIO7 |
| GND | GND |
| VCC | 3V3（3.3V対応OLED） |

OLEDは暫定でSSD1306・128×64です。PlatformIO版は`include/config.h`、Arduino IDE版は`arduino/esp32c3_clock/esp32c3_clock.ino`先頭の`OLED_MODEL`を1にするとSH1106・128×64、2にするとSSD1306・128×32を使用します。型番はI2Cスキャンだけでは判別できません。アドレスは0x3C/0x3Dを自動検出します。

## Arduino IDEですぐ使う

Arduino IDEだけが入っているPCでは、`arduino/esp32c3_clock/esp32c3_clock.ino`を開いてそのままビルド・書き込みできます。

事前に必要なのは次の2点です。

- XIAO ESP32C3を使えるESP32ボードパッケージ
- U8g2ライブラリ

手順:

1. リポジトリをCloneまたはZIPで取得します。
2. `arduino/esp32c3_clock/esp32c3_clock.ino`をArduino IDEで開きます。
3. ボードに`XIAO_ESP32C3`を選択します。
4. XIAOをUSB接続して対象ポートを選択します。
5. 書き込みボタンを押します。
6. Serial Monitorを115200bpsで開きます。
7. 必要なら`set 2026-10-04 12:34:56`の形式で現在のJSTを設定します。

Arduino IDE版は1ファイル完結です。PlatformIOの`platformio.ini`、`scripts/build_time.py`、`include/config.h`は不要です。

起動直後に時刻が未設定の場合、Arduino IDE版は`__DATE__`と`__TIME__`から得たコンパイル時刻をJSTとして仮設定します。そのため、書き込み後はSerial Monitorから正しい時刻を設定してください。

## VS Code / PlatformIOで使用

1. このフォルダーをVS Codeで開き、推奨拡張PlatformIO IDEをインストールします。
2. PlatformIOのBuildでビルドします。
3. XIAOをUSB接続し、Uploadで書き込みます。
4. Serial Monitor（115200bps）でI2C検出結果と時計を確認します。
5. 入力欄で`set 2026-10-04 12:34:56`のように現在の日本時間を入力し、Enterを押します。入力例は現在時刻ではありません。

CLIでは`pio run`、`pio run -t upload`、`pio device monitor`が利用できます。複数のシリアル機器がある場合は対象ポートを明示してください。

## 時刻の仕様

- 日本時間（JST / UTC+9）の日付と時分秒を毎秒表示します。
- ESP32-C3のRTCタイマーを含むシステム時刻を`time()`で読みます。外付けRTCは不要です。
- 内蔵時計は電源断で失われます。起動時に時刻が未設定ならビルド/コンパイル時刻で初期化します。これは推定値であり、書き込みや電源断の経過時間は反映できません。
- 推定時刻ではシリアルに`build time estimate`、64px OLEDに`BUILD TIME / SET USB`が出ます。USBから時刻を設定すると消えます。
- シリアル設定は2024〜2037年を受け付け、不正な日付を拒否します。
- NTPなどの自動時刻合わせは未実装です。時計にはドリフトがあります。
- OLEDが見つからなくてもシリアル出力は継続します。配線や設定を修正したらリセットしてください。

## 参考

- [Seeed XIAO ESP32-C3](https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/)
- [PlatformIOのボード定義](https://docs.platformio.org/en/stable/boards/espressif32/seeed_xiao_esp32c3.html)
- [Espressifのシステム時刻仕様](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32c3/api-reference/system/system_time.html)

## 検証状況

2026-10-04: PlatformIOでSSD1306・128×64設定のビルド成功。RAM 15,344 bytes、Flash 299,424 bytes。

Arduino IDE版はPlatformIO依存を除去した単一スケッチとして追加。実機での書き込み・OLED表示の確認は未実施です。
