// -----------------------------------------------------------------------
// Raspberry Pi Pico W e-Paper Test Program
//
//　透過型OLEDモジュールへのお天気画像表示プログラム
//
// 2023.04.10
// Author   : げんろく@karakuri-musha
// Device   : Raspberry Pi Pico W
//            1.51インチ透明OLEDスクリーン　128×64
//            
// License:
//    See the license file for the license.
// -----------------------------------------------------------------------
// ------------------------------------------------------------
// ライブラリインクルード部 Library include section.
// ------------------------------------------------------------
#include <WiFi.h>                                             // Wifi制御用ライブラリ
#include <time.h>                                             // 時刻制御用ライブラリ
#include <TimeLib.h>                                          // 時刻変換用ライブラリ
#include <stdio.h>                                            // 標準入出力処理ライブラリ

//#define LGFX_AUTODETECT                                     // 画面表示用ライブラリ
#include <LovyanGFX.hpp>                                      // 画面表示用ライブラリ
#include <lgfx_user/LGFX_RasPiPicoW_ST1309_TOLED.hpp>         // 画面表示用ライブラリ
static LGFX lcd;                                              // LGFXのインスタンス作成
static LGFX_Sprite sprite_wico(&lcd);                         // 天気アイコン用スプライト作成
static LGFX_Sprite sprite_time(&lcd);                         // 時間表示用スプライト作成

#include <HTTPClient.h>                                       // Http制御クライアントライブラリ
#include <WiFiClientSecure.h>                                 // Https制御クライアントライブラリ

#include <ArduinoJson.h>                                      // JSONデータ制御用ライブラリ

#include <SPI.h>                                              // SPI通信用ライブラリ

#include "Original_Image_Data.h"                              // 画面表示用の画像データ

// ------------------------------------------------------------
// 定数/変数　定義部　Constant / variable definition section.
// ------------------------------------------------------------ 
const char* ssid = "your-ssid";                               // ご自分のWi-FiルータのSSIDを記述します。 "your-ssid"
const char* password = "your-password";                       // ご自分のWi-Fiルータのパスワードを記述します。"your-password"

// NTP接続情報　NTP connection information.
const char* NTPSRV      = "ntp.jst.mfeed.ad.jp";              // NTPサーバーアドレス NTP server address.
const long  GMT_OFFSET  = 9;                                  // GMT-TOKYO(時差９時間）9 hours time difference.
int current_date;                                             // 日付表示判定用
int current_hour;                                             // 時計表示判定用
int current_min;                                              // 時計表示判定用
int current_sec;                                              // 時計表示判定用
int hour_v;                                                   // 時計表示判定用
int delay_time = 0;                                           // 時計表示用の待機カウンタ

// OpenWeatherへのhttpsクエリ生成用
const String sitehost = "api.openweathermap.org";             // OpenWeatherのサイトホスト名
const String Api_KEY  = "Please correct it to the value that suits your environment.";// API-KEY（ユーザにより変わります。）
const String lang = "ja";                                     // 言語設定
char geo_lat[20] = "35.6828";                                 // 緯度（東京）
char geo_lon[20] = "139.759";                                 // 経度（東京）
int delay_icon = 0;                                           // 天気アイコンアニメーション用の待機カウンタ
int icon_stats = 0;                                           // 天気アイコンのステータス格納用

// JSON用
const size_t mem_cap = 2048;                                  // JSON形式データ格納用メモリサイズ
StaticJsonDocument<mem_cap> n_jsondata;                       // JSON形式データ格納用

// 天気ステータス格納用
uint8_t Weather_Stats = 0;                                    // 0:初期化 1:雨 2:雪 3:注意 4:晴れ 5:曇り

// OpenWeatherのルート証明書を定義
const char* ow_rootca = \
"-----BEGIN CERTIFICATE-----\n" \
"Please correct it to the value that suits your environment." 
"-----END CERTIFICATE-----\n";

// 色指定
static uint16_t  c_BLACK           = 0x0000;                  //  0,   0,   0 
static uint16_t  c_WHITE           = 0xFFFF;                  //255, 255, 255

