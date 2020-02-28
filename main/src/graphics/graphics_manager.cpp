#include "graphics_manager.h"

#include "esp_log.h"
#include "esp_spiffs.h"

#include "graphics_events.h"

EventGroupHandle_t graphics_events_handle;

namespace graphics {

GraphicsManager::GraphicsManager() {

  esp_vfs_spiffs_conf_t spiffs_cfg = {};
  spiffs_cfg.base_path = "/spiffs";
  spiffs_cfg.partition_label = "storage";
  spiffs_cfg.max_files = 5;
  spiffs_cfg.format_if_mount_failed = true;

  ESP_ERROR_CHECK(esp_vfs_spiffs_register(&spiffs_cfg));

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
  tft_font_transparent = 1;
  tft_font_forceFixed = 0;
  tft_gray_scale = 0;
  TFT_setGammaCurve(DEFAULT_GAMMA_CURVE);
  TFT_setRotation(LANDSCAPE);
  TFT_resetclipwin();

  graphics_events_handle = xEventGroupCreate();

  register_event(FULL_REDRAW, &base_group);
}

void GraphicsManager::add_to_base(IDrawAble *draw_able) {
  base_group.add_child(draw_able);
}

void GraphicsManager::draw_all() { base_group.draw(); }

void GraphicsManager::update() {
  EventBits_t triggered_bits = xEventGroupWaitBits(
      graphics_events_handle, full_mask, pdTRUE, pdFALSE, portMAX_DELAY);
  for (auto bit_drawable : event_map) {
    if ((bit_drawable.first & triggered_bits) != 0) {
      for (auto draw_able : bit_drawable.second) {
        draw_able->draw();
      }
    }
  }
}

void GraphicsManager::register_event(EventBits_t mask, IDrawAble *draw_able) {
  full_mask = full_mask | mask;

  auto element = event_map.find(mask);
  if (element != event_map.end()) {
    element->second.push_back(draw_able);
  } else {
    std::vector<IDrawAble *> new_map;
    new_map.push_back(draw_able);
    event_map.insert({mask, std::move(new_map)});
  }
}

} // namespace graphics