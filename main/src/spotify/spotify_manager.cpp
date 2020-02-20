//
// Created by manuel on 25.10.19.
//

#include "spotify_manager.h"

extern std::string CLIENT_ID;
extern std::string CLIENT_SECRET;
extern std::string REFRESH_TOKEN;

extern network::HttpsRequester *https_requester;

namespace spotify {

SpotifyManager::SpotifyManager() { request_refresh_access_token(); }

SpotifyManager::~SpotifyManager() {}

void SpotifyManager::request_refresh_access_token() {
  access_token_retry_times = ACCESS_TOKEN_RETRY_AMOUNT;
  network::Request *request_data = new network::Request(
      "accounts.spotify.com", "/api/token", "POST",
      [this](network::Response *response_data) {
        if (response_data->was_success()) {
          auto json_ptr = response_data->get_body_as_json();
          const cJSON *body_json = json_ptr.get();
          if (cJSON_HasObjectItem(body_json, "error")) {
            ESP_LOGE(LOG_TAG, "Error while requesting access code: %s",
                     cJSON_GetObjectItem(body_json, "error")->valuestring);
          } else if (cJSON_HasObjectItem(body_json, "access_token")) {
            cJSON *json_token = cJSON_GetObjectItem(body_json, "access_token");
            access_token = std::string(json_token->valuestring);

            access_token_received = true;
            if (!initialized) {
              request_update_local_player();
              request_update_local_volume();
              initialized = true;
            }

            ESP_LOGI(LOG_TAG, "Access token received successfully: %s",
                     access_token.c_str());
          }
        } else {
          if (access_token_retry_times != 0) {
            ESP_LOGE(LOG_TAG,
                     "Request to initialize accesstoken was unsuccessfull, "
                     "retrying %i times",
                     access_token_retry_times);
            access_token_retry_times--;
            request_refresh_access_token();
          } else {
            ESP_LOGE(LOG_TAG,
                     "Could not receive access token after several tries ...");
          }
        }
      });
  request_data->add_body_data(
      network::KeyValuePair("grant_type", "refresh_token"));
  request_data->add_body_data(
      network::KeyValuePair("refresh_token", REFRESH_TOKEN));
  request_data->add_body_data(network::KeyValuePair("client_id", CLIENT_ID));
  request_data->add_body_data(
      network::KeyValuePair("client_secret", CLIENT_SECRET));

  https_requester->add_request_to_queue(request_data);
}

void SpotifyManager::request_update_local_player() {
  if (!access_token_received) {
    ESP_LOGE(LOG_TAG, "Can't call API before receiving access token!");
    return;
  }
  network::Request *request_data = new network::Request(
      "api.spotify.com", "/v1/me/player/currently-playing", "GET",
      [this](network::Response *response_data) {
        if (response_data->was_success()) {
          auto json_ptr = response_data->get_body_as_json();
          const cJSON *body_json = json_ptr.get();
          if (cJSON_HasObjectItem(body_json, "error")) {
            ESP_LOGE(LOG_TAG, "Error while update local player: %s",
                     cJSON_GetObjectItem(body_json, "error")->valuestring);
          } else {
            const cJSON *item_json = cJSON_GetObjectItem(body_json, "item");

            current_state.is_playing =
                cJSON_IsTrue(cJSON_GetObjectItem(body_json, "is_playing"));

            uint64_t current_time = esp_timer_get_time();

            int duration =
                cJSON_GetObjectItem(item_json, "duration_ms")->valueint;
            int progress =
                cJSON_GetObjectItem(body_json, "progress_ms")->valueint;

            current_state.end_time =
                current_time + uint64_t(1000 * (duration - progress));

            current_state.start_time = current_time - uint64_t(progress);

            char *track_name =
                cJSON_GetObjectItem(item_json, "name")->valuestring;

            ESP_LOGI(LOG_TAG, "Updated local player, Track: %s", track_name);

            current_state.current_track = std::string(std::move(track_name));
          }
        } else {
          ESP_LOGE(LOG_TAG, "Request to update local player was unsuccessfull");
        }
      });
  request_data->add_header_data(
      network::KeyValuePair("Authorization", "Bearer " + access_token));

  https_requester->add_request_to_queue(request_data);
}

void SpotifyManager::request_update_local_volume() {
  if (!access_token_received) {
    ESP_LOGE(LOG_TAG, "Can't call API before receiving access token!");
    return;
  }
  network::Request *request_data = new network::Request(
      "api.spotify.com", "/v1/me/player/devices", "GET",
      [this](network::Response *response_data) {
        if (response_data->was_success()) {
          auto json_ptr = response_data->get_body_as_json();
          const cJSON *body_json = json_ptr.get();
          if (cJSON_HasObjectItem(body_json, "error")) {
            ESP_LOGE(LOG_TAG, "Error while update local volume: %s",
                     cJSON_GetObjectItem(body_json, "error")->valuestring);
          } else {
            auto devices_array = cJSON_GetObjectItem(body_json, "devices");

            cJSON device_struct;
            cJSON *device = &device_struct;
            cJSON_ArrayForEach(device, devices_array) {
              if (cJSON_IsTrue(cJSON_GetObjectItem(device, "is_active"))) {
                current_state.volume =
                    cJSON_GetObjectItem(device, "volume_percent")->valueint;
              }
            }

            ESP_LOGI(LOG_TAG, "Updated local volume, new volume %hhu",
                     current_state.volume);
          }
        } else {
          ESP_LOGE(LOG_TAG, "Request to update local volume was unsuccessfull");
        }
      });
  request_data->add_header_data(
      network::KeyValuePair("Authorization", "Bearer " + access_token));

  https_requester->add_request_to_queue(request_data);
}

void SpotifyManager::request_toggle_playback() {
  if (!access_token_received) {
    ESP_LOGE(LOG_TAG, "Can't call API before receiving access token!");
    return;
  }
  network::Request *request_data;
  if (current_state.is_playing) {
    request_data = new network::Request(
        "api.spotify.com", "/v1/me/player/pause", "PUT",
        [this](network::Response *response_data) {
          current_state.is_playing = false;
        },
        false);
  } else {
    request_data = new network::Request(
        "api.spotify.com", "/v1/me/player/play", "PUT",
        [this](network::Response *response_data) {
          current_state.is_playing = true;
        },
        false);
  }
  request_data->add_header_data(
      network::KeyValuePair("Authorization", "Bearer " + access_token));
  https_requester->add_request_to_queue(request_data);
}

void SpotifyManager::request_set_volume() {
  network::Request *request_data = new network::Request(
      "api.spotify.com", "/v1/me/player/volume", "PUT",
      [](network::Response *response_data) {}, false);
  request_data->add_header_data(
      network::KeyValuePair("Authorization", "Bearer " + access_token));
  request_data->add_query_data(network::KeyValuePair(
      "volume_percent", std::to_string(current_state.volume)));

  https_requester->add_request_to_queue(request_data);
}

void SpotifyManager::request_next_track() {
  if (!access_token_received) {
    ESP_LOGE(LOG_TAG, "Can't call API before receiving access token!");
    return;
  }
  network::Request *request_data = new network::Request(
      "api.spotify.com", "/v1/me/player/next", "POST",
      [this](network::Response *response_data) {
        request_update_local_player();
      },
      false);
  request_data->add_header_data(
      network::KeyValuePair("Authorization", "Bearer " + access_token));

  https_requester->add_request_to_queue(request_data);
}

void SpotifyManager::request_previous_track() {
  if (!access_token_received) {
    ESP_LOGE(LOG_TAG, "Can't call API before receiving access token!");
    return;
  }
  network::Request *request_data = new network::Request(
      "api.spotify.com", "/v1/me/player/previous", "POST",
      [this](network::Response *response_data) {
        request_update_local_player();
      },
      true);
  request_data->add_header_data(
      network::KeyValuePair("Authorization", "Bearer " + access_token));

  https_requester->add_request_to_queue(request_data);
}

void SpotifyManager::change_local_volume(int8_t dif) {
  int new_val = int(current_state.volume) + int(dif);
  if (new_val > 100) {
    current_state.volume = 100;
  } else if (new_val < 0) {
    current_state.volume = 0;
  } else {
    current_state.volume += dif;
  }
  ESP_LOGI(LOG_TAG, "Changed local volume to: %hhu", current_state.volume);
}

} // namespace spotify