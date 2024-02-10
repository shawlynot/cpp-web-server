//
// Created by shawlynot on 2/4/24.
//

#pragma once

#include <string>
#include <vector>
#include <map>

namespace shawlynot {

  class http_request {
  private:
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    int socket;
    bool end_of_stream;
    const static long body_buffer_chunk_size;
    http_request(std::string method, std::string path, const std::map<std::string, std::string> &headers,
                 int socket);

  public:

    static http_request receive_from_socket(int socket);

    bool is_end_of_stream() const;

    const std::string &get_method() const;

    const std::map<std::string, std::string> &get_headers() const;

    const std::string &get_path() const;

    std::vector<char> get_body();
  };

} // shawlynot