#ifndef SPOTIFY_SPOTIFY_TYPES_H
#define SPOTIFY_SPOTIFY_TYPES_H

#include <string>

#include "../graphics/graphics_types.h"

namespace spotify {

struct Track {
  graphics::DynamicText track_name = {};
  graphics::DynamicText artist_name = {};

  uint8_t *image_buf = nullptr;
  int buf_size;

  uint64_t length_time = 0;
  uint64_t start_time = 0;

  uint8_t progress = 0;
};

struct SpotifyState {
  bool is_playing = false;
  bool initialized = false;

  Track track;

  uint8_t volume = 0;
  uint8_t local_volume = 0;

  uint8_t volume_change_counter = 0;

  char *device_id = nullptr;
};

} // namespace spotify

#endif