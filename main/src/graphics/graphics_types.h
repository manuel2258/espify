
#ifndef GRAPHICS_GRAPHICS_TYPES_H
#define GRAPHICS_GRAPHICS_TYPES_H

#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include "stdint.h"
#include "tft.h"
#include "tftspi.h"

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

class TextPtrDrawAble : public PositionedDrawAble {
protected:
  color_t *color;
  uint8_t font;
  char **text_buf;

public:
  TextPtrDrawAble(uint8_t n_x, uint8_t n_y, color_t *n_color, uint8_t n_font,
                  char **buf_adr_ptr)
      : PositionedDrawAble(n_x, n_y), color(n_color), font(n_font),
        text_buf(buf_adr_ptr) {}

  virtual ~TextPtrDrawAble() = default;

  void draw() override;
};

class TextDrawAble : public PositionedDrawAble {
protected:
  color_t *color;
  uint8_t font;
  char *text;

public:
  TextDrawAble(uint8_t n_x, uint8_t n_y, color_t *n_color, uint8_t n_font,
               std::string n_text)
      : PositionedDrawAble(n_x, n_y), color(n_color), font(n_font) {
    auto text_src = n_text.c_str();
    text = new char[n_text.size() + 1];
    memcpy(text, text_src, n_text.size() + 1);
  }

  virtual ~TextDrawAble();

  void draw() override;
};

class JpegBufferDrawAble : public PositionedDrawAble {
private:
  uint8_t **image_buf;
  int *buf_size;

public:
  JpegBufferDrawAble(uint8_t n_x, uint8_t n_y, uint8_t **buf_adr_ptr,
                     int *buf_size_ptr)
      : PositionedDrawAble(n_x, n_y), image_buf(buf_adr_ptr),
        buf_size(buf_size_ptr) {}

  virtual ~JpegBufferDrawAble() = default;

  void draw() override;
};

class BmpPathDrawAble : public PositionedDrawAble {
private:
  char *bmp_path;

public:
  BmpPathDrawAble(uint8_t n_x, uint8_t n_y, std::string &&path)
      : PositionedDrawAble(n_x, n_y) {
    auto path_src = path.c_str();
    bmp_path = new char[path.size() + 1];
    memcpy(bmp_path, path_src, path.size() + 1);
  }

  virtual ~BmpPathDrawAble();

  void draw() override;
};

class ConditionalDrawAble : public IDrawAble {
private:
  std::function<bool()> condition;

  IDrawAble *true_drawable = nullptr;
  IDrawAble *false_drawable = nullptr;

public:
  ConditionalDrawAble(std::function<bool()> cond_func, IDrawAble *on_true,
                      IDrawAble *on_false)
      : condition(cond_func), true_drawable(on_true), false_drawable(on_false) {
  }

  virtual ~ConditionalDrawAble();

  void draw() override;
};

class ArcDrawAble : public PositionedDrawAble {
private:
  uint16_t r;
  uint16_t line_width;

  uint8_t *value_ptr;
  float value_scale;

  float start;

  color_t *color;

public:
  ArcDrawAble(uint8_t n_x, uint8_t n_y, uint16_t rad, uint16_t n_width,
              uint8_t *n_value, float n_scale, float n_start, color_t *n_color)
      : PositionedDrawAble(n_x, n_y), r(rad), line_width(n_width),
        value_ptr(n_value), value_scale(n_scale), start(n_start),
        color(n_color) {}

  virtual ~ArcDrawAble() = default;

  void draw() override;
};

class LineDrawAble : public PositionedDrawAble {
private:
  uint16_t length;

  color_t *color;

public:
  LineDrawAble(uint8_t n_x, uint8_t n_y, uint16_t n_length, color_t *n_color)
      : PositionedDrawAble(n_x, n_y), length(n_length), color(n_color) {}

  virtual ~LineDrawAble() = default;

  void draw() override;
};

} // namespace graphics

#endif