#include "graphics_types.h"

#include "esp_log.h"

#include "graphics_events.h"
#include "graphics_manager.h"

extern graphics::GraphicsManager *graphics_manager;

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
  if (text->text != nullptr) {
    if (last_id != text->id) {
      last_id = text->id;
      offset = 0;
      initial_drawn = false;

      TFT_setFont(font, NULL);
      if (TFT_getStringWidth(text->text) > width) {
        char *buf = text->text;
        while (TFT_getStringWidth(buf) > width) {
          buf++;
        }
        length = buf - text->text;
        dynamic = true;
        if (!update_registered) {
          graphics_manager->register_event(SECOND_OVER, this);
          update_registered = true;
          ESP_LOGI("TextPtr", "Registered update event");
        }
      } else {
        dynamic = false;
        if (update_registered) {
          graphics_manager->unregister_event(SECOND_OVER, this);
          update_registered = false;
          ESP_LOGI("TextPtr", "Unregistered update event");
        }
      }
    }

    if (dynamic || !initial_drawn) {
      initial_drawn = true;
      TFT_fillRect(x, y, width + 5, text_height, *bg_color);

      tft_fg = *color;
      TFT_setFont(font, NULL);
      TFT_print(text->text + offset, x, y);

      offset++;
      if (offset > length) {
        offset = 0;
      }
    }
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
