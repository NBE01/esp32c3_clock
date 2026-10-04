# XIAO ESP32-C3 OLED Clock

Seeed Studio XIAO ESP32-C3の内蔵システム時計をI2C OLEDに表示するPlatformIO / Arduinoプロジェクトです。PlatformIO版は起動時と以後24時間ごとにWi-Fiへ接続し、NTPで時刻を合わせます。Arduino IDE版はUSBからの手動時刻設定に対応します。

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

Arduino IDE版は1ファイル完結です。PlatformIOの`platformio.ini`、`scripts/build_time.py`、`include/config.h`は不要です。Arduino IDE版にはWi-Fi/NTP同期と`.env`の読み込みは含まれていません。

起動直後に時刻が未設定の場合、Arduino IDE版は`__DATE__`と`__TIME__`から得たコンパイル時刻をJSTとして仮設定します。そのため、書き込み後はSerial Monitorから正しい時刻を設定してください。

## VS Code / PlatformIOで使用

プロジェクト直下の`.env`に`WIFI_SSID`と`WIFI_PASSWORD`を設定してください。新しい環境では`.env.example`を`.env`へコピーします。NTPサーバーは`NTP_SERVER`で変更できます（初期値: `pool.ntp.org`）。

```dotenv
WIFI_SSID="your-ssid"
WIFI_PASSWORD="your-password"
NTP_SERVER="pool.ntp.org"
```

`.env`はビルド時に読み込み、`.pio/generated/config_env.h`へ変換します。値はそのまま扱い、変数展開やバックスラッシュの解釈は行いません。前後の空白を残す場合は引用符で囲んでください。コメントは独立した行に書きます（値の中の`#`はそのまま残ります）。`.env`がない場合やSSIDが空の場合はNTP同期を行いません。**設定変更後は再ビルド・書き込みが必要です。** `.env`と生成ヘッダーはGit管理の対象外です。同期間隔とタイムアウトは引き続き`include/wifi_config.h`で変更できます。

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
- 推定時刻ではシリアルに`build time estimate`、64px OLEDに`BUILD TIME / SET USB`が出ます。USBからの時刻設定、またはPlatformIO版のNTP同期で消えます。NTP同期後は64px OLEDに`NTP / INTERNAL CLOCK`を表示します。
- シリアル設定は2024〜2037年を受け付け、不正な日付を拒否します。
- PlatformIO版のNTP同期は起動時に開始し、以後は前回の試行開始から24時間ごとです。毎日固定の時刻に同期する方式ではありません。スケジュールは稼働時間で管理するため、NTPや手動で時刻を変更しても影響しません。
- Wi-Fi接続は最大30秒、NTP応答は最大15秒待ちます。その間もOLED表示とUSB時刻設定を続けます。同期成功はSNTPの完了状態で判定します。
- 同期成功・失敗のどちらでもSNTPを停止し、Wi-Fiをオフにします。失敗時は現在の内蔵時計で表示を続け、次の24時間周期で再試行します。1回の同期処理の中ではSNTPによる応答待ちの再送が発生する場合があります。
- 同期間は内蔵時計で進むためドリフトがあります。
- OLEDが見つからなくてもシリアル出力は継続します。配線や設定を修正したらリセットしてください。

## 参考

- [Seeed XIAO ESP32-C3](https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/)
- [PlatformIOのボード定義](https://docs.platformio.org/en/stable/boards/espressif32/seeed_xiao_esp32c3.html)
- [Espressifのシステム時刻仕様](https://docs.espressif.com/projects/esp-idf/en/v4.4.6/esp32c3/api-reference/system/system_time.html)

## 検証状況

2026-10-04: PlatformIOでSSD1306・128×64設定のビルド成功。RAM 15,344 bytes、Flash 299,424 bytes。

2026-10-04: NTP機能追加後のビルド成功。RAM 40,236 bytes、Flash 765,820 bytes。

PlatformIO版は実機でI2C検出・OLED表示・時計の毎秒更新を確認済みです。I2Cスキャン後に`Wire.end()`でバスを解放してからOLEDを初期化します。シリアルモニターはDTR/RTSを無効にしています。

NTP追加後の実機確認は未実施です。SSID・パスワード設定後、`NTP: synchronized (JST)`が出ることを確認してください。Wi-Fi接続失敗・NTP応答失敗ではタイムアウトログが出て時計表示が継続すること、翌周期で再接続することも確認対象です。

Arduino IDE版はPlatformIO依存を除去した単一スケッチとして追加。実機での書き込み・OLED表示の確認は未実施です。
