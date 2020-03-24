#include "io_types.h"

#include "esp_log.h"

namespace io {

static void IRAM_ATTR input_buttons_isr_handler(void *arg) {
  uint32_t gpio_num = (uint32_t)arg;
  xQueueSendFromISR(buttons_queue, &gpio_num, NULL);
}

ButtonInput::ButtonInput(gpio_num_t gpio_pin, std::function<void()> callback)
    : call_back(callback), pin(gpio_pin) {

  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_NEGEDGE;
  io_conf.pin_bit_mask = 1ULL << (int)pin;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

  ESP_ERROR_CHECK(gpio_config(&io_conf));

  gpio_isr_handler_add(pin, input_buttons_isr_handler, (void *)pin);
}

void ButtonInput::on_pin_down() {
  int64_t current_time = esp_timer_get_time();

  if (last_pressed + 500000 < current_time) {
    call_back();
    last_pressed = current_time;
  }
}

} // namespace io