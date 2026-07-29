/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/21 18:30:42 by lasoubai          #+#    #+#             */
/*   Updated: 2026/07/29 23:10:43 by lasoubai         ###   ########.fr       */
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
  respLine = Line.str();
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
  respHeaders = Headers.str();
}

std::string Response::matchMimeType(std::string extension) {
  std::map<std::string, std::string>::iterator it;
  it =  mime_map.find(extension);
  if (it !=  mime_map.end()) {
    return (it->second);
  }
  it =  mime_map.find("default");
  return (it->second);
}

void Response::response() {
  httpResponse = respLine + respHeaders + serveStaticRq->getRespBody();
}

void Response::mime_Types() {

   mime_map[".html"] = "text/html";
   mime_map[".htm"] = "text/html";
   mime_map[".css"] = "text/css";
   mime_map[".js"] = "application/javascript";
   mime_map[".json"] = "application/json";
   mime_map[".png"] = "image/png";
   mime_map[".jpeg"] = "image/jpeg";
   mime_map[".gif"] = "image/gif";
   mime_map[".mp3"] = "audio/mpeg";
   mime_map[".mp4"] = "audio/mpeg";
   mime_map[".xml"] = "application/xml";
   mime_map[".pdf"] = "application/pdf";
   mime_map[".txt"] = "text/plain";
   mime_map["default"] = "application/octet-stream";
}

std::string generateHttpDate() {
  time_t now = time(NULL);
  struct tm *gmt = gmtime(&now);
  char buffer[128];
  strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
  return std::string(buffer);
}

std::string Response::getHttpResponse() const { return (httpResponse); }

Response::~Response(){}