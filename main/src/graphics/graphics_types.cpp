#include "graphics_types.h"

namespace graphics {

DrawGroup::~DrawGroup() {
  for (auto child : children) {
    delete child;
  }
}

void DrawGroup::add_child(IDrawAble *child) { children.push_back(child); }

void DrawGroup::draw() {
  for (auto child : children) {
    child->draw();
  }
}

void RectangleDrawAble::draw() {
  if (fill) {
    TFT_fillRect(x, y, width, height, *color);
  } else {
    TFT_drawRect(x, y, width, height, *color);
  }
}

} // namespace graphics
