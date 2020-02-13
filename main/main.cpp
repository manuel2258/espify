// . $HOME/esp-idf/export.sh

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "src/network/HttpsRequester.h"

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

extern "C" void app_main(void) {
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_event_loop_create_default());

  https_requester = new network::HttpsRequester();

  auto request_data =
      new network::Request("accounts.spotify.com", "/api/token", "POST",
                           [](network::Response *response_data) {
                             ESP_LOGI("Main", "Received response!");
                             delete response_data;
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

  while (true)
    https_requester->update();
}

// void wifi_update(void) { https_requester->update(); }
