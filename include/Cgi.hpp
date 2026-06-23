/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zatais <zatais@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/13 10:00:00 by zatais            #+#    #+#             */
/*   Updated: 2026/06/14 22:24:53 by m45kf4c3         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_HPP
#define CGI_HPP

#include "WebServ.hpp"

class Cgi {
  private:
    std::vector<struct pollfd>&     _poll_fds;
    std::map<int, int>&             _pipe_to_client_fd;
    Logger&                         _logger;

    Cgi();
    Cgi(const Cgi&);
    Cgi& operator=(const Cgi&);

    bool    createPipes(int pipe_to_cgi[2], int pipe_from_cgi[2]);
    void    initCgiClient(Client* client, pid_t pid, int stdout_fd);

  public:
    Cgi(std::vector<struct pollfd>& poll_fds, std::map<int, int>& pipe_to_client_fd, Logger& logger);
    ~Cgi();

    void    startCgi(Client* client, std::string& interpreter, std::string& script_path, char** envp);
    void    handleCgiRead(std::map<int, int>::iterator pipe_it, std::map<int, Client*>& clients);
    void    cleanupCgi(Client* client, int exit_status);
    void    killCgi(Client* client, int signal);
};

#endif // !CGI_HPP
