#include "io_manager.h"

#include "esp_log.h"

io::IOManager::IOManager() {
  ESP_ERROR_CHECK(gpio_install_isr_service(0));
  buttons_queue = xQueueCreate(10, sizeof(uint32_t));
}

io::IOManager::~IOManager() {
  for (auto button : buttons) {
    delete button;
  }
  for (auto rotary : rotarys) {
    delete rotary;
  }
}

void io::IOManager::add_button(io::ButtonInput *button) {
  buttons.push_back(button);
}

void io::IOManager::add_rotary(io::RotaryInput *rotary) {
  rotarys.push_back(rotary);
}

void io::IOManager::update() {
  uint32_t triggered_pin;
  if (xQueueReceive(buttons_queue, &triggered_pin, 0)) {
    // ESP_LOGI("IOManager", "Triggered %i button", triggered_pin);
    for (auto button : buttons) {
      if ((uint32_t)button->pin == triggered_pin) {
        button->on_pin_down();
      }
    }
  }

  for (auto rotary : rotarys) {
    rotary->check_queue();
  }
}