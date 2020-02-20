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

  if (last_pressed + 1000000 < current_time) {
    call_back();
    last_pressed = current_time;
  }
}

RotaryInput::RotaryInput(gpio_num_t a, gpio_num_t b,
                         std::function<void(int8_t dir)> callback)
    : pin_a(a), pin_b(b), call_back(callback) {
  rotary_encoder_info_t info;
  ESP_ERROR_CHECK(rotary_encoder_init(&info, pin_a, pin_b));
  ESP_ERROR_CHECK(rotary_encoder_enable_half_steps(&info, true));

  queue_handle = rotary_encoder_create_queue();
  ESP_ERROR_CHECK(rotary_encoder_set_queue(&info, queue_handle));
}

void RotaryInput::check_queue() {
  rotary_encoder_event_t event;
  if (xQueueReceive(queue_handle, &event, 1000 / portTICK_PERIOD_MS) ==
      pdTRUE) {
    call_back(event.state.direction == ROTARY_ENCODER_DIRECTION_CLOCKWISE ? 1
                                                                          : -1);
  }
}

} // namespace io