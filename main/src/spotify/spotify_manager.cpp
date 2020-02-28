//
// Created by manuel on 25.10.19.
//

#include "spotify_manager.h"

#include <algorithm>
#include <cstring>

#include "freertos/event_groups.h"

#include "../graphics/graphics_events.h"

extern std::string CLIENT_ID;
extern std::string CLIENT_SECRET;
extern std::string REFRESH_TOKEN;

extern network::HttpsRequester *https_requester;
extern EventGroupHandle_t graphics_events_handle;

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
            if (!current_state.initialized) {
              current_state.initialized = true;
              xEventGroupSetBits(graphics_events_handle, STATUS_CHANGED);
              request_update_local_track();
              request_update_local_volume();
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

void SpotifyManager::request_update_local_track() {
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

            // Parse is playing
            current_state.is_playing =
                cJSON_IsTrue(cJSON_GetObjectItem(body_json, "is_playing"));

            if (!current_state.is_playing) {
              return;
            }

            uint64_t current_time = esp_timer_get_time();

            // Parse duration and progress
            uint64_t duration =
                1000 * cJSON_GetObjectItem(item_json, "duration_ms")->valueint;
            uint64_t progress =
                1000 * cJSON_GetObjectItem(body_json, "progress_ms")->valueint;

            current_state.track.length_time = duration;

            current_state.track.start_time = current_time - progress;

            // Parse track name
            char *track_name =
                cJSON_GetObjectItem(item_json, "name")->valuestring;

            if (current_state.track.track_name != nullptr) {
              if (strcmp(track_name, current_state.track.track_name) == 0) {
                ESP_LOGI(LOG_TAG, "Updated local player, still same track");
                return;
              }
            }

            size_t str_len = strlen(track_name);

            // Delete old buffer
            if (current_state.track.track_name != nullptr) {
              delete[] current_state.track.track_name;
            }

            // Then create new one and copy char array into it
            current_state.track.track_name = new char[str_len + 1];
            std::copy(track_name, track_name + str_len + 1,
                      current_state.track.track_name);

            // Parse artist
            const cJSON *artists_json =
                cJSON_GetObjectItem(item_json, "artists");
            const cJSON *artist_json = cJSON_GetArrayItem(artists_json, 0);
            char *artist_name =
                cJSON_GetObjectItem(artist_json, "name")->valuestring;

            str_len = strlen(artist_name);

            if (current_state.track.artist_name != nullptr) {
              delete[] current_state.track.artist_name;
            }

            current_state.track.artist_name = new char[str_len + 1];
            std::copy(artist_name, artist_name + str_len + 1,
                      current_state.track.artist_name);

            ESP_LOGI(LOG_TAG, "Updated local player, new Track: %s->%s",
                     current_state.track.artist_name,
                     current_state.track.track_name);

            xEventGroupSetBits(graphics_events_handle, NEW_SONG);

            auto images_json = cJSON_GetObjectItem(
                cJSON_GetObjectItem(item_json, "album"), "images");

            cJSON image_struct;
            cJSON *image = &image_struct;
            cJSON_ArrayForEach(image, images_json) {
              int height = cJSON_GetObjectItem(image, "height")->valueint;
              if (height < 70 && height > 60) {
                request_track_image(std::string(
                    cJSON_GetObjectItem(image, "url")->valuestring));
              }
            }
          }
        } else {
          ESP_LOGE(LOG_TAG, "Request to update local player was unsuccessfull");
        }
      } // namespace spotify
  );
  request_data->add_header_data(
      network::KeyValuePair("Authorization", "Bearer " + access_token));

  https_requester->add_request_to_queue(request_data);
}

