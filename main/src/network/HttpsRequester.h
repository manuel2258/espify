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

#include "HttpData.h"
#include <queue>

/* The examples use WiFi configuration that you can set via projectb
   configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY CONFIG_ESP_MAXIMUM_RETRY

/* FreeRTOS event group to signal when we are connected*/

/* The event group allows multiple bits for each event, but we only care about
 * two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

extern const uint8_t
    server_root_cert_pem_start[] asm("_binary_server_root_cert_pem_start");
extern const uint8_t
    server_root_cert_pem_end[] asm("_binary_server_root_cert_pem_end");

namespace network {

class HttpsRequester {
private:
  EventGroupHandle_t s_wifi_event_group;
  const char *LOG_TAG = "wifi station";
  bool connected = false;

  xSemaphoreHandle request_lock = xSemaphoreCreateBinary();
  std::queue<Request *> request_queue;

  xSemaphoreHandle response_lock = xSemaphoreCreateBinary();
  std::queue<Response *> response_queue;

  esp_tls_cfg_t tls_cfg;

  Response *make_request(Request *request);

public:
  HttpsRequester();
  ~HttpsRequester();

  void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id,
                     void *event_data);

  void update();

  void trigger_response_callbacks();
  void add_request_to_queue(network::Request *request);
};

} // namespace network

#endif