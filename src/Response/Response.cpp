/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/21 18:30:42 by lasoubai          #+#    #+#             */
/*   Updated: 2026/07/03 10:48:47 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/WebServ.hpp"


Response::Response(Client* _client, ServeStaticRq& _staticRq ,ServerConfig& srv)
    : serveStaticRq(&_staticRq),client(_client)
{   
    status_map();
    if (client->processRq->getStatusCode() >= 201)
        serveStaticRq->setResponseBody(serveError(client->processRq->getStatusCode(),srv));
    //if (client->processRq->getStatusCode() <= 201)
        //js body
    responseLine();
    mime_Types();
    staticRespHeaders();
    response();
}

void Response::responseLine()
{
    std::stringstream Line;
    Line << client->request->getRequestLine().HttpVers;
    Line << " " << client->processRq->getStatusCode() << " " << serveStaticRq->getStatus();
    Line << "\r\n";
    _RespLine  = Line.str(); 
}

// MIME :: Multipurpose Internet Mail Extensions.
void Response::staticRespHeaders()
{
    std::stringstream Headers;
    std::string Date = generateHttpDate();
     Headers<< "Date: "<< Date <<"\r\n";
    Headers << "Server: " << "WebServer/1.1" <<"\r\n"; 
    if (!serveStaticRq->getRespBody().empty())
    {
        Headers << "Content-Length: " << serveStaticRq->getRespBody().size() << "\r\n";
        Headers << "Content-Type: " << matchMimeType(client->processRq->getExtension()) << "\r\n";
    }
    Headers << "Connection: " << client->request->getConnection() << "\r\n";
    if (client->processRq->is_RedirecRq || client->processRq->getStatusCode() == 201)
        Headers << "Location: " <<client->processRq->getRedirectUrl() << "\r\n";
    Headers <<"\r\n";
     _RespHeaders = Headers.str();
}

std::string Response::matchMimeType(std::string extension)
{
    std::map<std::string, std::string> ::iterator it;
    it = _Mime_map.find(extension);
    if (it != _Mime_map.end())
    {
        return(it->second);
    }
    it = _Mime_map.find("default");
    return (it->second);
}

void Response::response()
{
    _HttpResponse = _RespLine + _RespHeaders + serveStaticRq->getRespBody();
}

std::string Response::serveError(int status_code, ServerConfig& srv)
{ 
    client->processRq->setExtension(".html");
    std::map<int, std::string>::iterator it;
    it = srv.error_pages.find(status_code);
    if (it !=  srv.error_pages.end())
        return (serveStaticRq->servFile(it->second));
    return(ServeStaticRq::html_Error_page(status_code, getStatusMsg(status_code)));
}

void Response::mime_Types()
{
    // type/subtype
    _Mime_map[".html"] = "text/html";  
    _Mime_map[".htm"] = "text/html";  
    _Mime_map[".css"] = "text/css";
    _Mime_map[".js"] = "application/javascript";    
    _Mime_map[".json"] = "application/json";     
    _Mime_map[".png"] = "image/png" ;   
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

void Response:: status_map()
{
    status_messg[200]   =   "OK";
    status_messg[201]   =   "Created";
    status_messg[301]   =   "Moved Permanently";
    status_messg[400]   =   "Bad Request";
    status_messg[403]   =   "Forbidden";
    status_messg[404]   =   "Not Found";
    status_messg[405]   =   "Method Not Allowed";
    status_messg[411]   =   "Length Required";
    status_messg[413]   =   "Content Too Large";
    status_messg[414]   =   "URI Too Long";
    status_messg[500]   =   "Internal Server Error";
    status_messg[501]   =   "Not Implemented";
    status_messg[505]   =   "HTTP Version Not Supported";
    status_messg[502]   =   "Bad Gateway";
    
}

std::string generateHttpDate()
{
    time_t now = time(NULL);
    struct tm* gmt = gmtime(&now);
    char buffer[128];
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
    return std::string(buffer);
}

std::string& Response::getStatusMsg(int code)
{
    std::map<int , std::string> :: iterator it ;
    it = status_messg.find(code);
    return(it->second);
}

 std::string   Response::getHttpResponse() const
 {
    return(_HttpResponse);
 }