void SpotifyManager::request_update_local_volume() {
  if (!access_token_received) {
    ESP_LOGE(LOG_TAG, "Can't call API before receiving access token!");
    return;
  }

  if (current_state.volume_change_counter > 0) {
    ESP_LOGI(LOG_TAG, "Can't request volume as its being changed right now");
    return;
  }
  auto request_data = new network::Request(
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
                current_state.local_volume = current_state.volume;
                ESP_LOGI(LOG_TAG, "Got new volume: %u", current_state.volume);
                xEventGroupSetBits(graphics_events_handle, VOLUME_CHANGED);
              }
            }
          }
        } else {
          ESP_LOGE(LOG_TAG, "Request to update local volume was unsuccessfull");
        }
      });
  request_data->add_header_data(
      network::KeyValuePair("Authorization", "Bearer " + access_token));

  https_requester->add_request_to_queue(request_data);
}

void SpotifyManager::request_track_image(std::string &&full_url) {
  // Cut of https://
  auto url = full_url.substr(8);
  int sep = url.find('/');

  // Seperate host and endpoint
  auto host = url.substr(0, sep);
  auto endpoint = url.substr(sep);
  auto request_data = new network::Request(
      std::move(host), std::move(endpoint), "GET",
      [this](network::Response *response_data) {
        if (response_data->was_success()) {
          auto body_str = response_data->get_body_raw();

          int body_length = body_str->length();
          if (current_state.track.image_buf != nullptr) {
            delete[] current_state.track.image_buf;
          }
          current_state.track.image_buf = new uint8_t[body_length];
          memcpy(current_state.track.image_buf, body_str->c_str(), body_length);
          current_state.track.buf_size = body_length;
          ESP_LOGI(LOG_TAG, "Copied image buffer, length: %i", body_length);

          xEventGroupSetBits(graphics_events_handle, NEW_IMAGE);
        } else {
          ESP_LOGE(LOG_TAG, "Request to receive track image was unsuccessfull");
        }
      });

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
      [this](network::Response *response_data) {
        request_update_local_volume();
      },
      false);
  request_data->add_header_data(
      network::KeyValuePair("Authorization", "Bearer " + access_token));
  request_data->add_query_data(network::KeyValuePair(
      "volume_percent", std::to_string(current_state.local_volume)));

  https_requester->add_request_to_queue(request_data);
}

void SpotifyManager::request_next_track() {
  if (!access_token_received) {
    ESP_LOGE(LOG_TAG, "Can't call API before receiving access token!");
    return;
  }
  network::Request *request_data = new network::Request(
      "api.spotify.com", "/v1/me/player/next", "POST",
      [this](network::Response *response_data) {}, false);
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
      [this](network::Response *response_data) {}, true);
  request_data->add_header_data(
      network::KeyValuePair("Authorization", "Bearer " + access_token));

  https_requester->add_request_to_queue(request_data);
}

void SpotifyManager::change_local_volume(int8_t dif) {
  int new_val = int(current_state.local_volume) + int(dif);
  if (new_val > 100) {
    current_state.local_volume = 100;
  } else if (new_val < 0) {
    current_state.local_volume = 0;
  } else {
    current_state.local_volume += dif;
    xEventGroupSetBits(graphics_events_handle, VOLUME_CHANGED);
  }
  current_state.volume_change_counter = 2;
  ESP_LOGI(LOG_TAG, "Changed local volume to: %u", current_state.local_volume);
}

void SpotifyManager::update() {
  if (current_state.volume_change_counter > 0) {
    ESP_LOGI(LOG_TAG, "Volume change counter: %u",
             current_state.volume_change_counter);
    current_state.volume_change_counter--;
    if (current_state.volume_change_counter == 0) {
      current_state.local_volume = current_state.volume;
      xEventGroupSetBits(graphics_events_handle, VOLUME_CHANGED);
    }
  }
  // Update Track Progress
  if (!current_state.is_playing) {
    return;
  }
  uint64_t current_time = esp_timer_get_time();

  uint64_t current_progress = current_time - current_state.track.start_time;

  current_state.track.progress =
      (float(current_progress) / current_state.track.length_time) * 100;

  if (current_state.track.progress > 100) {
    current_state.track.progress = 100;
    request_update_local_track();
  } else {
    xEventGroupSetBits(graphics_events_handle, PROGRESS_ADVANCE);
  }
}

} // namespace spotify