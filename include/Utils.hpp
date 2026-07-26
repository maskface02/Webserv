#ifndef UTILS_HPP
#define UTILS_HPP

#include <map>
#include <poll.h>
#include <vector>

struct Client;

void removeFdFromPoll(int fd, std::vector<struct pollfd> &poll_fds);
void deleteClientObjects(Client *client);
void switchClientToSending(Client *client,
                           std::vector<struct pollfd> &poll_fds);
void closePipeAndRemove(int fd, std::vector<struct pollfd> &poll_fds,
                        std::map<int, int> &pipe_map, int &client_fd);

#endif
