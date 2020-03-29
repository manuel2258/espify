// . $HOME/esp-idf/export.sh

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "src/graphics/graphics_events.h"
#include "src/graphics/graphics_manager.h"
#include "src/graphics/graphics_types.h"

#include "src/io/io_manager.h"
#include "src/io/io_types.h"

#include "src/network/https_requester.h"

#include "src/spotify/spotify_manager.h"

#define ESPIFY_VERSION_TEXT "V1.0"

#include "config.h"

#define NEXT_TRACK_PIN GPIO_NUM_21
#define TOGGLE_PLAYBACK_PIN GPIO_NUM_19
#define PREV_TRACK_PIN GPIO_NUM_18
#define MINUS_VOL_PIN GPIO_NUM_23
#define SET_VOL_PIN GPIO_NUM_4
#define PLUS_VOL_PIN GPIO_NUM_15
#define VOL_CHANGE_AMOUNT 3

network::HttpsRequester *https_requester;
spotify::SpotifyManager *spotify_manager;
io::IOManager *io_manager;
graphics::GraphicsManager *graphics_manager;

QueueHandle_t buttons_queue;
extern EventGroupHandle_t graphics_events_handle;

void main_update(void *pv_pars) {
  for (;;) {
    https_requester->trigger_response_callbacks();
    io_manager->update();
    vTaskDelay(10);
  }
}

void https_requester_update(void *pv_pars) {
  https_requester->initialize_wifi();
  for (;;) {
    https_requester->update();
    vTaskDelay(10);
  }
}

#define SECONDS_PER_PROGRESS_UPDATE 2
#define SECONDS_PER_STATE_UPDATE 30
#define SECONDS_PER_REFRESH_TOKEN_UPDATE 3540

void spotify_update(void *pv_pars) {
  for (;;) {
    for (int i = 0;
         i < SECONDS_PER_REFRESH_TOKEN_UPDATE / SECONDS_PER_STATE_UPDATE; i++) {
      for (int j = 0;
           j < SECONDS_PER_STATE_UPDATE / SECONDS_PER_PROGRESS_UPDATE; j++) {
        vTaskDelay(configTICK_RATE_HZ * SECONDS_PER_PROGRESS_UPDATE);
        spotify_manager->update();
        xEventGroupSetBits(graphics_events_handle, SECOND_OVER);
      }
      spotify_manager->request_update_local_track();
      spotify_manager->request_update_local_volume();
    }
    spotify_manager->request_refresh_access_token();
  }
}

void graphics_update(void *pv_pars) {
  for (;;) {
    graphics_manager->update();
    vTaskDelay(10);
  }
}

#define SCREEN_HEIGHT 128
#define SCREEN_WIDTH 160
#define TRACK_HEIGHT 50
#define IMAGE_WIDTH 64

