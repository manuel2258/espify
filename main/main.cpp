// . $HOME/esp-idf/export.sh

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "src/io/io_manager.h"
#include "src/io/io_types.h"
#include "src/network/https_requester.h"
#include "src/spotify/spotify_manager.h"

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

void spotify_refresh_update(void *pv_pars) {
  for (;;) {
    vTaskDelay(portTICK_PERIOD_MS * 36000);
    spotify_manager->refresh_access_token();
  }
}

extern "C" void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  https_requester = new network::HttpsRequester();
  spotify_manager = new spotify::SpotifyManager();
  io_manager = new io::IOManager();

  io_manager->add_button(new io::ButtonInput(
      GPIO_NUM_18, []() { spotify_manager->next_track(); }));

  io_manager->add_button(new io::ButtonInput(
      GPIO_NUM_19, []() { spotify_manager->previous_track(); }));

  io_manager->add_button(new io::ButtonInput(
      GPIO_NUM_21, []() { spotify_manager->toggle_playback(); }));

  xTaskCreate(&main_update, "main_update", 8192, NULL, 5, NULL);
  xTaskCreate(&https_requester_update, "https_update", 4096, NULL, 5, NULL);
  xTaskCreate(&spotify_refresh_update, "spotify_refresh_update", 2048, NULL, 5,
              NULL);
}
