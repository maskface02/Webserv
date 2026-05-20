/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zatais <zatais@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 21:39:15 by zatais            #+#    #+#             */
/*   Updated: 2026/05/20 04:16:04 by zatais           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef SERVER_HPP
#define SERVER_HPP

#include "WebServ.hpp"

#define BACKLOG 128
#define POLL_TIMEOUT 5000
#define CLIENT_TIMEOUT 60

struct Client {
    int         fd;
    std::string ip;
    int         port;
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
    Logger                      _logger;
    Config                      _config;
    std::vector<int>            _listen_fds;// close in the disctructor
    std::vector<struct pollfd>  _poll_fds;
    std::map<int, Client*>      _clients;// new used here need to be deallocated using distructor and close fds too!!
    std::map<int, int>          _fd_to_server_idx;

    void    createSockets();
    void    checkTimeouts();
    void    closeClient(int fd);
    void    setNonBlocking(int fd);
    void    acceptConnection(int listen_fd);
    void    handleClientRead(int client_fd);
    void    addToPoll(int fd, short events);
    size_t  getRequestSize(std::string& buffer);
    Client* initClient(int client_fd, int listen_fd, const std::string& client_ip, int client_port);

    Server();
    Server(const Server&);
    Server& operator=(const Server&);

  public:
    Server(Config& conf);
    ~Server();

    void run();
};

#endif // !SERVER_HPP
