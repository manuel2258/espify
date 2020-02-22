#include "graphics_manager.h"
#include "esp_log.h"

namespace graphics {

GraphicsManager::GraphicsManager() {
  tft_max_rdclock = 8000000;

  TFT_PinsInit();

  spi_lobo_device_handle_t spi;

  spi_lobo_bus_config_t bus_cfg = {};
  bus_cfg.miso_io_num = 0;
  bus_cfg.mosi_io_num = PIN_NUM_MOSI;
  bus_cfg.sclk_io_num = PIN_NUM_CLK;
  bus_cfg.quadwp_io_num = -1;
  bus_cfg.quadhd_io_num = -1;
  bus_cfg.max_transfer_sz = 6 * 1024;

  spi_lobo_device_interface_config_t dev_cfg = {};
  dev_cfg.clock_speed_hz = 8000000;
  dev_cfg.mode = 0;
  dev_cfg.spics_io_num = -1;
  dev_cfg.spics_ext_io_num = PIN_NUM_CS;
  dev_cfg.flags = LB_SPI_DEVICE_HALFDUPLEX;

  ESP_LOGI(LOG_TAG, "Initialized Display-Bus");
  ESP_ERROR_CHECK(
      spi_lobo_bus_add_device(TFT_HSPI_HOST, &bus_cfg, &dev_cfg, &spi));

  tft_disp_spi = spi;

  TFT_display_init();
  ESP_LOGI(LOG_TAG, "Initialized Display");

  tft_max_rdclock = find_rd_speed();

  spi_lobo_set_speed(spi, DEFAULT_SPI_CLOCK);
  ESP_LOGI(LOG_TAG, "Speed changed to: %u", spi_lobo_get_speed(spi));

  tft_font_rotate = 0;
  tft_text_wrap = 0;
  tft_font_transparent = 0;
  tft_font_forceFixed = 0;
  tft_gray_scale = 0;
  TFT_setGammaCurve(DEFAULT_GAMMA_CURVE);
  TFT_setRotation(LANDSCAPE);
  TFT_setFont(DEFAULT_FONT, NULL);
  TFT_resetclipwin();
}

void GraphicsManager::add_to_base(IDrawAble *draw_able) {
  base_group.add_child(draw_able);
}

void GraphicsManager::draw_all() { base_group.draw(); }

} // namespace graphics