/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zatais <zatais@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/15 13:28:58 by zatais            #+#    #+#             */
/*   Updated: 2026/05/17 05:38:17 by zatais           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "WebServ.hpp"

class Logger {
private:
  Logger(const Logger &);
  Logger &operator=(const Logger &);
  std::string getCurrentTimestamp();

public:
  Logger();
  ~Logger();

  void info(const std::string &message);
  void warn(const std::string &message);
  void error(const std::string &message);

  void logConnection(const std::string &client_ip, int port, bool connected);
  void logRequest(const std::string &client_ip, const std::string &method,
                  const std::string &path, const std::string &protocol,
                  int status, size_t response_size);
};

#endif // 
