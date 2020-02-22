
#ifndef GRAPHICS_GRAPHICS_TYPES_H
#define GRAPHICS_GRAPHICS_TYPES_H

#include "stdint.h"
#include "tft.h"
#include "tftspi.h"
#include <vector>

namespace graphics {

class IDrawAble {
public:
  virtual void draw();
  virtual ~IDrawAble() = default;
};

class DrawGroup : public IDrawAble {
private:
  std::vector<IDrawAble *> children;

public:
  virtual ~DrawGroup();
  void add_child(IDrawAble *child);

  void draw() override;
};

class PositionedDrawAble : public IDrawAble {
protected:
  uint8_t x;
  uint8_t y;

  PositionedDrawAble(uint8_t n_x, uint8_t n_y) : x(n_x), y(n_y) {}

public:
  virtual ~PositionedDrawAble() = default;
};

class RectangleDrawAble : public PositionedDrawAble {
protected:
  uint8_t width;
  uint8_t height;
  color_t *color;

  bool fill;

public:
  RectangleDrawAble(uint8_t n_x, uint8_t n_y, uint8_t n_width, uint8_t n_height,
                    color_t *n_color, bool n_fill)
      : PositionedDrawAble(n_x, n_y), width(n_width), height(n_height),
        color(n_color), fill(n_fill) {}

  virtual ~RectangleDrawAble() = default;

  void draw() override;
};

} // namespace graphics

#endif