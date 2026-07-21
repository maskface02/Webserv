#include "../include/WebServ.hpp"

PollDispatcher::PollDispatcher(std::vector<struct pollfd> &poll_fds,
                               std::map<int, Client *> &clients,
                               std::map<int, int> &pipe_to_client_fd,
                               std::map<int, int> &fd_to_server_idx,
                               std::vector<int> &listen_fds, Logger &logger,
                               Cgi *cgi, ClientHandler &clientHandler)
    : _poll_fds(poll_fds), _clients(clients),
      _pipe_to_client_fd(pipe_to_client_fd),
      _fd_to_server_idx(fd_to_server_idx), _listen_fds(listen_fds),
      _logger(logger), _cgi(cgi), _clientHandler(clientHandler) {}

PollDispatcher::~PollDispatcher() {}

void PollDispatcher::handlePollIn() {
  std::vector<int> ready_fds;
  for (size_t i = 0; i < _poll_fds.size(); ++i)
    if (_poll_fds[i].revents & POLLIN &&
        !_pipe_to_client_fd.count(_poll_fds[i].fd))
      ready_fds.push_back(_poll_fds[i].fd);

  for (size_t i = 0; i < ready_fds.size(); ++i) {
    if (_fd_to_server_idx.find(ready_fds[i]) != _fd_to_server_idx.end())
      _clientHandler.acceptConnection(ready_fds[i]);
    else if (_clients.find(ready_fds[i]) != _clients.end())
      _clientHandler.handleClientRead(ready_fds[i]);
  }
}

void PollDispatcher::handlePollOut() {
  std::vector<int> ready_write_fds;
  for (size_t i = 0; i < _poll_fds.size(); ++i)
    if (_poll_fds[i].revents & POLLOUT)
      ready_write_fds.push_back(_poll_fds[i].fd);

  for (size_t i = 0; i < ready_write_fds.size(); ++i) {
    int fd = ready_write_fds[i];
    std::map<int, Client *>::iterator client_it = _clients.find(fd);
    if (client_it != _clients.end())
      _clientHandler.sendResponse(fd);
  }
}

void PollDispatcher::handleCgiPipeRead() {
  std::vector<int> ready_pipe_fds;
  for (size_t i = 0; i < _poll_fds.size(); ++i)
    if ((_poll_fds[i].revents & (POLLIN | POLLHUP)) &&
        _pipe_to_client_fd.count(_poll_fds[i].fd))
      ready_pipe_fds.push_back(_poll_fds[i].fd);

  for (size_t i = 0; i < ready_pipe_fds.size(); ++i) {
    int pipe_fd = ready_pipe_fds[i];
    std::map<int, int>::iterator pipe_it = _pipe_to_client_fd.find(pipe_fd);
    if (pipe_it != _pipe_to_client_fd.end()) {
      int client_fd = pipe_it->second;
      _cgi->handleCgiRead(pipe_it, _clients);
      std::map<int, Client *>::iterator client_it = _clients.find(client_fd);
      if (client_it != _clients.end() &&
          (client_it->second->state == STATE_WRITING_RESPONSE ||
           client_it->second->state == STATE_CGI_ERROR)) {
        client_it->second->processCgi->GeneretCgiResponse();
        _logger.logRequest(
            client_it->second->ip,
            client_it->second->request->getRequestLine().Method,
            client_it->second->request->getRequestLine().Path,
            client_it->second->request->getRequestLine().HttpVers,
            client_it->second->processRq->getStatusCode(),
            client_it->second->write_buffer.size());
        switchClientToSending(client_it->second, _poll_fds);
      }
    }
  }
}

void PollDispatcher::handleCgiStdinWrite() {
  std::vector<int> ready_pipe_fds;
  for (size_t i = 0; i < _poll_fds.size(); ++i)
    if (_poll_fds[i].revents & POLLOUT &&
        _pipe_to_client_fd.count(_poll_fds[i].fd))
      ready_pipe_fds.push_back(_poll_fds[i].fd);

  for (size_t i = 0; i < ready_pipe_fds.size(); ++i) {
    int pipe_fd = ready_pipe_fds[i];
    int client_fd = _pipe_to_client_fd[pipe_fd];
    std::map<int, Client *>::iterator it = _clients.find(client_fd);
    if (it == _clients.end())
      continue;

    Client *client = it->second;
    if (client->state != STATE_CGI_RUNNING)
      continue;

    if (!client->cgi_input_buffer ||
        client->cgi_input_offset >= client->cgi_input_buffer->size()) {
      closePipeAndRemove(pipe_fd, _poll_fds, _pipe_to_client_fd,
                         client->cgi_stdin_fd);
      continue;
    }

    size_t rem = client->cgi_input_buffer->size() - client->cgi_input_offset;
    size_t to_write = std::min(rem, (size_t)CGI_CHUNK_SIZE);
    ssize_t bytes = write(
        pipe_fd, client->cgi_input_buffer->data() + client->cgi_input_offset,
        to_write);
    if (bytes > 0) {
      client->cgi_input_offset += bytes;
      client->cgi_start_time = time(NULL);
      if (client->cgi_input_offset == client->cgi_input_buffer->size()) {
        closePipeAndRemove(pipe_fd, _poll_fds, _pipe_to_client_fd,
                           client->cgi_stdin_fd);
      }
    }
  }
}

void PollDispatcher::handleCgiPipePollError(int fd) {
  int client_fd = _pipe_to_client_fd[fd];
  std::map<int, Client *>::iterator it = _clients.find(client_fd);
  if (it != _clients.end()) {
    Client *client = it->second;
    for (size_t i = 0; i < _poll_fds.size(); ++i) {
      if (_poll_fds[i].fd == fd) {
        if (fd == client->cgi_stdout_fd && !(_poll_fds[i].revents & POLLERR))
          break;
        if (client->cgi_pid != -1)
          _cgi->killCgi(client, SIGKILL);
        client->state = STATE_CGI_ERROR;
        break;
      }
    }
  }
}

void PollDispatcher::handleListenSocketPollError(int fd) {
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  std::ostringstream oss;
  if (getsockname(fd, (struct sockaddr *)&addr, &len) >= 0) {
    char ip[16];
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
    int port = ntohs(addr.sin_port);
    oss << "listen socket error" << " on " << ip << ":" << port;
  }
  _logger.warn(oss.str());
  removeFdFromPoll(fd, _poll_fds);
  removeListenFd(fd);
  _fd_to_server_idx.erase(fd);
  close(fd);
}

void PollDispatcher::removeListenFd(int fd) {
  for (size_t j = 0; j < _listen_fds.size(); ++j) {
    if (_listen_fds[j] == fd) {
      _listen_fds.erase(_listen_fds.begin() + j);
      break;
    }
  }
}

void PollDispatcher::handlePollErrors() {
  std::vector<int> error_fds;
  for (size_t i = 0; i < _poll_fds.size(); ++i)
    if (_poll_fds[i].revents & POLLERR)
      error_fds.push_back(_poll_fds[i].fd);

  for (size_t i = 0; i < error_fds.size(); ++i) {
    if (_clients.count(error_fds[i])) {
      _clientHandler.closeClient(error_fds[i]);
      continue;
    }

    if (_pipe_to_client_fd.count(error_fds[i])) {
      handleCgiPipePollError(error_fds[i]);
      continue;
    }

    if (_fd_to_server_idx.count(error_fds[i]))
      handleListenSocketPollError(error_fds[i]);
  }
}
