#include "graphics_types.h"

#include "esp_log.h"

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

void TextPtrDrawAble::draw() {
  if (*text_buf != nullptr) {
    tft_fg = *color;
    TFT_setFont(font, NULL);
    TFT_print(*text_buf, x, y);
  }
}

void TextDrawAble::draw() {
  tft_fg = *color;
  TFT_setFont(font, NULL);
  TFT_print(text, x, y);
}

TextDrawAble::~TextDrawAble() { delete[] text; }

void JpegBufferDrawAble::draw() {
  if (*image_buf != nullptr) {
    TFT_jpg_image(x, y, 0, NULL, *image_buf, *buf_size);
  }
}

void BmpPathDrawAble::draw() { TFT_bmp_image(x, y, 0, bmp_path, NULL, 0); }

BmpPathDrawAble::~BmpPathDrawAble() { delete[] bmp_path; }

void ConditionalDrawAble::draw() {
  if (condition()) {
    if (true_drawable != nullptr) {
      true_drawable->draw();
    }
  } else {
    if (false_drawable != nullptr) {
      false_drawable->draw();
    }
  }
}

ConditionalDrawAble::~ConditionalDrawAble() {
  if (true_drawable != nullptr) {
    delete true_drawable;
  }
  if (false_drawable != nullptr) {
    delete false_drawable;
  }
}

void ArcDrawAble::draw() {
  TFT_drawArc(x, y, r, line_width, start, start + *value_ptr * value_scale,
              *color, *color);
}

void LineDrawAble::draw() { TFT_drawLine(x, y, x + length, y, *color); }

} // namespace graphics
