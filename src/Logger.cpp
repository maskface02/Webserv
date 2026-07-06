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

Logger::Logger(){}

Logger::~Logger() {}

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

void Logger::info(const std::string& message) {std::cout << "[" << getCurrentTimestamp() << "] " << "[" << GREEN "INFO" RESET "] " << message << std::endl;}

void Logger::warn(const std::string& message) {std::cout << "[" << getCurrentTimestamp() << "] " << "[" << YELLOW "WARN" RESET "] " << message << std::endl;}

void Logger::error(const std::string& message) {throw std::runtime_error("[" + getCurrentTimestamp() + "]" + " [" RED "ERROR" RESET "] " + message);}

void Logger::logRequest(const std::string& client_ip, const std::string& method, const std::string& path, const std::string& protocol, int status, size_t response_size) {
  std::ostringstream logMsg;
  std::string requestLine = method + " " + path + " " + protocol;
  std::string statusColor = colorizeStatus(status);
  std::string statusStr = statusText(status);
  std::string sizeStr = formatSize(response_size);

  logMsg << "" << client_ip << " \"" << requestLine << "\" " << statusColor <<" "<< status << " " << statusStr << " "<< RESET << sizeStr;
  info(logMsg.str());
}

std::string Logger::colorizeStatus(int status) {
  if (status >= 200 && status < 300)
    return GREEN;
  else if (status >= 300 && status < 400)
    return YELLOW;
  else if (status >= 400 && status < 500)
    return RED;
  return RESET;
}

std::string Logger::formatSize(size_t bytes) {
  std::ostringstream oss;
  if (bytes < 1024)
    oss << bytes << " B";
  else if (bytes < 1024 * 1024)
    oss << (bytes / 1024.0) << " KB";
  else if (bytes < 1024 * 1024 * 1024)
    oss << (bytes / (1024.0 * 1024.0)) << " MB";
  else
    oss << (bytes / (1024.0 * 1024.0 * 1024.0)) << " GB";
  return oss.str();
}

std::string Logger::statusText(int status) {
  switch (status) {
    case 200: return "OK";
    case 301: return "Moved Permanently";
    case 302: return "Found";
    case 400: return "Bad Request";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 405: return "Method Not Allowed";
    case 413: return "Payload Too Large";
    case 500: return "Internal Server Error";
    case 501: return "Not Implemented";
    case 502: return "Bad Gateway";
    case 503: return "Service Unavailable";
    default:  return "Unknown";
  }
}

void Logger::logConnection(const std::string& client_ip, int port, bool connected, const std::string& server_name) {
  std::ostringstream logMsg;
  if (connected)
    logMsg << "Client " << client_ip << ":" << port << " connected to " << server_name;
  else
    logMsg << "Client " << client_ip << ":" << port << " disconnected from " << server_name;
  info(logMsg.str());
}
