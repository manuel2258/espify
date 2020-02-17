
#include "http_types.h"

namespace network {

Request::Request(std::string const &&host, std::string const &&end_point,
                 std::string const &&method,
                 std::function<void(Response *)> &&callback,
                 bool const read_result)
    : host(std::move(host)), end_point(std::move(end_point)),
      method(std::move(method)), read_result(std::move(read_result)),
      query_data(new std::vector<network::KeyValuePair>()),
      header_data(new std::vector<network::KeyValuePair>()),
      body_data(new std::vector<network::KeyValuePair>()),
      callback(std::move(callback)) {}

Request::~Request() {
  delete query_data;
  delete header_data;
  delete body_data;
}

void Request::add_body_data(KeyValuePair &&key_val_pair) {
  if (body_data == nullptr) {
    body_data = new std::vector<KeyValuePair>;
  }
  body_data->push_back(std::move(key_val_pair));
}

void Request::add_header_data(KeyValuePair &&key_val_pair) {
  if (header_data == nullptr) {
    header_data = new std::vector<KeyValuePair>;
  }
  header_data->push_back(std::move(key_val_pair));
}

void Request::add_query_data(KeyValuePair &&key_val_pair) {
  if (query_data == nullptr) {
    query_data = new std::vector<KeyValuePair>;
  }
  query_data->push_back(std::move(key_val_pair));
}

void Request::build() {
  if (!query_data->empty()) {
    query += "?";
    build_string_from_key_value_pairs(query_data, &query);
  }

  build_string_from_key_value_pairs(body_data, &body);

  header += method + " " + end_point + query + " HTTP/1.1\r\n";
  header += "Host: " + host + "\r\n";
  header += "Accept-Language: en-us\r\n";
  header += "Connection: close\r\n";
  header += "Accept: */*\r\n";
  if (header_data != nullptr) {
    for (KeyValuePair request_data : *header_data) {
      header += request_data.to_equal_sign();
    }
  }
  header += "Content-length: " + std::to_string(body.length()) + "\r\n";

  header += "\r\n";
}

void Request::build_string_from_key_value_pairs(std::vector<KeyValuePair> *data,
                                                std::string *result_str) {
  bool first = true;
  for (KeyValuePair request_data : *data) {
    if (first) {
      first = false;
    } else {
      (*result_str) += "&";
    }
    (*result_str) += request_data.to_equal_sign();
  }
}

bool Request::is_read_result() const { return read_result; }

const std::string *Request::get_host() const { return &host; }

std::unique_ptr<std::string> Request::get_full_url(bool tls) const {
  return std::make_unique<std::string>((tls ? "https://" : "http://") + host +
                                       end_point);
}

std::unique_ptr<std::string> Request::get_to_send_data() const {
  return std::make_unique<std::string>(header + body);
}

std::string KeyValuePair::to_equal_sign() { return name + "=" + value; }

std::string KeyValuePair::to_double_point() { return name + ": " + value; }

const std::string *Response::get_body_raw() const { return &body; }

const std::string *Response::get_header_raw() const { return &header; }

bool Response::was_success() const { return success; }

std::unique_ptr<cJSON, std::function<void(cJSON *)>>
Response::get_body_as_json() {
  return std::unique_ptr<cJSON, std::function<void(cJSON *)>>(
      cJSON_Parse(body.c_str()), [](cJSON *json) { cJSON_Delete(json); });
}

void Response::add_response_data(std::string &data) {
  int sep = data.find("\r\n\r\n");
  header = data.substr(0, sep);
  body = data.substr(sep + 4, data.length() - 1);
  success = true;
}

void Response::execute_callback() { callback(this); }

Response::Response(std::function<void(Response *)> callback)
    : callback(callback) {}

Response::~Response() {}

} // namespace network