#ifndef CLIENTHANDLER_HPP
#define CLIENTHANDLER_HPP

#include "WebServ.hpp"
#include "RequestDelimiter.hpp"

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

  Client *initClient(int client_fd, int listen_fd,
                     const std::string &client_ip, int client_port);

public:
  ClientHandler(std::map<int, Client *> &clients,
                std::vector<struct pollfd> &poll_fds,
                Config &config,
                Logger &logger,
                Cgi *cgi,
                std::map<int, int> &fd_to_server_idx,
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
