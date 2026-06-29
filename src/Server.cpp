/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 21:51:55 by zatais            #+#    #+#             */
/*   Updated: 2026/06/27 04:28:07 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/WebServ.hpp"

Server::Server(){}

Server::Server(Config& conf) : _config(conf), _cgi(new Cgi(_poll_fds, _pipe_to_client_fd, _logger)) {}

Server::~Server() {
  for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
    if (it->second->state == STATE_CGI_RUNNING && it->second->cgi_pid != -1)
      _cgi->killCgi(it->second, SIGKILL);
    close(it->second->fd);
    delete it->second;
  }

  for (size_t i = 0; i < _listen_fds.size(); ++i)
    close(_listen_fds[i]);

  delete _cgi;
}

/******************************************************************************/

void Server::setNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::addToPoll(int fd, short events, std::vector<struct pollfd>& poll_fds) {
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = events;
  pfd.revents = 0;
  poll_fds.push_back(pfd);
}

void Server::closeClient(int fd) {
  std::map<int, Client*>::iterator it = _clients.find(fd);
  if (it != _clients.end()) {
    if (it->second->state == STATE_CGI_RUNNING && it->second->cgi_pid != -1)
      _cgi->killCgi(it->second, SIGKILL);
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
    if (it->second->state == STATE_CGI_RUNNING && it->second->cgi_pid != -1) {
      if (difftime(now, it->second->cgi_start_time) > CGI_TIMEOUT) {
        std::ostringstream oss;
        oss << "CGI timeout: " << it->second->ip << ":" << it->second->port;
        _logger.warn(oss.str());
        _cgi->killCgi(it->second, SIGKILL);
        std::string body = ServeStaticRq::html_Error_page(504, "Gateway Timeout");
        std::ostringstream res;
        res << "HTTP/1.1 504 Gateway Timeout\r\n"<< "Content-Length: "<< body.size() << "\r\n\r\n" << body; /// still needs the date to be added
        it->second->write_buffer = res.str(); 
                                    
        it->second->state = STATE_SENDING;
        for (size_t j = 0; j < _poll_fds.size(); ++j) {
          if (_poll_fds[j].fd == it->first) {
            _poll_fds[j].events |= POLLOUT;
            break;
          }
        }
      }
    }
    else if (difftime(now, it->second->last_activity) > CLIENT_TIMEOUT) {
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
  if (client_fd < 0) {
    _logger.warn("accept failed");
    return;
  }

  setNonBlocking(client_fd);

  char client_ip[16];
  inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, 16);
  int client_port = ntohs(client_addr.sin_port);

  Client* client = initClient(client_fd, listen_fd, client_ip, client_port);
  _clients[client_fd] = client;
  addToPoll(client_fd, POLLIN, _poll_fds);

  _logger.logConnection(client_ip, client_port, true);
}

void Server::handleClientRead(int client_fd) {
  std::map<int, Client*>::iterator it = _clients.find(client_fd);
  if (it == _clients.end())
    return;

  Client* client = it->second;
  if (client->state != STATE_READING)
    return;

  while (true) {
    char buffer[4096];
    ssize_t bytes = recv(client_fd, buffer, sizeof(buffer), 0);

    if (bytes > 0) {
      client->read_buffer.append(buffer, bytes);
      client->last_activity = time(NULL);
    }
    else if (!bytes) {
      closeClient(client_fd);
      return;
    }
    else
      break;
  }

  size_t request_size = getRequestSize(client->read_buffer);
  if (request_size != std::string::npos) {
    std::string request_data = client->read_buffer.substr(0, request_size);
    client->read_buffer.erase(0, request_size);

    //TODO test

    Request request(client ,request_data);
    // client->request_obj = &request;
    ProcessRequest ProcessRq(request, _config.getServers()[client->server_idx]);
    if (!ProcessRq.is_CgiRq)
    {
      ServeStaticRq StaticRq(request, ProcessRq, _config.getServers()[client->server_idx]);
      Response StaticResponse (ProcessRq, StaticRq, request);
      // client->response_obj = &StaticResponse;
      client->write_buffer = StaticResponse.getHttpResponse();
      _logger.logRequest(client->ip, request.getRequestLine().Method, request.getRequestLine().URI, request.getRequestLine().HttpVers, ProcessRq.getStatusCode(), sizeof(client->write_buffer));
      client->state = STATE_SENDING;
       for (size_t i = 0; i < _poll_fds.size(); ++i) {
            if (_poll_fds[i].fd == client_fd) {
                _poll_fds[i].events |= POLLOUT;
                break;
            }
        }
    }
    else 
    {
      ProcessCgi ProcessCGI(client, ProcessRq, request);
      // _cgi->startCgi(client, interpreter, ProcessCGI.getCgiPath(), ProcessCGI.getEnv());
    }
      //interpreter will be define by Zakaria

      
    // parse request_data -> sets client->request_obj, req->isCgi

    // if (!req->isCgi) {
    //     generate response
    //     client->state = STATE_SENDING;
    //     for (size_t i = 0; i < _poll_fds.size(); ++i) {
    //         if (_poll_fds[i].fd == client_fd) {
    //             _poll_fds[i].events |= POLLOUT;
    //             break;
    //         }
    //     }
    // }
    // else
    //     _cgi->startCgi(client, interpreter, script_path, req->envp);
  }
}

bool Server::iequal(std::string& a, const std::string& b) {
  if (a.size() != b.size())
    return false;
  for (size_t i = 0; i < a.size(); ++i)
    if (tolower(a[i]) != tolower(b[i]))
      return false;
  return true;
}

size_t Server::findHeaderValue(std::string& buffer, const std::string& name, size_t headerEnd) {
  size_t pos = 0;
  while (pos < headerEnd) {
    size_t lineEnd = buffer.find("\r\n", pos);
    if (lineEnd == std::string::npos)
      break;
    size_t colon = buffer.find(':', pos);
    if (colon != std::string::npos && colon < lineEnd) {
      std::string hdrName(buffer, pos, colon - pos);
      if (iequal(hdrName, name)) {
        size_t val = colon + 1;
        while (val < lineEnd && isspace(buffer[val]))
          ++val;
        return val;
      }
    }
    pos = lineEnd + 2;
  }
  return std::string::npos;
}

size_t Server::parseChunkedBody(std::string& buffer, size_t bodyStart) {
  size_t bodyEnd = buffer.find("0\r\n\r\n", bodyStart);
  if (bodyEnd == std::string::npos)
    return std::string::npos;
  return bodyEnd - bodyStart + 5;
}

size_t Server::getRequestSize(std::string& buffer) {
  size_t header_end = buffer.find("\r\n\r\n");
  if (header_end == std::string::npos)
    return std::string::npos;

  size_t body_start = header_end + 4;

  size_t te_pos = findHeaderValue(buffer, "Transfer-Encoding", header_end);
  if (te_pos != std::string::npos) {
    size_t lineEnd = buffer.find("\r\n", te_pos);
    if (lineEnd != std::string::npos) {
      std::string value(buffer, te_pos, lineEnd - te_pos);
      size_t last = value.find_last_not_of(" \t");
      if (last != std::string::npos)
        value = value.substr(0, last + 1);
      if (iequal(value, "chunked")) {
          size_t bodySize = parseChunkedBody(buffer, body_start);
          if (bodySize == std::string::npos)
            return std::string::npos;
          size_t total = body_start + bodySize;
          return (buffer.size() >= total) ? total : std::string::npos;
        }
    }
  }

  size_t content_length = 0;
  size_t cl_pos = findHeaderValue(buffer, "Content-Length", header_end);
  if (cl_pos != std::string::npos) {
    size_t lineEnd = buffer.find("\r\n", cl_pos);
    if (lineEnd == std::string::npos)
      return std::string::npos;
    std::string value(buffer, cl_pos, lineEnd - cl_pos);
    size_t lastNonSpace = value.find_last_not_of(" \t");
    if (lastNonSpace != std::string::npos)
      value = value.substr(0, lastNonSpace + 1);
    else
      value.clear();

    char* endptr;
    long num = std::strtol(value.c_str(), &endptr, 10);
    if (endptr == value.c_str() || *endptr != '\0' || num < 0)
      num = 0;
    content_length = static_cast<size_t>(num);
  }

  size_t total = body_start + content_length;
  return (buffer.size() >= total) ? total : std::string::npos;
}

Client* Server::initClient(int client_fd, int listen_fd, const std::string& client_ip, int client_port) {
  Client* client = new Client();
  client->fd = client_fd;
  client->ip = client_ip;
  client->port = client_port;
  client->read_buffer = "";
  client->write_buffer = "";
  client->state = STATE_READING;
  client->keep_alive = true;
  client->server_idx = _fd_to_server_idx[listen_fd];
  client->last_activity = time(NULL);
  client->cgi_pid = -1;
  client->cgi_stdin_fd = -1;
  client->cgi_stdout_fd = -1;
  client->cgi_input_buffer = "";
  client->cgi_output_buffer = "";
  client->cgi_start_time = 0;
  client->request_obj = NULL;
  client->response_obj = NULL;
  return client;
}

void Server::sendResponse(int client_fd) {
  std::map<int, Client*>::iterator it = _clients.find(client_fd);
  if (it == _clients.end())
    return;

  Client* client = it->second;
  if (client->write_buffer.empty())
    return;

  ssize_t bytes = send(client_fd, client->write_buffer.c_str(), client->write_buffer.size(), 0);

  if (bytes > 0) {
    client->write_buffer.erase(0, bytes);
    client->last_activity = time(NULL);
  }
  else if (bytes < 0)
    return;

  if (client->write_buffer.empty()) {
    for (size_t i = 0; i < _poll_fds.size(); ++i) {
      if (_poll_fds[i].fd == client_fd) {
        _poll_fds[i].events = POLLIN;
        break;
      }
    }
    if (client->keep_alive)
      client->state = STATE_READING;
    else
      closeClient(client_fd);
  }
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
    addToPoll(_listen_fds[i], POLLIN, _poll_fds);

  while (true) {
    int ready = poll(&_poll_fds[0], _poll_fds.size(), POLL_TIMEOUT);
    if (ready < 0)
      _logger.error("poll failed");

    if (!_clients.empty())
      checkTimeouts();

    handlePollErrors();
    handlePollIn();
    handleCgiPipeRead();
    handleCgiStdinWrite();
    handlePollOut();
  }
}

void Server::handlePollIn() {
  std::vector<int> ready_fds;
  for (size_t i = 0; i < _poll_fds.size(); ++i)
    if (_poll_fds[i].revents & POLLIN && !_pipe_to_client_fd.count(_poll_fds[i].fd))
      ready_fds.push_back(_poll_fds[i].fd);

  for (size_t i = 0; i < ready_fds.size(); ++i) {
    if (_fd_to_server_idx.find(ready_fds[i]) != _fd_to_server_idx.end())
      acceptConnection(ready_fds[i]);
    else if (_clients.find(ready_fds[i]) != _clients.end())
      handleClientRead(ready_fds[i]);
  }
}

void Server::handleCgiPipeRead() {
  std::vector<int> ready_pipe_fds;
  for (size_t i = 0; i < _poll_fds.size(); ++i)
    if (_poll_fds[i].revents & POLLIN && _pipe_to_client_fd.count(_poll_fds[i].fd))
      ready_pipe_fds.push_back(_poll_fds[i].fd);

  for (size_t i = 0; i < ready_pipe_fds.size(); ++i) {
    int pipe_fd = ready_pipe_fds[i];
    std::map<int, int>::iterator pipe_it = _pipe_to_client_fd.find(pipe_fd);
    if (pipe_it != _pipe_to_client_fd.end()) {
      int client_fd = pipe_it->second;
      _cgi->handleCgiRead(pipe_it, _clients);

      std::map<int, Client*>::iterator client_it = _clients.find(client_fd);
      if (client_it != _clients.end() &&
          (client_it->second->state == STATE_WRITING_RESPONSE ||
           client_it->second->state == STATE_CGI_ERROR)) {
            
      //  client->second->_ProcessCgi->GeneretCgiResponse();

      
        client_it->second->state = STATE_SENDING;
        for (size_t j = 0; j < _poll_fds.size(); ++j) {
          if (_poll_fds[j].fd == client_fd) {
            _poll_fds[j].events |= POLLOUT;
            break;
          }
        }
      }
    }
  }
}

void Server::handleCgiStdinWrite() {
  std::vector<int> ready_pipe_fds;
  for (size_t i = 0; i < _poll_fds.size(); ++i)
    if (_poll_fds[i].revents & POLLOUT && _pipe_to_client_fd.count(_poll_fds[i].fd))
      ready_pipe_fds.push_back(_poll_fds[i].fd);

  for (size_t i = 0; i < ready_pipe_fds.size(); ++i) {
    int pipe_fd = ready_pipe_fds[i];
    int client_fd = _pipe_to_client_fd[pipe_fd];
    std::map<int, Client*>::iterator it = _clients.find(client_fd);
    if (it == _clients.end())
      continue;

    Client* client = it->second;
    if (client->cgi_input_buffer.empty()) {
      close(pipe_fd);
      for (size_t j = 0; j < _poll_fds.size(); ++j) {
        if (_poll_fds[j].fd == pipe_fd) {
          _poll_fds.erase(_poll_fds.begin() + j);
          break;
        }
      }
      _pipe_to_client_fd.erase(pipe_fd);
      client->cgi_stdin_fd = -1;
      continue;
    }

    size_t to_write = std::min(client->cgi_input_buffer.size(), (size_t)CGI_CHUNK_SIZE);
    ssize_t bytes = write(pipe_fd, client->cgi_input_buffer.c_str(), to_write);
    if (bytes > 0)
      client->cgi_input_buffer.erase(0, bytes);
  }
}

void Server::handlePollOut() {
  std::vector<int> ready_write_fds;
  for (size_t i = 0; i < _poll_fds.size(); ++i)
    if (_poll_fds[i].revents & POLLOUT)
      ready_write_fds.push_back(_poll_fds[i].fd);

  for (size_t i = 0; i < ready_write_fds.size(); ++i) {
    int fd = ready_write_fds[i];
    std::map<int, Client*>::iterator client_it = _clients.find(fd);
    if (client_it != _clients.end())
      sendResponse(fd);
  }
}

void Server::handlePollErrors() {
  std::vector<int> error_fds;
  for (size_t i = 0; i < _poll_fds.size(); ++i)
    if (_poll_fds[i].revents & (POLLERR | POLLHUP))
      error_fds.push_back(_poll_fds[i].fd);

  for (size_t i = 0; i < error_fds.size(); ++i) {
    int fd = error_fds[i];

    if (_clients.count(fd)) {
      closeClient(fd);
      continue;
    }

    if (_pipe_to_client_fd.count(fd)) {
      int client_fd = _pipe_to_client_fd[fd];
      std::map<int, Client*>::iterator it = _clients.find(client_fd);
      if (it != _clients.end()) {
        Client* client = it->second;
        if (client->cgi_pid != -1)
          _cgi->killCgi(client, SIGKILL);
        client->state = STATE_CGI_ERROR;
      }
      continue;
    }

    if (_fd_to_server_idx.count(fd)) {
      struct sockaddr_in addr;
      socklen_t len = sizeof(addr);
      std::ostringstream oss;
      if (getsockname(fd, (struct sockaddr*)&addr, &len) >= 0) {
        char ip[16];
        inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
        int port = ntohs(addr.sin_port);
       oss << "listen socket error" << " on " << ip << ":" << port;
      }
      _logger.warn(oss.str());
      for (size_t j = 0; j < _poll_fds.size(); ++j) {
        if (_poll_fds[j].fd == fd) {
          _poll_fds.erase(_poll_fds.begin() + j);
          break;
        }
      }
      for (size_t j = 0; j < _listen_fds.size(); ++j) {
        if (_listen_fds[j] == fd) {
          _listen_fds.erase(_listen_fds.begin() + j);
          break;
        }
      }
      _fd_to_server_idx.erase(fd);
      close(fd);
    }
  }
}
