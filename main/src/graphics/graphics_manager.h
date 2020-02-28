#ifndef GRAPHICS_GRAPHICS_MANANGER_H
#define GRAPHICS_GRAPHICS_MANANGER_H

#include <unordered_map>
#include <vector>

#include "FreeRTOS.h"
#include "freertos/event_groups.h"

#include "graphics_types.h"

namespace graphics {

class GraphicsManager {

private:
  const char *LOG_TAG = "GraphicsManager";

  std::unordered_map<EventBits_t, std::vector<IDrawAble *>> event_map;

  EventBits_t full_mask = 0;

  graphics::DrawGroup base_group;

public:
  GraphicsManager();

  void add_to_base(IDrawAble *draw_able);
  void draw_all();

  void update();
  void register_event(EventBits_t mask, IDrawAble *draw_able);
};

} // namespace graphics

#endif