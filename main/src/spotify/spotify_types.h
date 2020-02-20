#ifndef SPOTIFY_SPOTIFY_TYPES_H
#define SPOTIFY_SPOTIFY_TYPES_H

#include <string>

namespace spotify {

struct SpotifyState {
  bool is_playing;

  std::string current_track;

  uint64_t end_time;
  uint64_t start_time;

  uint8_t volume;
};

} // namespace spotify

#endif