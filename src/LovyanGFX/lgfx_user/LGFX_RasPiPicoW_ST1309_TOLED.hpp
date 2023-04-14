#pragma once

#define LGFX_USE_V1

#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device
{
lgfx::Panel_SSD1306     _panel_instance;

// パネルを接続するバスの種類にあったインスタンスを用意します。
lgfx::Bus_SPI       _bus_instance;   // SPIバスのインスタンス


public:
  LGFX(void)
  {
    { // バス制御の設定を行います。
      auto cfg = _bus_instance.config();    // バス設定用の構造体を取得します。

      // SPIバスの設定
      cfg.spi_host = 0;     
      cfg.spi_mode = 3;             // SPI通信モードを設定 (0 ~ 3)
      cfg.freq_write = 2000000;     // 送信時のSPIクロック (最大80MHz, 80MHzを整数で割った値に丸められます)
      cfg.pin_sclk = 18;            // SPIのSCLKピン番号を設定
      cfg.pin_mosi = 19;            // SPIのMOSIピン番号を設定
      cfg.pin_miso = -1;            // SPIのMISOピン番号を設定 (-1 = disable)
      cfg.pin_dc   = 13;            // SPIのD/Cピン番号を設定  (-1 = disable)
      _bus_instance.config(cfg);    // 設定値をバスに反映します。
      _panel_instance.setBus(&_bus_instance);      // バスをパネルにセットします。
    }

    { // 表示パネル制御の設定を行います。
      auto cfg = _panel_instance.config();    // 表示パネル設定用の構造体を取得します。
      cfg.pin_cs           =    17;  // CSが接続されているピン番号   (-1 = disable)
      cfg.pin_rst          =    12;  // RSTが接続されているピン番号  (-1 = disable)
      cfg.panel_width      =   128;  // 実際に表示可能な幅
      cfg.panel_height     =    64;  // 実際に表示可能な高さ
      cfg.offset_x         =     0;  // パネルのX方向オフセット量
      cfg.offset_y         =     0;  // パネルのY方向オフセット量
      cfg.invert           = false;  // パネルの明暗が反転してしまう場合 trueに設定
      cfg.rgb_order        = false;  // パネルの赤と青が入れ替わってしまう場合 trueに設定
      _panel_instance.config(cfg);
    }

    setPanel(&_panel_instance); // 使用するパネルをセットします。
  }
};

