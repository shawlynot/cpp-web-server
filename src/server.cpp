#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sstream>

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

  struct sockaddr_in client_addr{};
  int client_addr_len = sizeof(client_addr);

  std::cout << "Waiting for a client to connect...\n";

  int socket_descriptor = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
  std::cout << "Client connected\n";

  std::cout << "Reading from socket" << std::endl;

  const int len{1024};
  char input_buffer[len];
  long bytes_received = recv(socket_descriptor, input_buffer, len, 0);
  std::cout << bytes_received << std::endl;

  std::string request_str(input_buffer, bytes_received);
  std::string request_line = request_str.substr(0, request_str.find("\r\n"));

  std::istringstream stream{request_line};

  std::string path;
  stream >> path >> path;

  std::cout << "Path: " << path << "\n";

  std::string path_start = "/echo/";
  std::size_t found = path.find(path_start);

  std::string response;
  if (found != std::string::npos && path.length() > path_start.length()) {
    std::string to_echo { path.substr(path_start.length(), path.length() - path_start.length()) };
    std::ostringstream response_stream;
    response_stream << "HTTP/1.1 200 OK\r\n Content-Type: text/plain\r\n Content-Length: " << to_echo.length() <<  "\r\n\r\n" << to_echo;
    response = response_stream.str();
  } else {
    response = "HTTP/1.1 404 Not Found\r\n\r\n";
  }

  unsigned long bytes = response.length();
  std::cout << "Sending response" << std::endl;
  send(socket_descriptor, response.data(), bytes, 0);

  close(server_fd);

  return 0;
}
