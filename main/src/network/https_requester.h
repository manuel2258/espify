#ifndef HTTPSREQUESTER_H
#define HTTPSREQUESTER_H

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <stdlib.h>
#include <string.h>

#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "esp_tls.h"

#include "http_types.h"
#include <queue>

#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY CONFIG_ESP_MAXIMUM_RETRY

#define HTTP_RECEIVE_BUFFER_SIZE 1024

extern const uint8_t
    server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t
    server_root_cert_pem_end[] asm("_binary_server_root_cert_pem_end");

namespace network {

class HttpsRequester {
private:
  const char *LOG_TAG = "HttpsRequester";

  // Whether the device is connected to a wifi.
  bool connected = false;

  // Locks to ensure threadsavety
  xSemaphoreHandle request_lock = xSemaphoreCreateBinary();
  xSemaphoreHandle response_lock = xSemaphoreCreateBinary();

  // Queues for request and responses
  std::queue<Request *> request_queue;
  std::queue<Response *> response_queue;

  // Queue for buffers to receive http responses.
  std::queue<char *> buffer_pool;

  // The settings to connect to the http host.
  esp_tls_cfg_t *tls_cfg;

  /**
   * @brief Makes a request to the given host.
   *
   * @param request The to make request object.
   * @return Response* The resulting response.
   */
  Response *make_request(Request *request);

public:
  HttpsRequester();
  ~HttpsRequester();

  /**
   * @brief Handles Wifi Events
   */
  void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                     void *event_data);

  void initialize_wifi();

  /**
   * @brief Update function to make queued requests (blocking).
   * Should be called from a seperat thread.
   */
  void update();

  /**
   * @brief Triggeres callbacks of the made requests.
   * Should be called from a thread that is in sync with the callbacks target.
   */
  void trigger_response_callbacks();

  /**
   * @brief Queues a new request.
   * Can be called from any thread.
   *
   * @param request The to shedule request.
   */
  void add_request_to_queue(network::Request *request);
};

} // namespace network

#endif