// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 FerienBSD aka Marcel Sauder
// Display- und Touch-Konfiguration fuer ESP32-2432S028 (CYD), ILI9341, XPT2046
#pragma once
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// Einstellbar in secrets.h (vor diesem Include):
//   PANEL_ST7789 1    ST7789 statt ILI9341 (CYD mit 2 USB-Anschluessen, bei Farbstich ausprobieren)
//   PANEL_INVERT 0/1  Farben negativ? umschalten
//   PANEL_RGB_ORDER 0/1  Rot und Blau vertauscht? umschalten
#ifndef PANEL_ST7789
#define PANEL_ST7789 0
#endif
#ifndef PANEL_INVERT
#if PANEL_ST7789
#define PANEL_INVERT 1
#else
#define PANEL_INVERT 0
#endif
#endif
#ifndef PANEL_RGB_ORDER
#define PANEL_RGB_ORDER 0
#endif

class LGFX_CYD : public lgfx::LGFX_Device {
#if PANEL_ST7789
  lgfx::Panel_ST7789  _panel;
#else
  lgfx::Panel_ILI9341 _panel;
#endif
  lgfx::Bus_SPI       _bus;
  lgfx::Light_PWM     _light;
  lgfx::Touch_XPT2046 _touch;

public:
  LGFX_CYD() {
    {
      auto cfg = _bus.config();
      cfg.spi_host    = SPI2_HOST;
      cfg.spi_mode    = 0;
      cfg.freq_write  = 40000000;
      cfg.freq_read   = 16000000;
      cfg.spi_3wire   = false;
      cfg.use_lock    = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk    = 14;
      cfg.pin_mosi    = 13;
      cfg.pin_miso    = -1;   // wie im Werksprogramm
      cfg.pin_dc      = 2;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs           = 15;
      cfg.pin_rst          = -1;
      cfg.pin_busy         = -1;
      cfg.panel_width      = 240;
      cfg.panel_height     = 320;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      cfg.offset_rotation  = 0;
      cfg.readable         = false;
      cfg.invert           = PANEL_INVERT;
      cfg.rgb_order        = PANEL_RGB_ORDER;
      cfg.dlen_16bit       = false;
      cfg.bus_shared       = false;
      _panel.config(cfg);
    }
    {
      auto cfg = _light.config();
      cfg.pin_bl      = 21;
      cfg.invert      = false;
      cfg.freq        = 44100;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    {
      auto cfg = _touch.config();
      cfg.x_min           = 300;
      cfg.x_max           = 3900;
      cfg.y_min           = 3700;
      cfg.y_max           = 200;
      cfg.pin_int         = 36;
      cfg.bus_shared      = false;
      cfg.offset_rotation = 0;
      cfg.spi_host        = SPI3_HOST;
      cfg.freq            = 1000000;
      cfg.pin_sclk        = 25;
      cfg.pin_mosi        = 32;
      cfg.pin_miso        = 39;
      cfg.pin_cs          = 33;
      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }
    setPanel(&_panel);
  }
};
