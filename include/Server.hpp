/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zatais <zatais@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 21:39:15 by zatais            #+#    #+#             */
/*   Updated: 2026/05/13 01:51:39 by zatais           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include "Config.hpp"
#include <iostream>

struct Client {
    int         fd;
    std::string read_buffer;
    std::string write_buffer;
    int         state;
    bool        keep_alive;
    int         server_idx;
    time_t      last_activity;

    int         cgi_pid;
    int         cgi_stdin_fd;
    int         cgi_stdout_fd;
    std::string cgi_input_buffer;
    std::string cgi_output_buffer;

    void*       request_obj;
    void*       response_obj;

   };

class Server {
  private:
    Config                      _config;
    std::vector<int>            _listen_fds;
    std::vector<struct pollfd>  _poll_fds;
    std::map<int, Client*>      _clients;
    std::map<int, int>          _fd_to_server_idx;

    Server();
    Server(const Server&);
    Server& operator=(const Server&);
  public:
    Server(Config& conf);
    ~Server();


    void run();

};






#include "Config.hpp"

/*
class Server {
private:

    static const int            BACKLOG = 128;
    static const int            POLL_TIMEOUT = 1000; // ms
    static const int            BUFFER_SIZE = 4096;
    static const int            CLIENT_TIMEOUT = 60; // seconds

    void    createSockets();
    void    addToPoll(int fd, short events);
    void    removeFromPoll(int fd);
    void    acceptConnection(int listen_fd);
    void    handleClientRead(int client_fd);
    void    handleClientWrite(int client_fd);
    void    closeClient(int client_fd);
    void    checkTimeouts();
    void    setNonBlocking(int fd);
    std::string makeHostPortKey(const std::string& host, int port);

public:
    Server(const Config& config);
    ~Server();
    void    run();
};
*/

#endif
