//
// Created by manuel on 25.10.19.
//

#include "spotify_manager.h"

extern std::string CLIENT_ID;
extern std::string CLIENT_SECRET;
extern std::string REFRESH_TOKEN;

extern network::HttpsRequester *https_requester;

namespace spotify {

SpotifyManager::SpotifyManager() { refresh_access_token(); }

SpotifyManager::~SpotifyManager() {}

void SpotifyManager::refresh_access_token() {
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
            update_local_player();

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
            refresh_access_token();
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

void SpotifyManager::update_local_player() {
  if (!access_token_received) {
    ESP_LOGE(LOG_TAG, "Can't call API before receiving access token!");
    return;
  }
  network::Request *request_data = new network::Request(
      "api.spotify.com", "/v1/me/player", "GET",
      [this](network::Response *response_data) {
        if (response_data->was_success()) {
          auto json_ptr = response_data->get_body_as_json();
          const cJSON *body_json = json_ptr.get();
          if (cJSON_HasObjectItem(body_json, "error")) {
            ESP_LOGE(LOG_TAG, "Error while update local player: %s",
                     cJSON_GetObjectItem(body_json, "error")->valuestring);
          } else {

            cJSON *json_playing = cJSON_GetObjectItem(body_json, "is_playing");
            is_playing = cJSON_IsTrue(json_playing);
          }
        } else {
          ESP_LOGE(LOG_TAG, "Request to update local player was unsuccessfull");
        }
      });
  request_data->add_header_data(
      network::KeyValuePair("Authorization", "Bearer " + access_token));

  https_requester->add_request_to_queue(request_data);
}

void SpotifyManager::toggle_playback() {
  if (!access_token_received) {
    ESP_LOGE(LOG_TAG, "Can't call API before receiving access token!");
    return;
  }
  network::Request *request_data;
  if (is_playing) {
    request_data = new network::Request(
        "api.spotify.com", "/v1/me/player/pause", "PUT",
        [this](network::Response *response_data) { update_local_player(); },
        false);
  } else {
    request_data = new network::Request(
        "api.spotify.com", "/v1/me/player/play", "PUT",
        [this](network::Response *response_data) { update_local_player(); },
        false);
  }
  request_data->add_header_data(
      network::KeyValuePair("Authorization", "Bearer " + access_token));
  https_requester->add_request_to_queue(request_data);
}

/*

void SpotifyManager::set_volume(int const volume) const {
  network::Request *request_data = new network::Request(
      "api.spotify.com", "/v1/me/player/volume", "PUT",
      [](network::Response *response_data) {}, false);
  request_data->AddHeaderData("Authorization", "Bearer " + access_token_);
  request_data->AddQueryData("volume_percent", String(volume));

  http_manager_->AddRequestToQueue(request_data);
}*/

void SpotifyManager::next_track() const {
  if (!access_token_received) {
    ESP_LOGE(LOG_TAG, "Can't call API before receiving access token!");
    return;
  }
  network::Request *request_data = new network::Request(
      "api.spotify.com", "/v1/me/player/next", "POST",
      [](network::Response *response_data) {}, false);
  request_data->add_header_data(
      network::KeyValuePair("Authorization", "Bearer " + access_token));

  https_requester->add_request_to_queue(request_data);
}

void SpotifyManager::previous_track() const {
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

} // namespace spotify