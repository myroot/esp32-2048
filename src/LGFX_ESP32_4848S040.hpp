#ifndef LGFX_ESP32_4848S040_HPP
#define LGFX_ESP32_4848S040_HPP

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>

class LGFX_ESP32_4848S040 : public lgfx::LGFX_Device {
  lgfx::Panel_ST7701_guition_esp32_4848S040 panel_instance;
  lgfx::Bus_RGB bus_instance;
  lgfx::Light_PWM light_instance;
  lgfx::Touch_GT911 touch_instance;

public:
  LGFX_ESP32_4848S040() {
    // ─── Backlight ─────────────────────────────────────────────
    {
      auto cfg = light_instance.config();
      cfg.pin_bl = 38;
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;
      light_instance.config(cfg);
      panel_instance.setLight(&light_instance);
    }

    // ─── RGB Bus ───────────────────────────────────────────────
    {
      auto cfg = bus_instance.config();
      cfg.panel = &panel_instance;

      // Pixel clock & sync
      cfg.pin_pclk = 21;    // PCLK
      cfg.pin_henable = 18; // DE
      cfg.pin_vsync = 17;   // VSYNC
      cfg.pin_hsync = 16;   // HSYNC

      // LovyanGFX Bus_RGB internal routing (rgb565sig_tbl =
      // {8,9,10,11,12,13,14,15, 0,1,2,3,4,5,6,7}) Exact hardware signal
      // alignment: d0..d2  = G3..G5 (Signals 8..10) d3..d7  = R0..R4
      // (Signals 11..15) d8..d12 = B0..B4 (Signals 0..4) d13..d15= G0..G2
      // (Signals 5..7)

      // Red 0-4 (d0~d4)
      cfg.pin_d0 = 4;  // R0
      cfg.pin_d1 = 5;  // R1
      cfg.pin_d2 = 6;  // R2
      cfg.pin_d3 = 7;  // R3
      cfg.pin_d4 = 15; // R4

      // Green 0-5 (d5~d10)
      cfg.pin_d5 = 8;   // G0
      cfg.pin_d6 = 20;  // G1
      cfg.pin_d7 = 3;   // G2
      cfg.pin_d8 = 46;  // G3
      cfg.pin_d9 = 9;   // G4
      cfg.pin_d10 = 10; // G5

      // Blue 0-4 (d11~d15) -> LovyanGFX Bus_RGB D11..D15 is Blue
      cfg.pin_d11 = 11; // B0
      cfg.pin_d12 = 12; // B1
      cfg.pin_d13 = 13; // B2
      cfg.pin_d14 = 14; // B3
      cfg.pin_d15 = 0;  // B4

      // Horizontal timing
      cfg.freq_write = 16000000;
      cfg.hsync_polarity = 0;
      cfg.hsync_front_porch = 10;
      cfg.hsync_pulse_width = 8;
      cfg.hsync_back_porch = 50;
      // Vertical timing
      cfg.vsync_polarity = 0;
      cfg.vsync_front_porch = 10;
      cfg.vsync_pulse_width = 8;
      cfg.vsync_back_porch = 20;

      bus_instance.config(cfg);
      panel_instance.setBus(&bus_instance);
    }

    // ─── Panel ─────────────────────────────────────────────────
    {
      auto cfg = panel_instance.config();
      cfg.memory_width = 480;
      cfg.memory_height = 480;
      cfg.panel_width = 480;
      cfg.panel_height = 480;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      panel_instance.config(cfg);
    }

    // ─── Panel Detail (ST7701 3-wire SPI init) ─────────────────
    {
      auto cfg = panel_instance.config_detail();
      cfg.pin_cs = 39;   // LCD CS
      cfg.pin_sclk = 48; // ST7701 SCK
      cfg.pin_mosi = 47; // ST7701 MOSI/SDA
      cfg.use_psram = 1; // Use PSRAM for framebuffer
      panel_instance.config_detail(cfg);
    }

    // ─── Touch (GT911, I2C) ────────────────────────────────────
    {
      auto cfg = touch_instance.config();
      cfg.x_min = 0;
      cfg.x_max = 479;
      cfg.y_min = 0;
      cfg.y_max = 479;
      cfg.pin_int = -1;
      cfg.bus_shared = false;
      cfg.offset_rotation = 0;
      cfg.i2c_port = 1;
      cfg.pin_sda = 19;
      cfg.pin_scl = 45; // 실제 보드: SCL=45
      cfg.freq = 400000;
      cfg.i2c_addr = 0x5D;
      touch_instance.config(cfg);
      panel_instance.setTouch(&touch_instance);
    }

    setPanel(&panel_instance);
  }
};

#endif
