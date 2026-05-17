/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zatais <zatais@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 21:51:55 by zatais            #+#    #+#             */
/*   Updated: 2026/05/15 23:35:34 by zatais           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../include/WebServ.hpp"

Server::Server(){}

Server::Server(Config& conf) : _config(conf){}

Server::~Server(){}

/******************************************************************************/

void Server::setNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    _logger.error("fcntl set non-blocking failed");
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

  while (true) {
    sleep(1);
  }
}


