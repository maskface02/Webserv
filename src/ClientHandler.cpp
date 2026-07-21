#include "../include/WebServ.hpp"

ClientHandler::ClientHandler(std::map<int, Client *> &clients,
                             std::vector<struct pollfd> &poll_fds,
                             Config &config, Logger &logger, Cgi *cgi,
                             std::map<int, int> &fd_to_server_idx,
                             std::vector<int> &listen_fds)
    : _clients(clients), _poll_fds(poll_fds), _config(config), _logger(logger),
      _cgi(cgi), _fd_to_server_idx(fd_to_server_idx), _listen_fds(listen_fds) {}

ClientHandler::~ClientHandler() {}

void ClientHandler::closeClient(int fd) {
  std::map<int, Client *>::iterator it = _clients.find(fd);
  if (it != _clients.end()) {
    if (it->second->state == STATE_CGI_RUNNING && it->second->cgi_pid != -1)
      _cgi->killCgi(it->second, SIGKILL);
    ServerConfig &srv = _config.getServers()[it->second->server_idx];
    std::ostringstream server_name;
    server_name << srv.listens[0].host << ":" << srv.listens[0].port;
    _logger.logConnection(it->second->ip, it->second->port, false,
                          server_name.str());
    close(fd);
    deleteClientObjects(it->second);
    delete it->second;
    _clients.erase(it);
  }
  removeFdFromPoll(fd, _poll_fds);
}

void ClientHandler::acceptConnection(int listen_fd) {
  if (_clients.size() + 1 > 1024 - 3 - _listen_fds.size()) {
    _logger.warn("max clients reached (FD LIMIT)");
    return;
  }

  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);
  int client_fd =
      accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
  if (client_fd < 0) {
    _logger.warn("accept failed");
    return;
  }

  Server::setNonBlocking(client_fd);

  char client_ip[16];
  inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, 16);
  int client_port = ntohs(client_addr.sin_port);

  Client *client = initClient(client_fd, listen_fd, client_ip, client_port);
  _clients[client_fd] = client;
  Server::addToPoll(client_fd, POLLIN, _poll_fds);

  int srv_idx = _fd_to_server_idx[listen_fd];
  ServerConfig &srv = _config.getServers()[srv_idx];
  std::ostringstream server_name;
  server_name << srv.listens[0].host << ":" << srv.listens[0].port;
  _logger.logConnection(client_ip, client_port, true, server_name.str());
}

Client *ClientHandler::initClient(int client_fd, int listen_fd,
                                  const std::string &client_ip,
                                  int client_port) {
  Client *client = new Client();
  client->fd = client_fd;
  client->ip = client_ip;
  client->port = client_port;
  client->read_buffer = "";
  client->write_buffer = "";
  client->state = STATE_READING;
  client->keep_alive = false;
  client->server_idx = _fd_to_server_idx[listen_fd];
  client->last_activity = time(NULL);
  client->cgi_pid = -1;
  client->cgi_stdin_fd = -1;
  client->cgi_stdout_fd = -1;
  client->cgi_input_buffer = NULL;
  client->cgi_output_buffer = "";
  client->write_offset = 0;
  client->cgi_input_offset = 0;
  client->cgi_start_time = 0;
  client->request = NULL;
  client->processCgi = NULL;
  client->processRq = NULL;
  return client;
}

void ClientHandler::handleClientRead(int client_fd) {
  std::map<int, Client *>::iterator it = _clients.find(client_fd);
  if (it == _clients.end())
    return;

  Client *client = it->second;
  if (client->state != STATE_READING)
    return;

  if (!readClientData(client))
    return;

  size_t request_size = _delimiter.getRequestSize(client->read_buffer);
  if (request_size != std::string::npos)
    processCompleteRequest(client);
}

bool ClientHandler::readClientData(Client *client) {
  std::vector<ServerConfig> &servers = _config.getServers();
  size_t max_size = servers[client->server_idx].client_max_body_size;

  size_t max_per_cycle = 64 * 1024;
  size_t read_this_cycle = 0;
  while (read_this_cycle < max_per_cycle) {
    char buffer[4096];
    ssize_t bytes = recv(client->fd, buffer, sizeof(buffer), 0);

    if (bytes > 0) {
      client->read_buffer.append(buffer, bytes);
      client->last_activity = time(NULL);
      if (client->read_buffer.size() > max_size) {
        ParsedRequestLine line = _delimiter.parseRequestLine(client->read_buffer);
        client->write_buffer = "HTTP/1.1 400 Bad Request\r\nContent-Length: "
                               "0\r\nConnection: close\r\n\r\n";
        client->write_offset = 0;
        _logger.logRequest(client->ip, line.method, line.uri, line.httpVers,
                           400, client->write_buffer.size());
        client->keep_alive = false;
        switchClientToSending(client, _poll_fds);
        return false;
      }
      read_this_cycle += bytes;
    } else if (!bytes) {
      closeClient(client->fd);
      return false;
    } else
      break;
  }
  return true;
}

void ClientHandler::processCompleteRequest(Client *client) {
  size_t request_size = _delimiter.getRequestSize(client->read_buffer);
  client->request = new Request(client, client->read_buffer);
  client->read_buffer.erase(0, request_size);
  client->processRq =
      new ProcessRequest(client, _config.getServers()[client->server_idx]);
  if (!client->processRq->is_CgiRq) {
    ServeStaticRq StaticRq(client, _config.getServers()[client->server_idx]);
    Response StaticResponse(client, StaticRq,
                            _config.getServers()[client->server_idx]);
    client->write_buffer = StaticResponse.getHttpResponse();
    client->write_offset = 0;
    _logger.logRequest(client->ip, client->request->getRequestLine().Method,
                       client->request->getRequestLine().URI,
                       client->request->getRequestLine().HttpVers,
                       client->processRq->getStatusCode(),
                       client->write_buffer.size());
    switchClientToSending(client, _poll_fds);
  } else {
    ProcessCgi *procCgi =
        new ProcessCgi(client, *client->processRq, *client->request);
    std::string script = client->processRq->getResourcePath();
    _cgi->startCgi(client, procCgi->getCgiPath(), script, procCgi->getEnv());
  }
}

void ClientHandler::sendResponse(int client_fd) {
  std::map<int, Client *>::iterator it = _clients.find(client_fd);
  if (it == _clients.end())
    return;

  Client *client = it->second;
  if (client->state != STATE_SENDING)
    return;

  if (client->write_offset >= client->write_buffer.size())
    return;

  ssize_t bytes =
      send(client_fd, client->write_buffer.data() + client->write_offset,
           client->write_buffer.size() - client->write_offset, 0);

  if (bytes > 0) {
    client->write_offset += bytes;
    client->last_activity = time(NULL);
  } else if (bytes < 0)
    return;

  if (client->write_offset == client->write_buffer.size()) {
    client->write_buffer.clear();
    client->write_offset = 0;
    for (size_t i = 0; i < _poll_fds.size(); ++i) {
      if (_poll_fds[i].fd == client_fd) {
        _poll_fds[i].events = POLLIN;
        break;
      }
    }
    client->cgi_output_buffer.clear();
    if (client->keep_alive) {
      deleteClientObjects(client);
      client->write_offset = 0;
      client->cgi_input_offset = 0;
      client->state = STATE_READING;
    } else
      closeClient(client_fd);
  }
}
