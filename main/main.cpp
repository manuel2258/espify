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

#define ESPIFY_VERSION_TEXT "V0.1"

#define SCREEN_WIDTH 160
#define SCREEN_HEIGHT 128
#define TRACK_HEIGHT 50
#define IMAGE_WIDTH 75

std::string CLIENT_ID = "e0fabf234d944aeba88bee39c39abc9f";
std::string CLIENT_SECRET = "fbe969d7f3c544dda1f607d129577017";

std::string REFRESH_TOKEN =
    "AQDzN3UESuB3G7RnIzM1-BrBpVd9Zc_"
    "XSlCo6Romsf7qZd58aDngQs3sUC5kbOgMNMVblNMQGzjiGEfHEQramwJiEMgN0SUMFmDIsY-"
    "i1KCxX9VJ-48aM3FTKknYYQSzegLm3A";

QueueHandle_t buttons_queue;

network::HttpsRequester *https_requester;
spotify::SpotifyManager *spotify_manager;
io::IOManager *io_manager;
graphics::GraphicsManager *graphics_manager;

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

void initialize_graphics_objects() {
  // Initializes colods
  auto green_color = new color_t{0, 150, 0};
  auto red_color = new color_t{150, 0, 0};
  auto text_color = new color_t{150, 150, 150};
  auto bg_color = new color_t{5, 5, 5};

  // Lower Trackgroup
  auto track_group = new graphics::DrawGroup();
  auto track_background = new graphics::RectangleDrawAble(
      0, SCREEN_HEIGHT - TRACK_HEIGHT, SCREEN_WIDTH, TRACK_HEIGHT, bg_color,
      true);

  auto track_text = new graphics::TextPtrDrawAble(
      7, SCREEN_HEIGHT - 40, text_color, DEFAULT_FONT,
      &spotify_manager->current_state.track.track_name);
  auto artist_text = new graphics::TextPtrDrawAble(
      7, SCREEN_HEIGHT - 20, green_color, DEFAULT_FONT,
      &spotify_manager->current_state.track.artist_name);

  auto line = new graphics::LineDrawAble(5, SCREEN_HEIGHT - TRACK_HEIGHT,
                                         SCREEN_WIDTH - 10, green_color);

  track_group->add_child(track_background);
  track_group->add_child(line);
  track_group->add_child(track_text);
  track_group->add_child(artist_text);

  // Left Imagegroup
  auto image_group = new graphics::DrawGroup();
  auto image_background = new graphics::RectangleDrawAble(
      0, 0, IMAGE_WIDTH, SCREEN_HEIGHT - TRACK_HEIGHT, bg_color, true);

  auto image_jpeg = new graphics::JpegBufferDrawAble(
      5, 5, &spotify_manager->current_state.track.image_buf,
      &spotify_manager->current_state.track.buf_size);

  image_group->add_child(image_background);
  image_group->add_child(image_jpeg);

  // Right Progressgroup
  auto progress_group = new graphics::DrawGroup();
  auto progress_background = new graphics::RectangleDrawAble(
      IMAGE_WIDTH, 0, SCREEN_WIDTH - IMAGE_WIDTH, SCREEN_HEIGHT - TRACK_HEIGHT,
      bg_color, true);
  uint8_t *const_val = new uint8_t(100);

  auto progress_arc = new graphics::ArcDrawAble(
      IMAGE_WIDTH + 40, 35, 32, 5,
      &spotify_manager->current_state.track.progress, 3, 30, green_color);
  auto progress_arc_bg = new graphics::ArcDrawAble(
      IMAGE_WIDTH + 40, 35, 32, 5, const_val, 3, 30, text_color);

  auto volume_arc_remote = new graphics::ArcDrawAble(
      IMAGE_WIDTH + 40, 35, 20, 4, &spotify_manager->current_state.volume, 3,
      30, green_color);
  auto volume_arc_local = new graphics::ArcDrawAble(
      IMAGE_WIDTH + 40, 35, 20, 4, &spotify_manager->current_state.local_volume,
      3, 30, red_color);
  auto volume_local_cond = new graphics::ConditionalDrawAble(
      []() { return spotify_manager->current_state.volume_change_counter > 0; },
      volume_arc_local, volume_arc_remote);
  auto volume_arc_bg = new graphics::ArcDrawAble(IMAGE_WIDTH + 40, 35, 20, 4,
                                                 const_val, 3, 30, text_color);

  progress_group->add_child(progress_background);
  progress_group->add_child(progress_arc_bg);
  progress_group->add_child(progress_arc);
  progress_group->add_child(volume_arc_bg);
  progress_group->add_child(volume_local_cond);

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
      SCREEN_WIDTH / 2 - TFT_getStringWidth(version_text) / 2, 40, text_color,
      DEFAULT_FONT, version_text);

  TFT_setFont(DEJAVU18_FONT, NULL);

  char wifi_text[] = "WiFi";

  auto init_wifi = new graphics::TextDrawAble(
      SCREEN_WIDTH / 2 - TFT_getStringWidth(wifi_text) / 2 - 15, 60, text_color,
      DEJAVU18_FONT, wifi_text);

  auto init_wifi_img_err =
      new graphics::BmpPathDrawAble(110, 52, "/spiffs/err.bmp");
  auto init_wifi_img_ok =
      new graphics::BmpPathDrawAble(110, 52, "/spiffs/ok.bmp");

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

  auto main_cond = new graphics::ConditionalDrawAble(
      []() {
        return spotify_manager->current_state.initialized &
               https_requester->connected;
      },
      active_group, init_group);

  active_group->add_child(track_group);
  active_group->add_child(image_group);
  active_group->add_child(progress_group);

  graphics_manager->add_to_base(init_group);

  graphics_manager->register_event(NEW_SONG, track_group);
  graphics_manager->register_event(NEW_IMAGE, image_group);
  graphics_manager->register_event(STATUS_CHANGED, main_cond);
  graphics_manager->register_event(PROGRESS_ADVANCE, progress_arc);
  graphics_manager->register_event(NEW_SONG, progress_arc_bg);
  graphics_manager->register_event(VOLUME_CHANGED, volume_arc_bg);
  graphics_manager->register_event(VOLUME_CHANGED, volume_local_cond);

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
      GPIO_NUM_21, []() { spotify_manager->request_next_track(); }));

  io_manager->add_button(new io::ButtonInput(
      GPIO_NUM_19, []() { spotify_manager->request_toggle_playback(); }));

  io_manager->add_button(new io::ButtonInput(
      GPIO_NUM_18, []() { spotify_manager->request_previous_track(); }));

  io_manager->add_button(new io::ButtonInput(
      GPIO_NUM_23, []() { spotify_manager->change_local_volume(-5); }));

  io_manager->add_button(new io::ButtonInput(
      GPIO_NUM_4, []() { spotify_manager->request_set_volume(); }));

  io_manager->add_button(new io::ButtonInput(
      GPIO_NUM_15, []() { spotify_manager->change_local_volume(+5); }));

  // --- Create Tasks ---

  xTaskCreate(&main_update, "main_update", 8192, NULL, 5, NULL);
  xTaskCreate(&https_requester_update, "https_update", 4096, NULL, 5, NULL);
  xTaskCreate(&spotify_update, "spotify_update", 3072, NULL, 5, NULL);
  xTaskCreate(&graphics_update, "graphics_update", 2048, NULL, 5, NULL);
}