// ------------------------------------------------------------
// 時刻同期 関数　Time synchronization function.
// ------------------------------------------------------------ 
void setClock(const char* ntpsrv, const long gmt_offset) {
  char buf[64];                                                 // 日時出力用

  NTP.begin(ntpsrv);                                            // NTPサーバとの同期
  NTP.waitSet();                                                // 同期待ち

  time_t now = time(nullptr);                                   // 時刻の取得
  struct tm timeinfo;                                           // tm（時刻）構造体の生成
  gmtime_r(&now, &timeinfo);                                    // time_tからtmへ変換

  setTime(timeinfo.tm_hour,                                     // 時刻表示用の領域に時間をセット
          timeinfo.tm_min, 
          timeinfo.tm_sec, 
          timeinfo.tm_mday, 
          timeinfo.tm_mon+1, 
          timeinfo.tm_year+1900); 
  adjustTime(gmt_offset * SECS_PER_HOUR);                       // 時刻を日本時間へ変更
  Serial.print("Current time: ");                               // シリアル出力
  sprintf(buf,                                                  // シリアル出力用に日時データの編集
          " %04d-%02d-%02d %02d:%02d:%02d\n", 
          year(), 
          month(), 
          day(), 
          hour(), 
          minute(), 
          second());
  Serial.print(buf);                                             // 日時データのシリアル出力
}

// ------------------------------------------------------------
// https要求実行 関数　
// ------------------------------------------------------------
bool Https_GetRes(String url_str, 
                  String *payload) {
  
  if (WiFi.status() != WL_CONNECTED)                              // Wi-Fi接続の確認
    return false;                                                 // 未接続の場合Falseを戻す

  // OpenWeather に対して天気データを要求
  // https通信を行い、JSON形式データを取得する
  WiFiClientSecure *client = new WiFiClientSecure;                // インスタンス生成
  if(client) {
    client -> setCACert(ow_rootca);                               // ルート証明書のセット
    {
      HTTPClient https;                                           // HTTPClientのhttpsスコープブロックを追加

      Serial.print("[HTTPS] begin...\n");                         // シリアルポート出力
      if (https.begin(*client, url_str)) {                        // HTTPS要求セット
        Serial.print("[HTTPS] GET...\n");                         // シリアルポート出力
        int httpCode = https.GET();
  
        if (httpCode > 0) {
          Serial.printf("[HTTPS] GET... code: %d\n", httpCode);   // シリアルポート出力
  
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            *payload = https.getString();                         // OpenWeatherから取得したデータの格納
            Serial.println(*payload);                             // シリアルポート出力
            return true;                                          // 取得成功
          } else {
            return false;                                         // 取得失敗
          }
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
          return false;                                           // 取得失敗
        }
        https.end();                                              // Https通信の終了
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");             // シリアルポート出力
        return false;                                             // 取得失敗
      }
    }
    delete client;                                                // Clientの削除
  } else {
    Serial.println("Unable to create client");                    // シリアルポート出力
    return false;                                                 // 取得失敗
  }  
}

// ============================================================
// OpenWeatherデータ取得：描画 関数　
// ============================================================
void OpenWeather_get_print() {

  // 天気情報取得用URLの生成
  String url = "https://" + String(sitehost);                     // 要求先ホストの指定
  url += "/data/2.5/weather?";                                    // APIの指定
  url += "lat=" + String(geo_lat) + "&lon=" + String(geo_lon);    // 経度、緯度の指定
  url += "&units=metric&lang=" + lang;                            // 言語設定の指定
  url += "&appid=" + Api_KEY;                                     // APIキーの指定
  Serial.println(url);                                            // シリアルポート出力

  // 天気情報の取得とJSONデータからの値の取得
  String payload;                                                 // 取得データ格納用

  // OpenWeatherへのデータを要求が正常に行えた場合
  if (Https_GetRes(url, &payload)){                               
    DeserializationError error = deserializeJson(n_jsondata, payload); // JSONデータの格納
                                                                  
    if (error) {                                                  // JSONデータ格納に失敗した場合
      Serial.print(F("deserializeJson() failed: "));              // シリアルモニタに出力
      Serial.println(error.f_str());                              // シリアルモニタにエラーを出力
    } else {                                                      // 正常な場合はJSON形式データから値を取得して画面表示
                                                                  // JSON形式データから値の取得
      const int weather_id = n_jsondata["weather"][0]["id"];      // 天気IDの取得
      Serial.println("WeatherID : "+ String(weather_id));                                     
      // 画面表示内容の生成
      if (weather_id >= 200 && weather_id < 600) {                // 雨と判定
        Weather_Stats = 1;                                        // 天気ステータス設定
      }
      else if (weather_id >= 600 && weather_id < 700) {           // 雪と判定
        Weather_Stats = 2;                                        // 天気ステータス設定
      }
      else if (weather_id >= 700 && weather_id < 800) {           // その他の天気と判定
        Weather_Stats = 3;                                        // 天気ステータス設定
      }
      else if (weather_id == 800) {                               // 晴と判定
        Weather_Stats = 4;                                        // 天気ステータス設定
      }
      else {                                                      // 曇りと判定
        Weather_Stats = 5;                                        // 天気ステータス設定
      }
      Serial.println("WeatherStats : " + String(Weather_Stats));
    }
  }
  // OpenWeatherへのデータを要求が失敗した場合
  else {
    Serial.println("Connection failed (OpenWeather)");            // シリアルモニタに出力
  }
}

