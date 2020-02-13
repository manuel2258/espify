
#include "HttpsRequester.h"

extern network::HttpsRequester *https_requester;

namespace network {

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  https_requester->event_handler(arg, event_base, event_id, event_data);
}

static int s_retry_num = 0;

void HttpsRequester::event_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT &&
             event_id == WIFI_EVENT_STA_DISCONNECTED) {
    connected = false;
    if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(LOG_TAG, "retry to connect to the AP");
    } else {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    ESP_LOGI(LOG_TAG, "connect to the AP fail");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(LOG_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    connected = true;
  }
}

HttpsRequester::HttpsRequester() {
  xSemaphoreGive(response_lock);
  xSemaphoreGive(request_lock);

  s_wifi_event_group = xEventGroupCreate();

  ESP_ERROR_CHECK(esp_netif_init());

  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &wifi_event_handler, NULL));

  wifi_scan_threshold_t sta_threshhold = {0, wifi_auth_mode_t::WIFI_AUTH_OPEN};
  wifi_pmf_config_t sta_pmf = {true, false};

  wifi_sta_config_t sta_config = {EXAMPLE_ESP_WIFI_SSID,
                                  EXAMPLE_ESP_WIFI_PASS,
                                  wifi_scan_method_t::WIFI_ALL_CHANNEL_SCAN,
                                  0,
                                  {0},
                                  0,
                                  0,
                                  wifi_sort_method_t::WIFI_CONNECT_AP_BY_SIGNAL,
                                  sta_threshhold,
                                  sta_pmf};

  wifi_config_t wifi_config;
  wifi_config.sta = sta_config;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(LOG_TAG, "wifi_init_sta finished.");

  tls_cfg.cacert_buf = server_root_cert_pem_start;
  tls_cfg.cacert_bytes = server_root_cert_pem_end - server_root_cert_pem_start;
}

HttpsRequester::~HttpsRequester() {
  ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                               &wifi_event_handler));
  ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                               &wifi_event_handler));
  vEventGroupDelete(s_wifi_event_group);

  ESP_ERROR_CHECK(esp_wifi_disconnect());
}

Response *HttpsRequester::make_request(Request *request_data) {
  Response *response_data = new Response(request_data->callback);

  request_data->build();

  auto host = *request_data->get_host();
  host = "https://" + host;

  printf(host.c_str());

  struct esp_tls *tls = esp_tls_conn_http_new(host.c_str(), &tls_cfg);

  if (tls != NULL) {
    ESP_LOGI(LOG_TAG, "Connection established...");
  } else {
    ESP_LOGE(LOG_TAG, "Connection failed...");
    return response_data;
  }

  auto send_data = request_data->get_to_send_data().c_str();

  ESP_LOGI(LOG_TAG, "Sending data ... ");

  size_t written_bytes = 0;
  do {
    int ret = esp_tls_conn_write(tls, send_data + written_bytes,
                                 strlen(send_data) - written_bytes);
    if (ret >= 0) {
      ESP_LOGI(LOG_TAG, "%d bytes written", ret);
      written_bytes += ret;
    } else if (ret != ESP_TLS_ERR_SSL_WANT_READ &&
               ret != ESP_TLS_ERR_SSL_WANT_WRITE) {
      ESP_LOGE(LOG_TAG, "esp_tls_conn_write  returned 0x%x", ret);
      return response_data;
    }
  } while (written_bytes < strlen(send_data));

  ESP_LOGI(LOG_TAG, "Reading HTTP response...");

  bool read_result = request_data->is_read_result();
  delete request_data;

  if (read_result) {
    const int buffer_size = 64;
    std::string data;
    while (true) {
      char *buffer = new char[buffer_size];
      int recieved = esp_tls_conn_read(tls, buffer, buffer_size);

      if (recieved < 0 && !(recieved == ESP_TLS_ERR_SSL_WANT_WRITE ||
                            recieved == ESP_TLS_ERR_SSL_WANT_READ)) {
        ESP_LOGE(LOG_TAG, "esp_tls_conn_read  returned -0x%x", -recieved);
        return response_data;
      }

      if (recieved == 0) {
        ESP_LOGI(LOG_TAG, "connection closed");
        break;
      }

      data += std::string(buffer, recieved);
      delete buffer;
    }
    response_data->add_response_data(data);
  } else {
    ESP_LOGI(LOG_TAG, "Skipping to read result");
  }
  return response_data;
}

void HttpsRequester::add_request_to_queue(Request *request_data) {
  xSemaphoreTake(response_lock, portMAX_DELAY);
  request_queue.push(request_data);
  ESP_LOGI(LOG_TAG, "Added new request to queue");
  xSemaphoreGive(response_lock);
}

void HttpsRequester::update() {
  if (!connected)
    return;

  xSemaphoreTake(request_lock, portMAX_DELAY);
  if (request_queue.empty()) {
    xSemaphoreGive(request_lock);
    return;
  }
  auto request_data = request_queue.front();
  request_queue.pop();
  xSemaphoreGive(request_lock);

  auto response = make_request(request_data);

  xSemaphoreTake(response_lock, portMAX_DELAY);
  response_queue.push(response);
  xSemaphoreGive(response_lock);
}

void HttpsRequester::trigger_response_callbacks() {
  xSemaphoreTake(response_lock, portMAX_DELAY);
  while (!response_queue.empty()) {
    Response *response = response_queue.front();
    response_queue.pop();
    response->execute_callback();
    ESP_LOGI(LOG_TAG, "Triggered new callback");
  }
  xSemaphoreGive(response_lock);
}

} // namespace network
