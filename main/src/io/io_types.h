#ifndef IO_IO_TYPES_H
#define IO_IO_TYPES_H

#include <functional>

#include "rotary_encoder.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

extern QueueHandle_t buttons_queue;

namespace io {

/**
 * @brief A single push style button.
 */
class ButtonInput {
private:
  std::function<void()> call_back;
  int64_t last_pressed = 0;

public:
  const gpio_num_t pin;
  ButtonInput(gpio_num_t gpio_pin, std::function<void()> callback);

  void on_pin_down();
};

/**
 * @brief A rotary encoder input.
 */
class RotaryInput {
private:
  gpio_num_t pin_a;
  gpio_num_t pin_b;
  std::function<void(int8_t dir)> call_back;

  QueueHandle_t queue_handle;

public:
  RotaryInput(gpio_num_t a, gpio_num_t b,
              std::function<void(int8_t dir)> callback);

  void check_queue();
};

} // namespace io

#endif