#ifndef IO_IO_MANAGER_H
#define IO_IO_MANAGER_H

#include <vector>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "io_types.h"

namespace io {

/**
 * @brief Manages and updates Inputs.
 */
class IOManager {
private:
  std::vector<io::ButtonInput *> buttons;

public:
  IOManager();
  ~IOManager();

  void add_button(io::ButtonInput *i);

  /**
   * @brief Function to query events.
   */
  void update();
};

} // namespace io

#endif