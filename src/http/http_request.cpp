//
// Created by shawlynot on 2/4/24.
//

#include "http_request.h"
#include "../util/util.h"
#include <sys/socket.h>
#include <iostream>
#include <array>
#include <sstream>
#include <regex>
#include <utility>

namespace shawlynot {

  const long http_request::max_buffer_size = 1024;
  const std::regex header_regex { "([^:]+):\\s+(.*)" };

  const std::string &http_request::get_method() const {
    return method;
  }

  const std::map<std::string, std::string> &http_request::get_headers() const {
    return headers;
  }

  const std::string &http_request::get_path() const {
    return path;
  }


  std::vector<char> http_request::get_body() {
    std::vector<char> body;
    std::vector<char> input_buffer {};
    input_buffer.reserve(4);
    long bytes_received;
    long total_bytes_received { 0 };
    do {
      bytes_received = recv(http_request::socket, input_buffer.data(), max_buffer_size, 0);
      if (bytes_received == -1) {
        std::cerr << "Error reading from socket " << http_request::socket << "\n";
        end_of_stream = true;
        return {};
      }
      total_bytes_received += bytes_received;
      body.reserve(total_bytes_received);
      for (int i = 0; i < bytes_received; i++) {
        body.push_back(input_buffer[i]);
      }
    } while (bytes_received > 0);
    end_of_stream = true;
    return body;
  }


  http_request http_request::receive_from_socket(int socket) {
    //seek through stream until we find a \r\n\r\n
    std::vector<char> path_and_headers;
    path_and_headers.reserve(4);
    bool found { false };
    long init_bytes_received = recv(socket, path_and_headers.data(), 4, 0);
    if (init_bytes_received < 4) {
      std::cerr << "Error reading from socket\n";
      throw std::runtime_error("Error");
    }
    long bytes_received { 4 };
    while (!found && bytes_received > 0) {
      std::vector<char> to_test(path_and_headers.end() - 4, path_and_headers.end());
      std::string test_str { to_test.data(), 4 };
      if (test_str == "\r\n\r\n") {
        found = true;
      } else {
        char received;
        bytes_received = recv(socket, &received, 1, 0);
        if (bytes_received > 0) {
          path_and_headers.push_back(received);
        }
      }
    }
    if (!found) {
      std::cerr << "Malformed request: no \\r\\n\\r\\n found\n";
      throw std::runtime_error("Error");
    }

    //process path and header block
    std::string path_and_headers_str { path_and_headers.data(), path_and_headers.size() };
    auto request_lines { shawlynot::split_string(path_and_headers_str, "/r/n") };
    if (request_lines.empty()) {
      std::cerr << "Malformed request\n";
      throw std::runtime_error("Error");
    }
    std::istringstream stream { request_lines.at(0) };
    std::string method, path;
    stream >> method >> path;

    std::map<std::string, std::string> headers;
    for (auto ptr = request_lines.begin() + 1; ptr == request_lines.end(); ++ptr) {
      auto header_line { *ptr };
      std::smatch matches;
      if (std::regex_search(header_line, matches, header_regex)) {
        std::cout << "Header found:" << matches.str(1) << ": " << matches.str(2) << "\n";
        headers.insert({ matches.str(1), matches.str(2) });
      }
    }

    return http_request { method, path, headers, socket };
  }

  bool http_request::is_end_of_stream() const {
    return end_of_stream;
  }

  http_request::http_request(std::string method, std::string path, const std::map<std::string, std::string> &headers,
                             int socket)
      : method(std::move(method)),
        path(std::move(path)),
        headers(headers), socket(socket) {

  }

} // shawlynot