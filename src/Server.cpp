/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 21:39:15 by zatais            #+#    #+#             */
/*   Updated: 2026/07/03 15:08:42 by zatais           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/WebServ.hpp"

bool Server::running = true;

Server::Server() {}

Server::Server(Config &conf)
    : _config(conf), _cgi(new Cgi(_poll_fds, _pipe_to_client_fd, _logger)) {
  _clientHandler = new ClientHandler(_clients, _poll_fds, _config, _logger,
                                     _cgi, _fd_to_server_idx, _listen_fds);
  _pollDispatcher = new PollDispatcher(_poll_fds, _clients, _pipe_to_client_fd,
                                       _fd_to_server_idx, _listen_fds, _logger,
                                       _cgi, *_clientHandler);
}

Server::~Server() {
  for (std::map<int, Client *>::iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    if (it->second->state == STATE_CGI_RUNNING && it->second->cgi_pid != -1)
      _cgi->killCgi(it->second, SIGKILL);
    close(it->second->fd);
    delete it->second->request;
    delete it->second->processRq;
    delete it->second->processCgi;
    delete it->second;
  }

  for (size_t i = 0; i < _listen_fds.size(); ++i)
    close(_listen_fds[i]);

  delete _pollDispatcher;
  delete _clientHandler;
  delete _cgi;
}

/******************************************************************************/

void Server::signalHandler(int sig) {
  (void)sig;
  running = false;
}

void Server::setNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Server::addToPoll(int fd, short events,
                       std::vector<struct pollfd> &poll_fds) {
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = events;
  pfd.revents = 0;
  poll_fds.push_back(pfd);
}

void Server::checkTimeouts() {
  time_t now = time(NULL);
  std::vector<int> to_close;

  for (std::map<int, Client *>::iterator it = _clients.begin();
       it != _clients.end(); ++it) {
    if (it->second->state == STATE_CGI_RUNNING && it->second->cgi_pid != -1) {
      if (difftime(now, it->second->cgi_start_time) > CGI_TIMEOUT)
        handleCgiTimeout(it->second);
    }
    else if (difftime(now, it->second->last_activity) > CLIENT_TIMEOUT) {
      std::ostringstream oss;
      oss << "Client timeout: " << it->second->ip << ":" << it->second->port;
      _logger.warn(oss.str());
      to_close.push_back(it->first);
    }
  }

  for (size_t i = 0; i < to_close.size(); ++i)
    _clientHandler->closeClient(to_close[i]);
}

void Server::handleCgiTimeout(Client *client)
{
  std::ostringstream oss;
  oss << "CGI timeout: " << client->ip << ":" << client->port;
  _logger.warn(oss.str());
  _cgi->killCgi(client, SIGKILL);
  std::string body = ServeStaticRq::html_Error_page(504, "Gateway Timeout");
  client->processRq->setStatusCode(504);
  std::ostringstream res;
  res << "HTTP/1.1 504 Gateway Timeout\r\n"
      << "Content-Length: " << body.size() << "\r\n\r\n"
      << body;
  _logger.logRequest(client->ip,
                     client->request->getRequestLine().Method,
                     client->request->getRequestLine().Path,
                     client->request->getRequestLine().HttpVers,
                     client->processRq->getStatusCode(),
                     client->write_buffer.size());
  client->write_buffer = res.str();
  client->write_offset = 0;
  switchClientToSending(client, _poll_fds);
}

void Server::createSockets() {
  std::vector<ServerConfig> &servers = _config.getServers();

  for (size_t srv_idx = 0; srv_idx < servers.size(); ++srv_idx) {
    ServerConfig &server = servers[srv_idx];
    for (size_t i = 0; i < server.listens.size(); ++i) {
      ListenConfig &lc = server.listens[i];

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

      if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
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

  while (running) {
    int ready = poll(&_poll_fds[0], _poll_fds.size(), POLL_TIMEOUT);
    if (ready < 0) {
      if (errno == EINTR)
        continue;
      _logger.error("poll failed");
    }

    if (!_clients.empty())
      checkTimeouts();

    _pollDispatcher->handlePollErrors();
    _pollDispatcher->handlePollIn();
    _pollDispatcher->handleCgiPipeRead();
    _pollDispatcher->handleCgiStdinWrite();
    _pollDispatcher->handlePollOut();
  }
}
