#ifndef GRAPHICS_GRAPHICS_MANANGER_H
#define GRAPHICS_GRAPHICS_MANANGER_H

#include <unordered_map>
#include <vector>

#include "FreeRTOS.h"
#include "freertos/event_groups.h"

#include "graphics_types.h"

namespace graphics {

/**
 * @brief Manages and triggers drawables.
 */
class GraphicsManager {

private:
  const char *LOG_TAG = "GraphicsManager";

  std::unordered_map<EventBits_t, std::vector<IDrawAble *>> event_map;

  EventBits_t full_mask = 0;

  graphics::DrawGroup base_group;

public:
  GraphicsManager();

  /**
   * @brief Adds a drawable to the base drawable group
   */
  void add_to_base(IDrawAble *draw_able);

  /**
   * @brief Draws all current added drawables.
   */
  void draw_all();

  /**
   * @brief Updates the Event-System
   *
   */
  void update();

  /**
   * @brief Registers a new Event.
   * @param mask The to trigger at mask.
   * @param draw_able The to draw drawable.
   */
  void register_event(EventBits_t mask, IDrawAble *draw_able);

  /**
   * @brief Removes a Event.
   * Removes mask vector and rebuilds mask if nesserary.
   * @param mask
   * @param draw_able
   */
  void unregister_event(EventBits_t mask, IDrawAble *draw_able);
};

} // namespace graphics

#endif