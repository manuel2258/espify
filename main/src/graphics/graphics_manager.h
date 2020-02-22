#ifndef GRAPHICS_GRAPHICS_MANANGER_H
#define GRAPHICS_GRAPHICS_MANANGER_H

#include "graphics_types.h"

namespace graphics {

class GraphicsManager {

private:
  const char *LOG_TAG = "GraphicsManager";

  graphics::DrawGroup base_group;

public:
  GraphicsManager();

  void add_to_base(IDrawAble *draw_able);

  void draw_all();
};

} // namespace graphics

#endif