#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sstream>
#include <array>
#include <vector>
#include <algorithm>
#include <optional>

std::vector<std::string> split_string(const std::string &request_str, const std::string &separator) {
  std::vector<std::string> out;
  unsigned long start_pos = 0;
  unsigned long end_pos;
  for (; start_pos < request_str.length(); start_pos = end_pos + separator.length()) {
    end_pos = request_str.find(separator, start_pos);
    if (end_pos == std::string::npos) {
      end_pos = request_str.length();
    }
    out.push_back(request_str.substr(start_pos, end_pos - start_pos));
  }
  return out;
}

std::optional<std::string> find_header_value(const std::string &header_name, const std::string &request_str) {
  std::vector<std::string> request_lines = split_string(request_str, "\r\n");
  auto found = std::find_if(
      request_lines.begin(),
      request_lines.end(),
      [&header_name](auto line) { return line.find(header_name) != std::string::npos; }
  );
  if (found == request_lines.end()) {
    return std::nullopt;
  }
  return found->substr(header_name.length());
}


int main(int argc, char **argv) {
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  std::cout << "Logs from your program will appear here!\n";

  // Uncomment this block to pass the first stage

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    std::cerr << "Failed to create server socket\n";
    return 1;
  }

  // Since the tester restarts your program quite often, setting REUSE_PORT
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);

  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    std::cerr << "Failed to bind to port 4221\n";
    return 1;
  }

  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    std::cerr << "listen failed\n";
    return 1;
  }

  struct sockaddr_in client_addr {};
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for a client to connect...\n";

  int socket_descriptor = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  std::cout << "Client connected\n";

  std::cout << "Reading from socket" << std::endl;

  const int len { 1024 };
  char input_buffer[len];
  long bytes_received = recv(socket_descriptor, input_buffer, len, 0);
  std::cout << bytes_received << std::endl;

  std::string request_str(input_buffer, bytes_received);
  std::string request_line = request_str.substr(0, request_str.find("\r\n"));

  std::istringstream stream { request_line };

  std::string path;
  stream >> path >> path;

  std::cout << "Path: " << path << "\n";

  std::string echo_path_start = "/echo/";
  std::string user_agent_path = "/user-agent";

  std::string response;
  if (path.find(echo_path_start) != std::string::npos && path.length() > echo_path_start.length()) {
    std::string to_echo { path.substr(echo_path_start.length(), path.length() - echo_path_start.length()) };
    std::ostringstream response_stream;
    response_stream << "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " << to_echo.length()
                    << "\r\n\r\n" << to_echo;
    response = response_stream.str();
  } else if (path == user_agent_path) {
    const auto user_agent_header = find_header_value("User-Agent:", request_str);
    std::ostringstream response_stream;
    response_stream << "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " << user_agent_header->length()
                    << "\r\n\r\n" << *user_agent_header;
    response = response_stream.str();
  } else if (path == "/") {
    response = "HTTP/1.1 200 OK\r\n\r\n";
  } else {
    response = "HTTP/1.1 404 Not Found\r\n\r\n";
  }

  unsigned long bytes = response.length();
  std::cout << "Sending response" << std::endl;
  send(socket_descriptor, response.data(), bytes, 0);

  close(server_fd);

  return 0;
}