#ifndef IO_IO_TYPES_H
#define IO_IO_TYPES_H

#include <functional>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

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

} // namespace io

#endif