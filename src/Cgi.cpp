/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/13 10:00:00 by zatais            #+#    #+#             */
/*   Updated: 2026/07/11 14:24:31 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/WebServ.hpp"

Cgi::Cgi(std::vector<struct pollfd>& poll_fds, std::map<int, int>& pipe_to_client_fd, Logger& logger)
    : _poll_fds(poll_fds), _pipe_to_client_fd(pipe_to_client_fd), _logger(logger) {}

Cgi::~Cgi() {}

bool Cgi::createPipes(int pipe_to_cgi[2], int pipe_from_cgi[2]) {
  if (pipe(pipe_to_cgi) == -1) {
    _logger.warn("pipe creation failed");
    return false;
  }
  if (pipe(pipe_from_cgi) == -1) {
    close(pipe_to_cgi[0]);
    close(pipe_to_cgi[1]);
    _logger.warn("pipe creation failed");
    return false;
  }
  return true;
}

void Cgi::initCgiClient(Client* client, pid_t pid, int stdout_fd) {
  client->cgi_pid = pid;
  client->cgi_stdin_fd = -1;
  client->cgi_stdout_fd = stdout_fd;
  client->state = STATE_CGI_RUNNING;
  client->cgi_start_time = time(NULL);
}

void Cgi::startCgi(Client* client, std::string& interpreter, std::string& script_path, char** envp) {
  int pipe_to_cgi[2];
  int pipe_from_cgi[2];

  if (!createPipes(pipe_to_cgi, pipe_from_cgi)) {
    client->state = STATE_CGI_ERROR;
    return;
  }

  pid_t pid = fork();
  if (pid == -1) {
    _logger.warn("fork failed");
    close(pipe_to_cgi[0]);
    close(pipe_to_cgi[1]);
    close(pipe_from_cgi[0]);
    close(pipe_from_cgi[1]);
    client->state = STATE_CGI_ERROR;
    return;
  }

  if (!pid) {
    close(pipe_to_cgi[1]);
    close(pipe_from_cgi[0]);
    dup2(pipe_to_cgi[0], STDIN_FILENO);
    dup2(pipe_from_cgi[1], STDOUT_FILENO);
    close(pipe_to_cgi[0]);
    close(pipe_from_cgi[1]);

    char* args[3];
    args[0] = const_cast<char*>(interpreter.c_str());
    args[1] = const_cast<char*>(script_path.c_str());
    args[2] = NULL;

    execve(args[0], args, envp);
    _exit(1);
  }

  close(pipe_to_cgi[0]);
  close(pipe_from_cgi[1]);

  Server::setNonBlocking(pipe_from_cgi[0]);
  initCgiClient(client, pid, pipe_from_cgi[0]);

  Server::addToPoll(pipe_from_cgi[0], POLLIN, _poll_fds);
  _pipe_to_client_fd[pipe_from_cgi[0]] = client->fd;

  if (!client->cgi_input_buffer.empty()) {
    Server::setNonBlocking(pipe_to_cgi[1]);
    Server::addToPoll(pipe_to_cgi[1], POLLOUT, _poll_fds);
    _pipe_to_client_fd[pipe_to_cgi[1]] = client->fd;
    client->cgi_stdin_fd = pipe_to_cgi[1];
  }
  else 
    close(pipe_to_cgi[1]);
}

void Cgi::handleCgiRead(std::map<int, int>::iterator pipe_it, std::map<int, Client*>& clients) {
  int client_fd = pipe_it->second;
  std::map<int, Client*>::iterator client_it = clients.find(client_fd);
  if (client_it == clients.end())
    return;
  if (client_it->second->state != STATE_CGI_RUNNING)
    return;

  Client* client = client_it->second;
  int pipe_fd = pipe_it->first;
  size_t max_per_cycle = 64 * 1024;
  size_t read_this_cycle = 0;

  while (read_this_cycle < max_per_cycle) {
    char buffer[4096];
    ssize_t bytes = read(pipe_fd, buffer, sizeof(buffer));

    if (bytes > 0) {
      client->cgi_output_buffer.append(buffer, bytes);
      client->last_activity = time(NULL);
      read_this_cycle += bytes;
    }
    else if (bytes == 0) {
      int status;//
      pid_t result = waitpid(client->cgi_pid, &status, WNOHANG);
      if (result > 0)
        cleanupCgi(client, status);
      else
        cleanupCgi(client, -1);
      return;
    }
    else
      return;
  }
}

void Cgi::cleanupCgi(Client* client, int exit_status) {
  if (client->cgi_stdin_fd != -1) {
    close(client->cgi_stdin_fd);
    for (size_t i = 0; i < _poll_fds.size(); ++i) {
      if (_poll_fds[i].fd == client->cgi_stdin_fd) {
        _poll_fds.erase(_poll_fds.begin() + i);
        break;
      }
    }
    _pipe_to_client_fd.erase(client->cgi_stdin_fd);
    client->cgi_stdin_fd = -1;
  }

  if (client->cgi_stdout_fd != -1) {
    close(client->cgi_stdout_fd);
    for (size_t i = 0; i < _poll_fds.size(); ++i) {
      if (_poll_fds[i].fd == client->cgi_stdout_fd) {
        _poll_fds.erase(_poll_fds.begin() + i);
        break;
      }
    }
    _pipe_to_client_fd.erase(client->cgi_stdout_fd);
    client->cgi_stdout_fd = -1;
  }

  client->cgi_pid = -1;

  if (exit_status == -1) {
    client->state = STATE_CGI_ERROR;
    return;
  }

  if (WIFEXITED(exit_status) && WEXITSTATUS(exit_status) == 0)
    client->state = STATE_WRITING_RESPONSE;
  else
    client->state = STATE_CGI_ERROR;
}

void Cgi::killCgi(Client* client, int signal) {
  if (client->cgi_pid != -1) {
    kill(client->cgi_pid, signal);
    int status;
    waitpid(client->cgi_pid, &status, 0);
    cleanupCgi(client, status);
  }
}
