//
// Created by manuel on 25.10.19.
//

#ifndef ESPIFY_SRC_NETWORK_SPOTIFY_MANAGER_H_
#define ESPIFY_SRC_NETWORK_SPOTIFY_MANAGER_H_

#include <string>

#include "../network/http_types.h"
#include "../network/https_requester.h"
#include "spotify_types.h"

#define ACCESS_TOKEN_RETRY_AMOUNT 5

namespace spotify {

/**
 * @brief Manages the playback state and changes it.
 */
class SpotifyManager {
private:
  const char *LOG_TAG = "SpotifyManager";

  bool initialized = false;

  std::string access_token;
  uint16_t access_token_retry_times;
  bool access_token_received = false;

public:
  spotify::SpotifyState current_state;

  SpotifyManager();
  ~SpotifyManager();

  /**
   * @brief Refreshes the access token.
   * If successfull sets access_token_received to true.
   * Other request can't be made before the access token is received.
   */
  void request_refresh_access_token();

  /**
   * @brief Gets and updates the current state of the player
   */
  void request_update_local_track();

  void request_update_local_volume();

  void request_track_image(std::string &&url_string);

  /**
   * @brief Stops or starts the playback.
   */
  void request_toggle_playback();

  void request_set_volume();

  /**
   * @brief Skips to the next song.
   */
  void request_next_track();

  /**
   * @brief Skips to the last song.
   */
  void request_previous_track();

  void change_local_volume(int8_t dif);

  void update();
};

} // namespace spotify

#endif // ESPIFY_SRC_NETWORK_SPOTIFY_MANAGER_H_
