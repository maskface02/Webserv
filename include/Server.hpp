/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 21:39:15 by zatais            #+#    #+#             */
/*   Updated: 2026/07/03 15:08:42 by m45kf4c3         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef SERVER_HPP
#define SERVER_HPP

#include "WebServ.hpp"

#define BACKLOG 128
#define POLL_TIMEOUT 5000
#define CLIENT_TIMEOUT 60
#define CGI_TIMEOUT 60
#define CGI_CHUNK_SIZE 65536

class Request;
class Response;
class ProcessCgi;
class ProcessRequest;



enum ClientState {
    STATE_READING,
    STATE_CGI_RUNNING,
    STATE_WRITING_RESPONSE,
    STATE_CGI_ERROR,
    STATE_SENDING
};

struct Client {
    int             fd;
    std::string     ip;
    int             port;
    std::string     read_buffer;
    std::string     write_buffer;
    ClientState     state;
    bool            keep_alive;
    int             server_idx;
    time_t          last_activity;

    int             cgi_pid;
    int             cgi_stdin_fd;
    int             cgi_stdout_fd;
    std::string     cgi_input_buffer;
    std::string     cgi_output_buffer;
    time_t          cgi_start_time;

    Request*           request; 
    ProcessCgi*         processCgi;
    ProcessRequest*     processRq;
};

class Cgi;

class Server {
  private:
    Logger                      _logger;
    Config                      _config;
    std::vector<int>            _listen_fds;
    std::vector<struct pollfd>  _poll_fds;
    std::map<int, Client*>      _clients;
    std::map<int, int>          _fd_to_server_idx;
    std::map<int, int>          _pipe_to_client_fd;
    Cgi*                        _cgi;
    static bool                        running;

    void    createSockets();
    void    checkTimeouts();
    void    closeClient(int fd);
    void    acceptConnection(int listen_fd);
    void    handleClientRead(int client_fd);
    size_t  getRequestSize(std::string& buffer);
    bool    iequal(std::string& a, const std::string& b);
    size_t  findHeaderValue(std::string& buffer, const std::string& name, size_t headerEnd);
    size_t  parseChunkedBody(std::string& buffer, size_t bodyStart);
    Client* initClient(int client_fd, int listen_fd, const std::string& client_ip, int client_port);
    void    generateResponse(Client* client);
    void    sendResponse(int client_fd);
    void    handlePollIn();
    void    handleCgiPipeRead();
    void    handlePollOut();
    void    handlePollErrors();
    void    handleCgiStdinWrite();

    Server();
    Server(const Server&);
    Server& operator=(const Server&);

  public:
    Server(Config& conf);
    ~Server();

    void run();

    static void setNonBlocking(int fd);
    static void addToPoll(int fd, short events, std::vector<struct pollfd>& poll_fds);
    static void signalHandler(int sig); 
};
#endif // !SERVER_HPP
