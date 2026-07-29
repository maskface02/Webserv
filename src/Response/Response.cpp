/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/21 18:30:42 by lasoubai          #+#    #+#             */
/*   Updated: 2026/07/26 20:39:10 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/WebServ.hpp"



Response::Response(Client *_client, ServeStaticRq &_staticRq, ServerConfig &srv)
    : serveStaticRq(&_staticRq), client(_client) {

  if (client->processRq->getStatusCode() >= CREATED )
    serveStaticRq->setResponseBody(
        ServeStaticRq::serveError(client->processRq->getStatusCode(), srv, _client));
  responseLine();
  mime_Types();
  staticRespHeaders();
  response();
}

void Response::responseLine() {
  std::stringstream Line;
  Line << client->request->getRequestLine().HttpVers;
  Line << " " << client->processRq->getStatusCode() << " "
       << Logger::statusText(client->processRq->getStatusCode()) ;
  Line << "\r\n";
  _RespLine = Line.str();
}

void Response::staticRespHeaders() {
  std::stringstream Headers;
  std::string Date = generateHttpDate();
  Headers << "Date: " << Date << "\r\n";
  Headers << "Server: " << "WebServer/1.1" << "\r\n";
  if (!serveStaticRq->getRespBody().empty()) {
    Headers << "Content-Length: " << serveStaticRq->getRespBody().size()
            << "\r\n";
    Headers << "Content-Type: "
            << matchMimeType(client->processRq->getExtension()) << "\r\n";
  }
  Headers << "Connection: " << client->request->getConnection() << "\r\n";

  if (client->processRq->is_RedirecRq ||
      client->processRq->getStatusCode() == CREATED  ||
      client->processRq->getStatusCode() == MOVED_PERMANENTLY) 
    Headers << "Location: " << client->processRq->getRedirectUrl() << "\r\n";
  if (client->is_new)
  {
      Headers << "Set-Cookie: session_id="<< client->session_id<<"; Path=/; HttpOnly;"<<"\r\n";
  }
  Headers << "\r\n";
  _RespHeaders = Headers.str();
}

std::string Response::matchMimeType(std::string extension) {
  std::map<std::string, std::string>::iterator it;
  it = _Mime_map.find(extension);
  if (it != _Mime_map.end()) {
    return (it->second);
  }
  it = _Mime_map.find("default");
  return (it->second);
}

void Response::response() {
  _HttpResponse = _RespLine + _RespHeaders + serveStaticRq->getRespBody();
}

void Response::mime_Types() {
  // type/subtype
  _Mime_map[".html"] = "text/html";
  _Mime_map[".htm"] = "text/html";
  _Mime_map[".css"] = "text/css";
  _Mime_map[".js"] = "application/javascript";
  _Mime_map[".json"] = "application/json";
  _Mime_map[".png"] = "image/png";
  _Mime_map[".jpeg"] = "image/jpeg";
  _Mime_map[".gif"] = "image/gif";
  _Mime_map[".mp3"] = "audio/mpeg";
  _Mime_map[".mp4"] = "audio/mpeg";
  _Mime_map[".xml"] = "application/xml";
  _Mime_map[".pdf"] = "application/pdf";
  _Mime_map[".txt"] = "text/plain";
  _Mime_map["default"] = "application/octet-stream";

  //=> default ?
  // in case excutable or bin file or no extension=>  application/octet-stream
}

std::string generateHttpDate() {
  time_t now = time(NULL);
  struct tm *gmt = gmtime(&now);
  char buffer[128];
  strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
  return std::string(buffer);
}

std::string Response::getHttpResponse() const { return (_HttpResponse); }
