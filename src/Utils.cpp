#include "../include/WebServ.hpp"

void removeFdFromPoll(int fd, std::vector<struct pollfd> &poll_fds)
{
    for (size_t i = 0; i < poll_fds.size(); ++i)
    {
        if (poll_fds[i].fd == fd)
        {
            poll_fds.erase(poll_fds.begin() + i);
            break;
        }
    }
}

void deleteClientObjects(Client *client)
{
    delete client->request;
    delete client->processRq;
    delete client->processCgi;
    client->request = NULL;
    client->processRq = NULL;
    client->processCgi = NULL;
}

void switchClientToSending(Client *client, std::vector<struct pollfd> &poll_fds)
{
    client->state = STATE_SENDING;
    for (size_t i = 0; i < poll_fds.size(); ++i)
    {
        if (poll_fds[i].fd == client->fd)
        {
            poll_fds[i].events = POLLOUT;
            break;
        }
    }
}

void closePipeAndRemove(int fd, std::vector<struct pollfd> &poll_fds,
                        std::map<int, int> &pipe_map, int &client_fd)
{
    close(fd);
    removeFdFromPoll(fd, poll_fds);
    pipe_map.erase(fd);
    client_fd = -1;
}
