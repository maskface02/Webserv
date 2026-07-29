/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientHandler.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zatais <zatais@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/29 15:12:40 by zatais            #+#    #+#             */
/*   Updated: 2026/07/29 15:12:42 by zatais           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENTHANDLER_HPP
#define CLIENTHANDLER_HPP

#include "RequestDelimiter.hpp"
#include "WebServ.hpp"

struct Session {
  time_t last_activity;
  int counter;
};

class ClientHandler {
private:
  std::map<int, Client *> &_clients;
  std::vector<struct pollfd> &_poll_fds;
  Config &_config;
  Logger &_logger;
  Cgi *_cgi;
  std::map<int, int> &_fd_to_server_idx;
  std::vector<int> &_listen_fds;
  RequestDelimiter _delimiter;

  Client *initClient(int client_fd, int listen_fd, const std::string &client_ip,
                     int client_port);

  std::map<std::string, Session> sessions;

public:
  ClientHandler(std::map<int, Client *> &clients,
                std::vector<struct pollfd> &poll_fds, Config &config,
                Logger &logger, Cgi *cgi, std::map<int, int> &fd_to_server_idx,
                std::vector<int> &listen_fds);
  ~ClientHandler();

  void acceptConnection(int listen_fd);
  void handleClientRead(int client_fd);
  bool readClientData(Client *client);
  void processCompleteRequest(Client *client);
  void sendResponse(int client_fd);
  void closeClient(int fd);
};

#endif
