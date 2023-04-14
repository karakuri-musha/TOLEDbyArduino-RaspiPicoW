# TOLEDbyArduino-RaspiPicoW
# Transparent OLED by Arduino for Raspberry Pi Pico W

## OverView
Arduino を使って Raspberry Pi Pico W からWaveShareの透過型ディスプレイを制御するプログラムを作りました。
画面描画はLovyanGFXライブラリを使っています。
プログラムを実行するとWi-Fi接続し、OpenWeatherサービスから天気情報を取得後、透過型ディスプレイに結果を描画します。

詳しくは、次のブログで紹介していますので確認してください。

I made a program to control WaveShare's transmissive display from Raspberry Pi Pico W using Arduino.
Screen drawing uses the LovyanGFX library.
When you run the program, it connects to Wi-Fi, gets weather information from the OpenWeather service, and draws the results on a transparent display.

For details, please check the following blog.

### Blog URL
https://karakuri-musha.com/inside-technology/arduino-raspberrypi-picow-toled01/

## Src File
"src"フォルダに以下のソース一式が入っています。
- LovyanGFX （LovyanGFXをRaspberry Pi Pico Wと透過型ディスプレイで使用する場合のユーザ定義ファイル）
- RaspberryPi_Pico_WTOLED（今回作成したプログラム本体）

The "src" folder contains the following set of sources.
- LovyanGFX (user-defined file for using LovyanGFX with Raspberry Pi Pico W and transmissive display)
- RaspberryPi_Pico_WTOLED (program body created this time)

## How to use
1. GitHubの右上にある「Code」から「Download Zip」を選択します。
2. ダウンロードしたファイルを解凍します。
3. 「LovyanGFX」フォルダ内にある「lgfx_user」-「LGFX_RasPiPicoW_ST1309_TOLED.hpp」をLovyanGFXライブラリ内にある「lgfx_user」にコピーします。
　　※コピー先は、標準的な環境では、「Users\”ユーザ名”\Documents\Arduino\libraries\LovyanGFX\src\lgfx-user」にあります。
  　　ライブラリの場所を変更した場合は読み替えてください。
4. 「RaspberryPi_Pico_WTOLED」フォルダにある「RaspberryPi_Pico_WTOLED.ino」をArduino IDEで開いてください。

1. Select "Download Zip" from "Code" at the top right of GitHub.
2. Unzip the downloaded file.
3. Copy "lgfx_user"-"LGFX_RasPiPicoW_ST1309_TOLED.hpp" in the "LovyanGFX" folder to "lgfx_user" in the LovyanGFX library.
*The copy destination is in "Users\"user name"\Documents\Arduino\libraries\LovyanGFX\src\lgfx-user" in the standard environment.
   　　If you change the location of the library, please replace it.
4. Open "RaspberryPi_Pico_WTOLED.ino" in "RaspberryPi_Pico_WTOLED.ino" folder with Arduino IDE.

## Operating environment
動作確認環境は以下の通りです。

### 1.HARDWARE
- Raspberry Pi Pico W
- WaveShare Raspberry Pi Pico用 1.51 Transparent OLED　128×64

### 2.IDE
- Arduino IDE (ver:2.0.4）

### 3.BoardManager
- [Name] Raspberry Pi Pico/RP2040
- [BoardManager URL] https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json

### 4.Additional library
- ArduinoJson
- Time By Michael Margolis　（1.6.1）
- LovyanGFX


### 3.Other
 ライブラリやボードマネージャなどの構成は、上記のBlog URLを参照してください。
 
 Please refer to the blog URL above for configuration of libraries, boards manager, etc.
