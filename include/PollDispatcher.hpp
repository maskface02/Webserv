#ifndef POLLDISPATCHER_HPP
#define POLLDISPATCHER_HPP

#include "WebServ.hpp"

class ClientHandler;

class PollDispatcher {
private:
  std::vector<struct pollfd> &_poll_fds;
  std::map<int, Client *> &_clients;
  std::map<int, int> &_pipe_to_client_fd;
  std::map<int, int> &_fd_to_server_idx;
  std::vector<int> &_listen_fds;
  Logger &_logger;
  Cgi *_cgi;
  ClientHandler &_clientHandler;

  void handleCgiPipePollError(int fd);
  void handleListenSocketPollError(int fd);
  void removeListenFd(int fd);

  PollDispatcher();
  PollDispatcher(const PollDispatcher&);
  PollDispatcher& operator=(const PollDispatcher&);
public:
  PollDispatcher(std::vector<struct pollfd> &poll_fds,
                 std::map<int, Client *> &clients,
                 std::map<int, int> &pipe_to_client_fd,
                 std::map<int, int> &fd_to_server_idx,
                 std::vector<int> &listen_fds, Logger &logger, Cgi *cgi,
                 ClientHandler &clientHandler);
  ~PollDispatcher();

  void handlePollIn();
  void handlePollOut();
  void handlePollErrors();
  void handleCgiPipeRead();
  void handleCgiStdinWrite();
};

#endif
