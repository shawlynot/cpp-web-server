#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sstream>
#include <array>
#include <vector>
#include <algorithm>
#include <optional>
#include <thread>
#include <fstream>

void send_text_response(const int socket_descriptor, const std::string &response) {
  unsigned long bytes = response.length();
  std::cout << "Sending response\n";
  send(socket_descriptor, response.data(), bytes, 0);
}

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
  return found->substr(header_name.length() + 2);
}

bool has_path_and_param(const std::string &path, const std::string &path_start) {
  return path.rfind(path_start, 0) != std::string::npos && path.length() > path_start.length();
}

void handle_connection(const int socket_descriptor, const std::string &directory) {
  std::cout << "Client: " << socket_descriptor << " connected\n";

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

  std::string echo_path { "/echo/" };
  std::string user_agent_path { "/user-agent" };
  std::string files_path { "/files/" };

  if (has_path_and_param(path, echo_path)) {
    std::string to_echo { path.substr(echo_path.length(), path.length() - echo_path.length()) };
    std::ostringstream response_stream;
    response_stream << "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " << to_echo.length()
                    << "\r\n\r\n" << to_echo;
    send_text_response(socket_descriptor, response_stream.str());
  } else if (has_path_and_param(path, files_path)) {
    std::string file_from_path { path.substr(files_path.length(), path.length() - files_path.length()) };
    const std::string file_to_open = directory + "/" + file_from_path;
    std::cout << "Opening " << file_to_open << "\n";
    std::ifstream file { file_to_open, std::ios::binary };
    if (!file.is_open()) {
      send_text_response(socket_descriptor, "HTTP/1.1 404 Not Found\r\n\r\n");
    } else {
      file.seekg(0, std::ios::end);
      long content_length { file.tellg() };

      std::ostringstream response_stream;
      response_stream << "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " << content_length
                      << "\r\n" "Content-Type: application/octet-stream" "\r\n\r\n";
      auto headers = response_stream.str();
      send(socket_descriptor, headers.data(), headers.length(), 0);

      file.seekg(0, std::ios::beg);
      for (char from_file; file.get(from_file);) {
        send(socket_descriptor, &from_file, 1, 0);
      }
      file.close();
    }
  } else if (path == user_agent_path) {
    const auto user_agent_header = find_header_value("User-Agent", request_str);
    std::ostringstream response_stream;
    response_stream << "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " << user_agent_header->length()
                    << "\r\n\r\n" << *user_agent_header;
    send_text_response(socket_descriptor, response_stream.str());
  } else if (path == "/") {
    send_text_response(socket_descriptor, "HTTP/1.1 200 OK\r\n\r\n");
  } else {
    send_text_response(socket_descriptor, "HTTP/1.1 404 Not Found\r\n\r\n");
  }

  close(socket_descriptor);
  std::cout << "Connection closed\n";
}


int main(int argc, char **argv) {

  std::string directory { "./" };
  if (argc == 3) {
    directory = argv[2];
  }

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

  while (true) {
    int socket_descriptor = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    std::cout << "Client: " << socket_descriptor << " connected\n";
    std::thread connection_handler(handle_connection, socket_descriptor, directory);
    connection_handler.detach();
  }
}