// ============================================================
// 透過型OLED表示 関数　
// ============================================================
// 天気アイコンの描画関数
void sprite_wico_print(int weather_stats, int icon_stats) {
  lcd.fillRect(0, 0, 64, 64, c_BLACK);
  sprite_wico.fillScreen(c_BLACK);
  sprite_wico.pushRotateZoom(0, 0, 0.0, 1.0, 1.0, c_BLACK);       // 天気アイコンの描画（80×80px→64×64Px） 

  switch (weather_stats) {                                        // Weather＿Statsの値によって表示するアイコンを変更
    case 1:
      if (icon_stats == 0) {
        sprite_wico.pushImage(0, 0, 64, 64, o_image_Rain01);      // 雨アイコン01のロード
      } else {
        sprite_wico.pushImage(0, 0, 64, 64, o_image_Rain02);      // 雨アイコン02のロード
      }
      break;
    case 2:
      if (icon_stats == 0) {
        sprite_wico.pushImage(0, 0, 64, 64, o_image_Snow01);      // 雪アイコン01のロード
      } else {
        sprite_wico.pushImage(0, 0, 64, 64, o_image_Snow02);      // 雪アイコン02のロード
      }
      break;
    case 3:
      if (icon_stats == 0) {
        sprite_wico.pushImage(0, 0, 64, 64, o_image_Caut01);      // 注意アイコン01のロード
      } else {
        sprite_wico.pushImage(0, 0, 64, 64, o_image_Caut02);      // 注意アイコン02のロード
      }
      break;
     case 4:
      if (icon_stats == 0) {
        sprite_wico.pushImage(0, 0, 64, 64, o_image_Sun01);       // 晴れアイコン01のロード
      } else {
        sprite_wico.pushImage(0, 0, 64, 64, o_image_Sun02);       // 晴れアイコン02のロード
      }
      break;
    case 5:
      if (icon_stats == 0) {
        sprite_wico.pushImage(0, 0, 64, 64, o_image_Cloud01);     // くもりアイコン01のロード
      } else {
        sprite_wico.pushImage(0, 0, 64, 64, o_image_Cloud02);     // くもりアイコン02のロード
      }
      break;
  }
  sprite_wico.pushRotateZoom(5, 5, 0.0, 0.8, 0.8, c_BLACK);       // 天気アイコンの描画（64×64Pxの80%） 
}

// ============================================================
// 日付の描画関数
// ============================================================
void toled_date_print() {
  lcd.fillRect(66, 0, 32, 64, c_BLACK);                           // 日付表示部分の塗りつぶし
  lcd.setFont(&fonts::lgfxJapanMincho_24);                        // フォントの設定
  
  char buf[64];                                                   // 日付表示内容の格納用
  sprintf(buf, "%02d/%02d", month(), day());                      // 日付表示内容の生成  
  lcd.drawString(buf, 68, 0);                                     // 日付表示
}

// ============================================================
// 時間の描画関数
// ============================================================
void toled_time_print() {
  if (hour_v != hour()) {                                         // 時（hour）が変わった際の処理
    lcd.fillRect(68, 37, 64, 32, c_BLACK);                        // 時計表示部分の全領域塗りつぶし
    hour_v = hour();                                              // 現在時刻（時）の更新
  } else if (current_min != minute()) {                           // 分（minute）が変わった際の処理
    lcd.fillRect(85, 37, 64, 32, c_BLACK);                        // 分秒の表示部分の塗りつぶし
    current_min = minute();                                       // 現在時刻（分）の更新
  } else {                                                        // その他の場合の処理
    lcd.fillRect(105, 37, 64, 32, c_BLACK);                       // 秒の表示部分の塗りつぶし
    current_sec = second();                                       // 現在時刻（秒）の更新
  }
  sprite_time.fillRect(68, 37, 32, 64, c_BLACK);                  // スプライトの塗りつぶし
  sprite_time.setFont(&fonts::Font2);                             // スプライトのフォントの設定
  
  char buf[64];                                                   // 時刻表示内容の格納用
  sprintf(buf, "%02d:%02d:%02d", hour(), minute(), second());     // 時刻表示内容の生成
  sprite_time.drawString(buf, 0, 0);                              // スプライトへの時刻描画
  sprite_time.pushRotateZoom(68, 37, 0.0, 1.0, 1.0, c_BLACK);     // スプライトをLCDへ描画
}