void initialize_graphics_objects() {
  // Initializes colors
  auto green_color = new color_t{0, 150, 0};
  auto red_color = new color_t{150, 0, 0};
  auto text_color = new color_t{150, 150, 150};
  auto bg_color = new color_t{5, 5, 5};

  // Lower Trackgroup
  auto track_group = new graphics::DrawGroup();

  auto track_text = new graphics::TextPtrDrawAble(
      7, SCREEN_HEIGHT - 40, text_color, bg_color, DEFAULT_FONT,
      SCREEN_WIDTH - 14, &spotify_manager->current_state.track.track_name);
  auto artist_text = new graphics::TextPtrDrawAble(
      7, SCREEN_HEIGHT - 20, green_color, bg_color, DEFAULT_FONT,
      SCREEN_WIDTH - 14, &spotify_manager->current_state.track.artist_name);

  track_group->add_child(track_text);
  track_group->add_child(artist_text);

  // Left Image
  auto image_jpeg = new graphics::JpegBufferDrawAble(
      5, 5, &spotify_manager->current_state.track.image_buf,
      &spotify_manager->current_state.track.buf_size, 0);

  // Right Progressgroup
  auto playback_group = new graphics::DrawGroup();
  uint8_t *const_val = new uint8_t(100);

  auto progress_line = new graphics::LineDrawAble(
      5, SCREEN_HEIGHT - TRACK_HEIGHT,
      &spotify_manager->current_state.track.progress, 1.47, green_color);

  auto progress_line_bg = new graphics::LineDrawAble(
      5, SCREEN_HEIGHT - TRACK_HEIGHT, const_val, 1.47, text_color);

  auto volume_arc_remote = new graphics::ArcDrawAble(
      IMAGE_WIDTH + 47, 35, 30, 3, &spotify_manager->current_state.volume, 3,
      30, green_color);
  auto volume_arc_local = new graphics::ArcDrawAble(
      IMAGE_WIDTH + 47, 35, 30, 3, &spotify_manager->current_state.local_volume,
      3, 30, red_color);
  auto volume_local_cond = new graphics::ConditionalDrawAble(
      []() { return spotify_manager->current_state.volume_change_counter > 0; },
      volume_arc_local, volume_arc_remote);
  auto volume_arc_bg = new graphics::ArcDrawAble(IMAGE_WIDTH + 47, 35, 30, 3,
                                                 const_val, 3, 30, text_color);

  auto isplaying_cond = new graphics::ConditionalDrawAble(
      []() { return spotify_manager->current_state.is_playing; },
      new graphics::BmpPathDrawAble(IMAGE_WIDTH + 32, 20, "/spiffs/play.bmp"),
      new graphics::BmpPathDrawAble(IMAGE_WIDTH + 32, 20, "/spiffs/pause.bmp"));

  playback_group->add_child(progress_line);
  playback_group->add_child(progress_line_bg);
  playback_group->add_child(volume_arc_bg);
  playback_group->add_child(volume_local_cond);
  playback_group->add_child(isplaying_cond);

  // Init Screengroup
  auto init_group = new graphics::DrawGroup();
  auto init_background = new graphics::RectangleDrawAble(
      0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, bg_color, true);

  char title_text[] = "ESPIFY";
  TFT_setFont(DEJAVU24_FONT, NULL);
  auto init_title = new graphics::TextDrawAble(
      SCREEN_WIDTH / 2 - TFT_getStringWidth(title_text) / 2, 10, green_color,
      DEJAVU24_FONT, title_text);

  char version_text[] = ESPIFY_VERSION_TEXT;
  TFT_setFont(DEFAULT_FONT, NULL);
  auto init_version = new graphics::TextDrawAble(
      SCREEN_WIDTH / 2 - TFT_getStringWidth(version_text) / 2, 42, text_color,
      DEFAULT_FONT, version_text);

  TFT_setFont(DEJAVU18_FONT, NULL);

  char wifi_text[] = "WiFi";

  auto init_wifi = new graphics::TextDrawAble(
      SCREEN_WIDTH / 2 - TFT_getStringWidth(wifi_text) / 2 - 15, 63, text_color,
      DEJAVU18_FONT, wifi_text);

  auto init_wifi_img_err =
      new graphics::BmpPathDrawAble(110, 54, "/spiffs/err.bmp");
  auto init_wifi_img_ok =
      new graphics::BmpPathDrawAble(110, 54, "/spiffs/ok.bmp");

  auto init_wifi_cond = new graphics::ConditionalDrawAble(
      []() { return https_requester->connected; }, init_wifi_img_ok,
      init_wifi_img_err);

  char spotify_text[] = "Spotify";

  auto init_spotify = new graphics::TextDrawAble(
      SCREEN_WIDTH / 2 - TFT_getStringWidth(spotify_text) / 2 - 15, 95,
      text_color, DEJAVU18_FONT, spotify_text);

  auto init_spotify_img_err =
      new graphics::BmpPathDrawAble(110, 87, "/spiffs/err.bmp");
  auto init_spotify_img_ok =
      new graphics::BmpPathDrawAble(110, 87, "/spiffs/ok.bmp");

  auto init_spotify_cond = new graphics::ConditionalDrawAble(
      []() { return spotify_manager->current_state.initialized; },
      init_spotify_img_ok, init_spotify_img_err);

  init_group->add_child(init_background);
  init_group->add_child(init_title);
  init_group->add_child(init_version);
  init_group->add_child(init_wifi);
  init_group->add_child(init_spotify);
  init_group->add_child(init_wifi_cond);
  init_group->add_child(init_spotify_cond);

  auto active_group = new graphics::DrawGroup();
  auto active_background = new graphics::RectangleDrawAble(
      0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, bg_color, true);

  auto main_cond = new graphics::ConditionalDrawAble(
      []() {
        return spotify_manager->current_state.initialized &
               https_requester->connected;
      },
      active_group, init_group);

  active_group->add_child(active_background);
  active_group->add_child(track_group);
  active_group->add_child(image_jpeg);
  active_group->add_child(playback_group);

  graphics_manager->add_to_base(init_group);

  // Register Events
  graphics_manager->register_event(NEW_SONG, track_text);
  graphics_manager->register_event(NEW_SONG, artist_text);
  graphics_manager->register_event(NEW_IMAGE, image_jpeg);
  graphics_manager->register_event(STATUS_CHANGED, main_cond);
  graphics_manager->register_event(PROGRESS_ADVANCE, progress_line);
  graphics_manager->register_event(NEW_SONG, progress_line_bg);
  graphics_manager->register_event(VOLUME_CHANGED, volume_arc_bg);
  graphics_manager->register_event(VOLUME_CHANGED, volume_local_cond);
  graphics_manager->register_event(PLAYBACK_CHANGED, isplaying_cond);

  graphics_manager->draw_all();
}

extern "C" void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  https_requester = new network::HttpsRequester();
  spotify_manager = new spotify::SpotifyManager();
  io_manager = new io::IOManager();
  graphics_manager = new graphics::GraphicsManager();

  initialize_graphics_objects();

  // --- Init Inputs ---
  io_manager->add_button(new io::ButtonInput(
      NEXT_TRACK_PIN, []() { spotify_manager->request_next_track(); }));

  io_manager->add_button(new io::ButtonInput(TOGGLE_PLAYBACK_PIN, []() {
    spotify_manager->request_toggle_playback();
  }));

  io_manager->add_button(new io::ButtonInput(
      PREV_TRACK_PIN, []() { spotify_manager->request_previous_track(); }));

  io_manager->add_button(new io::ButtonInput(MINUS_VOL_PIN, []() {
    spotify_manager->change_local_volume(-VOL_CHANGE_AMOUNT);
  }));

  io_manager->add_button(new io::ButtonInput(
      SET_VOL_PIN, []() { spotify_manager->request_set_volume(); }));

  io_manager->add_button(new io::ButtonInput(PLUS_VOL_PIN, []() {
    spotify_manager->change_local_volume(+VOL_CHANGE_AMOUNT);
  }));

  // --- Create Tasks ---

  xTaskCreate(&main_update, "main_update", 8192, NULL, 5, NULL);
  xTaskCreate(&https_requester_update, "https_update", 8192, NULL, 5, NULL);
  xTaskCreate(&spotify_update, "spotify_update", 3072, NULL, 5, NULL);
  xTaskCreate(&graphics_update, "graphics_update", 2048, NULL, 5, NULL);
}
