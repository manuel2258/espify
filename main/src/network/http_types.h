
#ifndef REQUEST_H
#define REQUEST_H

#include "cJSON.h"
#include <memory>
#include <string>
#include <vector>

#include <functional>

namespace network {

class Response;

/**
 * @brief A simple key-pair value that can be formated
 */
class KeyValuePair {
private:
  const std::string name;
  const std::string value;

public:
  KeyValuePair(std::string &&n_name, std::string &&n_value)
      : name(std::move(n_name)), value(std::move(n_value)) {}

  KeyValuePair(std::string &&n_name, std::string &n_value)
      : name(std::move(n_name)), value(n_value) {}

  KeyValuePair(std::string &n_name, std::string &n_value)
      : name(n_name), value(n_value) {}

  KeyValuePair(KeyValuePair &pair) : name(pair.name), value(pair.value) {}

  KeyValuePair(KeyValuePair &&pair)
      : name(std::move(pair.name)), value(std::move(pair.value)) {}

  /**
   * @brief Formats to key=value
   */
  std::string to_equal_sign();

  /**
   * @brief Formats to key: value
   */
  std::string to_double_point();
};

/**
 * @brief Object to build a http-requests from key-pair-values
 */
class Request {
private:
  std::string const host;
  std::string const end_point;
  std::string const method;

  bool const read_result;

  std::string body;
  std::string header;
  std::string query;

  std::vector<KeyValuePair> *query_data;
  std::vector<KeyValuePair> *header_data;
  std::vector<KeyValuePair> *body_data;

  static void build_string_from_key_value_pairs(std::vector<KeyValuePair> *data,
                                                std::string *result_str);

public:
  const std::function<void(Response *)> callback;
  Request(std::string const &&host, std::string const &&end_point,
          std::string const &&method,
          std::function<void(Response *)> &&callback,
          bool const read_result = true);
  ~Request();

  void add_query_data(KeyValuePair &&key_val_pair);
  void add_header_data(KeyValuePair &&key_val_pair);
  void add_body_data(KeyValuePair &&key_val_pair);

  /*
   * Needs to be called after adding all data
   */
  void build();

  bool is_read_result() const;
  const std::string *get_host() const;
  std::unique_ptr<std::string> get_full_url(bool tls) const;
  std::unique_ptr<std::string> get_to_send_data() const;
};

/**
 * @brief Response to a Request
 */
class Response {
private:
  bool success = false;
  std::string header;
  std::string body;
  std::function<void(Response *)> callback;

public:
  Response(std::function<void(Response *)> callback);
  ~Response();

  void execute_callback();

  /**
   * @brief Adds the raw response data
   * @param &data: The to add response string
   */
  void add_response_data(std::string &data);

  /**
   * @brief Set the success of request to true
   */
  void set_success();

  std::unique_ptr<cJSON, std::function<void(cJSON *)>> get_body_as_json();
  const std::string *get_body_raw() const;
  const std::string *get_header_raw() const;
  bool was_success() const;
};

} // namespace network

#endif