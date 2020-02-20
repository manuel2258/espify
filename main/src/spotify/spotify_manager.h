//
// Created by manuel on 25.10.19.
//

#ifndef ESPIFY_SRC_NETWORK_SPOTIFY_MANAGER_H_
#define ESPIFY_SRC_NETWORK_SPOTIFY_MANAGER_H_

#include <string>

#include "../network/http_types.h"
#include "../network/https_requester.h"

#define ACCESS_TOKEN_RETRY_AMOUNT 5

namespace spotify {

/**
 * @brief Manages the playback state and changes it.
 */
class SpotifyManager {
private:
  const char *LOG_TAG = "SpotifyManager";

  std::string access_token;
  uint16_t access_token_retry_times;
  bool access_token_received = false;

  bool is_playing;

public:
  SpotifyManager();
  ~SpotifyManager();

  /**
   * @brief Refreshes the access token.
   * If successfull sets access_token_received to true.
   * Other request can't be made before the access token is received.
   */
  void refresh_access_token();

  /**
   * @brief Gets and updates the current state of the player
   */
  void update_local_player();

  /**
   * @brief Stops or starts the playback.
   */
  void toggle_playback();

  // void set_volume(int const volume) const;

  /**
   * @brief Skips to the next song.
   */
  void next_track() const;

  /**
   * @brief Skips to the last song.
   */
  void previous_track() const;
};

} // namespace spotify

#endif // ESPIFY_SRC_NETWORK_SPOTIFY_MANAGER_H_
