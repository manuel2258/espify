// . $HOME/esp-idf/export.sh

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "src/network/https_requester.h"

std::string AUTH_TOKEN =
    "AQBp6voO06wSFQu4Mv_i-mDoM_NeKH5W3AFJb32tOki2o9EnKPQYEcoGzWSonvOZ-1-"
    "Icmu6fcnI-Xa_TgvBXsTLiwbUlBE1pSsHerq4sJQHK-JLVzbRPWcvGlZ9ybu1CD3XIMoSM-"
    "Thg7e--y7wLHiqFiKbWbD_ZsgqP_CT94EG4Suz1_"
    "OrEM9ptKvNOhbqzpw8RMO8ioc7VwXIteVjYFuEQUVrDK_"
    "RRt7QiP23mxuHEeHAigBTF2wGHwwkqSsOYqF8TQ";

std::string CLIENT_ID = "e0fabf234d944aeba88bee39c39abc9f";
std::string CLIENT_SECRET = "fbe969d7f3c544dda1f607d129577017";

std::string REFRESH_TOKEN =
    "AQDzN3UESuB3G7RnIzM1-BrBpVd9Zc_"
    "XSlCo6Romsf7qZd58aDngQs3sUC5kbOgMNMVblNMQGzjiGEfHEQramwJiEMgN0SUMFmDIsY-"
    "i1KCxX9VJ-48aM3FTKknYYQSzegLm3A";

network::HttpsRequester *https_requester;

void main_update(void *pv_pars) {
  for (;;) {
    https_requester->trigger_response_callbacks();
    vTaskDelay(1);
  }
}

void https_requester_update(void *pv_pars) {
  https_requester->initialize_wifi();
  for (;;) {
    https_requester->update();
    vTaskDelay(1);
  }
}

extern "C" void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  https_requester = new network::HttpsRequester();

  auto request_data =
      new network::Request("accounts.spotify.com", "/api/token", "POST",
                           [](network::Response *response_data) {
                             ESP_LOGI("Main", "Received response: %s",
                                      response_data->get_body_raw()->c_str());
                           });
  request_data->add_body_data(
      network::KeyValuePair("grant_type", "authorization_code"));
  request_data->add_body_data(network::KeyValuePair("code", AUTH_TOKEN));
  request_data->add_body_data(
      network::KeyValuePair("redirect_uri", "https%3A%2F%2F2258studio.com"));
  request_data->add_body_data(network::KeyValuePair("client_id", CLIENT_ID));
  request_data->add_body_data(
      network::KeyValuePair("client_secret", CLIENT_SECRET));

  request_data->add_header_data(network::KeyValuePair(
      "Content-Type", "application/x-www-form-urlencoded"));

  https_requester->add_request_to_queue(request_data);

  xTaskCreate(&main_update, "main_update", 8192, NULL, 5, NULL);
  xTaskCreate(&https_requester_update, "https_update", 4096, NULL, 5, NULL);
}

// void wifi_update(void) { https_requester->update(); }
