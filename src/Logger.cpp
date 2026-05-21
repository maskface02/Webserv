/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zatais <zatais@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/16 16:33:28 by zatais            #+#    #+#             */
/*   Updated: 2026/05/16 16:33:28 by zatais           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/WebServ.hpp"

Logger::Logger() : _useFile(false) {}

Logger::~Logger() {
    if (_logFile.is_open())
        _logFile.close();
}

std::string Logger::getCurrentTimestamp() {
  time_t now = time(NULL);
  struct tm* tm_info = localtime(&now);

  char buffer[32];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);

  struct timeval tv;
  gettimeofday(&tv, NULL);

  std::ostringstream fullTime;
  fullTime << buffer << "." << std::setw(3) << std::setfill('0') << (tv.tv_usec / 1000);
  return fullTime.str();
}

void Logger::info(const std::string& message) {
  std::cout << "[" << getCurrentTimestamp() << "] " << "[" << GREEN "INFO" RESET "] " << message << std::endl;
  if (_useFile && _logFile.is_open())
    _logFile << "[" << getCurrentTimestamp() << "] " << "[INFO] " << message << std::endl;
}

void Logger::warn(const std::string& message) {
  std::cout << "[" << getCurrentTimestamp() << "] " << "[" << YELLOW "WARN" RESET "] " << message << std::endl;
  if (_useFile && _logFile.is_open())
    _logFile << "[" << getCurrentTimestamp() << "] " << "[WARN] " << message << std::endl;
}

void Logger::error(const std::string& message) {
  if (_useFile && _logFile.is_open())
    _logFile << "[" << getCurrentTimestamp() << "] " << "[ERROR] " << message << std::endl;
  throw std::runtime_error("[" + getCurrentTimestamp() + "]" + " [" RED "ERROR" RESET "] " + message);
}

void Logger::setLogFile(const std::string& filename) {
  _logFile.open(filename.c_str(), std::ios::app);
  if (_logFile.is_open())
    _useFile = true;
}

void Logger::logRequest(const std::string& client_ip, const std::string& method, const std::string& path, const std::string& protocol, int status, size_t response_size) {
  std::ostringstream logMsg;
  logMsg << client_ip << " \"" << method << " " << path << " " << protocol << "\" " << status << " " << response_size;
  info(logMsg.str());
}

void Logger::logConnection(const std::string& client_ip, int port, bool connected) {
  std::ostringstream logMsg;
  if (connected)
    logMsg << "Client connected: " << client_ip << ":" << port; // add on which listening server
  else
    logMsg << "Client disconnected: " << client_ip << ":" << port;
  info(logMsg.str());
}