// ============================================================
// 初期起動時のSetup関数
// ============================================================
void setup() {
  // シリアルコンソールの開始　Start serial console.
  Serial.begin(9600); 
  delay(3000); 

  // 画面描画設定
  lcd.init();                                                     // 画面描画インスタンス初期化
  lcd.setRotation(2);                                             // 画面回転設定
  lcd.setBrightness(60);                                          // 画面輝度を設定（0～255）

  // 起動時のロゴ画像の表示
  lcd.setSwapBytes(false);                                        
  lcd.pushImage(0, 0, 128, 64, o_image_logo);                     // 直接ディスプレイに表示

  // Wi-Fi接続に向けたシリアルモニタ出力
  Serial.println();                                               // シリアルポート出力 
  Serial.println();                                               // シリアルポート出力 
  Serial.print("Connecting to ");                                 // シリアルポート出力 
  Serial.println(ssid);                                           // シリアルポート出力 

  WiFi.begin(ssid, password);                                     // Wi-Fi接続開始
  
  while (WiFi.status() != WL_CONNECTED) {                         // Wi-Fi接続の状況を監視（WiFi.statusがWL_CONNECTEDになるまで繰り返し 
    delay(500); 
    Serial.print("."); } 
  
  // Wi-Fi接続結果をシリアルモニタへ出力 
  Serial.println("");                                             // シリアルモニタに出力
  Serial.println("WiFi connected");                               // シリアルモニタに出力
  Serial.println("IP address: ");                                 // シリアルモニタに出力
  Serial.println(WiFi.localIP());                                 // シリアルモニタに出力

  // 時刻同期関数
  setClock(NTPSRV, GMT_OFFSET);

  // 天気表示の準備
  // 1.Spriteのサイズ設定
  sprite_wico.createSprite(64, 64);                               // 天気アイコン表示用Sprite
  sprite_time.createSprite(64, 32);                               // 時刻表示用Sprite

  // 2.Sprite表示位置の設定
  sprite_wico.setPivot(0, 0);                                     // 天気アイコン用スプライトの中心設定
  sprite_time.setPivot(0, 0);                                     // 時刻用スプライトの中心設定

  // 画面クリア
  lcd.fillScreen(c_BLACK);                                        // lcd画面全域を塗りつぶし

  // 初期表示
  lcd.drawLine(64, 32, 128, 32, c_WHITE);                         // 日付と時間の間の直線描画
  OpenWeather_get_print();                                        // 天気情報の取得とアイコン表示 
  toled_date_print();                                             // 日付の表示
  toled_time_print();                                            // 時間の表示
}

// ============================================================
// 繰り返し（Loop）関数
// ============================================================
void loop() {
  if (current_date != day()) {                                    // 日付が変わった場合
    toled_date_print();                                           // 日付の表示
    current_date = day();                                         // 現在日付の更新
  }

  if (delay_time == 80000) {                                      // 時間表示タイミングカウンタが指定の値になった場合
    toled_time_print();                                           // 時間表示を更新
    delay_time = 0;                                               // 時間表示タイミングカウンタの初期化
  }

  // 一時間おきにOpenWeatherからのデータ取得と描画
  if (current_hour != hour()) {                                   // 時（hour)が変更になった場合
    OpenWeather_get_print();                                      // 天気情報の取得とアイコン表示
    current_hour = hour();                                        // 現在時刻の更新
  }

  if (delay_icon == 200000) {                                     // 天気アイコン表示タイミングカウンタが指定の値になった場合
    delay_icon = 0;                                               // 天気アイコン表示タイミングカウンタの初期化
    if (icon_stats == 0) {                                        // 天気アイコン１が表示済みの場合
      icon_stats = 1;                                             // 天気アイコン２の表示に変更
    } else {                                                      // 天気アイコン２が表示済みの場合
      icon_stats = 0;                                             // 天気アイコン１の表示に変更
    }
    sprite_wico_print(Weather_Stats, icon_stats);                 // 天気アイコンを表示
  }

  delay_icon = delay_icon + 1;                                    // 天気アイコン表示カウンタのカウントアップ
  delay_time = delay_time + 1;                                    // 時刻表示タイミングカウンタのカウントアップ
}
