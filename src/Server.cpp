/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zatais <zatais@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 21:51:55 by zatais            #+#    #+#             */
/*   Updated: 2026/05/17 22:25:32 by zatais           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include "../include/WebServ.hpp"
#include <cstdlib>

Server::Server(){}

Server::Server(Config& conf) : _config(conf){}

Server::~Server(){}

/******************************************************************************/

void Server::setNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    _logger.error("fcntl set non-blocking failed");
}

void Server::addToPoll(int fd, short events) {
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = events;
  pfd.revents = 0;
  _poll_fds.push_back(pfd);
}

void Server::closeClient(int fd) {
  std::map<int, Client*>::iterator it = _clients.find(fd);
  if (it != _clients.end()) {
    _logger.logConnection(it->second->ip, it->second->port, false);
    close(fd);
    delete it->second;
    _clients.erase(it);
  }

  for (size_t i = 0; i < _poll_fds.size(); ++i) {
    if (_poll_fds[i].fd == fd) {
      _poll_fds.erase(_poll_fds.begin() + i);
      break;
    }
  }
}

void Server::checkTimeouts() {
  time_t now = time(NULL);
  std::vector<int> to_close;

  for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
    if (difftime(now, it->second->last_activity) > CLIENT_TIMEOUT) {
      std::ostringstream oss;
      oss << "Client timeout: " << it->second->ip << ":" << it->second->port;
      _logger.warn(oss.str());
      to_close.push_back(it->first);
    }
  }

  for (size_t i = 0; i < to_close.size(); ++i)
    closeClient(to_close[i]);
}

void Server::acceptConnection(int listen_fd) {
  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
  if (client_fd < 0)
    _logger.error("accept failed");

  setNonBlocking(client_fd);

  char client_ip[16];
  inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, 16);
  int client_port = ntohs(client_addr.sin_port);

  Client* client = initClient(client_fd, listen_fd, client_ip, client_port);
  _clients[client_fd] = client;
  addToPoll(client_fd, POLLIN);

  _logger.logConnection(client_ip, client_port, true);
}

void Server::handleClientRead(int client_fd) {
  std::map<int, Client*>::iterator it = _clients.find(client_fd);
  if (it == _clients.end())
    return;

  Client* client = it->second;

  while (true) {
    char buffer[4096];
    ssize_t bytes = recv(client_fd, buffer, sizeof(buffer), 0);

    if (bytes > 0) {
      client->read_buffer.append(buffer, bytes);
      client->last_activity = time(NULL);
    }
    else if (bytes == 0) {
      closeClient(client_fd);
      return;
    }
    else {
      if (errno == EAGAIN)
        break;
      closeClient(client_fd);
      return;
    }
  }

  while (true) {
    size_t request_size = getRequestSize(client->read_buffer);
    if (request_size == std::string::npos)
      break;

    std::string request_data = client->read_buffer.substr(0, request_size);
    client->read_buffer.erase(0, request_size);

    // parseing start here(request_data) !!
  }
}

size_t getRequestSize(std::string& buffer) {
  size_t header_end = buffer.find("\r\n\r\n");
  if (header_end == std::string::npos)
    return std::string::npos;

  size_t body_start = header_end + 4;
  size_t content_length = 0;
  bool has_cl = false;

  size_t cl_pos = buffer.find("Content-Length:");
  if (cl_pos != std::string::npos && cl_pos < header_end) {
    size_t val_start = cl_pos + 15;
    while (val_start < header_end && buffer[val_start] == ' ')
      ++val_start;
    size_t line_end = buffer.find("\r\n", val_start);
    if (line_end == std::string::npos)
      return std::string::npos;
    std::string value = buffer.substr(val_start, line_end - val_start);

    char* endptr = NULL;
    long num = strtol(value.c_str(), &endptr, 10);

    if (endptr == value.c_str() || *endptr != '\0' || num < 0)
      num = 0;
    content_length = static_cast<size_t>(num);
    has_cl = true;
  }
  if (!has_cl)
    return body_start;

  size_t total = body_start + content_length;
  return (buffer.length() >= total) ? total : std::string::npos;
}

Client* Server::initClient(int client_fd, int listen_fd, const std::string& client_ip, int client_port) {
  Client* client = new Client();
  client->fd = client_fd;
  client->ip = client_ip;
  client->port = client_port;
  client->read_buffer = "";
  client->write_buffer = "";
  client->state = 0;
  client->keep_alive = true;
  client->server_idx = _fd_to_server_idx[listen_fd];
  client->last_activity = time(NULL);
  client->cgi_pid = -1;
  client->cgi_stdin_fd = -1;
  client->cgi_stdout_fd = -1;
  client->cgi_input_buffer = "";
  client->cgi_output_buffer = "";
  client->request_obj = NULL;
  client->response_obj = NULL;
  return client;
}

void Server::createSockets() {
  std::vector<ServerConfig> servers = _config.getServers();

  for (size_t srv_idx = 0; srv_idx < servers.size(); ++srv_idx) {
    ServerConfig& server = servers[srv_idx];
    for (size_t i = 0; i < server.listens.size(); ++i) {
      ListenConfig& lc = server.listens[i];

      int sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (sockfd < 0)
        _logger.error("socket creation failed");

      int opt = 1;
      if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(sockfd);
        _logger.error("setsockopt SO_REUSEADDR failed");
      }

      struct sockaddr_in addr;
      std::memset(&addr, 0, sizeof(addr));
      addr.sin_family = AF_INET;
      addr.sin_port = htons(lc.port);

      if (inet_pton(AF_INET, lc.host.c_str(), &addr.sin_addr) <= 0) {
        close(sockfd);
        _logger.error("invalid IP address: " + lc.host);
      }

      if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sockfd);
        std::ostringstream oss;
        oss << lc.port;
        _logger.error("bind failed on " + lc.host + ":" + oss.str());
      }

      if (listen(sockfd, BACKLOG) < 0) {
        close(sockfd);
        _logger.error("listen failed");
      }

      setNonBlocking(sockfd);

      std::ostringstream listenMsgInfo;
      listenMsgInfo << lc.host << ":" << lc.port;
      _logger.info("Listening on " + listenMsgInfo.str());

      _listen_fds.push_back(sockfd);
      _fd_to_server_idx[sockfd] = static_cast<int>(srv_idx);
    }
  }
}

void Server::run() {
  createSockets();

  std::ostringstream oss;
  oss << "Server started, listening on " << _listen_fds.size() << " port(s)";
  _logger.info(oss.str());

  for (size_t i = 0; i < _listen_fds.size(); ++i)
    addToPoll(_listen_fds[i], POLLIN);

  while (true) {
    int ready = poll(&_poll_fds[0], _poll_fds.size(), POLL_TIMEOUT);
    if (ready < 0)
      _logger.error("poll failed");

    if (!_clients.empty())
      checkTimeouts();

    for (size_t i = 0; i < _poll_fds.size(); ++i) {
      if (_poll_fds[i].revents & POLLIN) {
        int fd = _poll_fds[i].fd;
        if (_fd_to_server_idx.find(fd) != _fd_to_server_idx.end())
          acceptConnection(fd);
        else
          handleClientRead(fd);
      }
    }
  }
}
