/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/21 18:30:42 by lasoubai          #+#    #+#             */
/*   Updated: 2026/06/26 22:35:52 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/WebServ.hpp"


Response::Response(ProcessRequest& ProcessReq, ServeStaticRq& _staticRq, Request& req)
    :Static(&_staticRq),ProcessRq(&ProcessReq),request(&req)
{
    ResponseLine();
    MIME_Types();
    StaticRespHeaders();
    _Response();
}


void Response::ResponseLine()
{
    std::stringstream Line;
    Line << request->getRequestLine().HttpVers;
    Line << " " << ProcessRq->getStatusCode() << " " << Static->getStatus();
    Line << "\r\n";
    RespLine  = Line.str(); 
}

// MIME :: Multipurpose Internet Mail Extensions.
void Response::StaticRespHeaders()
{
    std::stringstream Headers ;
    // std::string Date;
   
    Headers << "Server: " << "WebServer/1.1"<<"\r\n"; 
    // Headers<< "Date: "<<Date<<"\r\n"; //not done yet
    
    if (!Static->getRespBody().empty())
    {
        Headers << "Content-Length: " << Static->getRespBody().size() << "\r\n";
        Headers << "Content-Type: " << MatchMimeType(ProcessRq->getExtension()) << "\r\n";
    }
    Headers << "Connection: " << request->getConnection() << "\r\n";
    if (ProcessRq->is_RedirecRq)
    {
        Headers << "Location: " <<ProcessRq->getRedirectUrl() << "\r\n";
    }
    Headers <<"\r\n";
     RespHeaders = Headers.str();
}

std::string Response::MatchMimeType(std::string extension)
{
    std::map<std::string, std::string> ::iterator it;
    it = MIME.find(extension);
    if (it != MIME.end())
    {
        return(it->second);
    }
    it = MIME.find("default");
    return (it->second);
}

void Response::_Response()
{
    HttpResponse = RespLine + RespHeaders + Static->getRespBody();
}

void Response::MIME_Types()
{
    // type/subtype
    MIME[".html"] = "text/html";  
    MIME[".htm"] = "text/html";  
    MIME[".css"] = "text/css";
    MIME[".js"] = "application/javascript";    
    MIME[".json"] = "application/json";     
    MIME[".png"] = "image/png" ;   
    MIME[".jpeg"] = "image/jpeg";   
    MIME[".gif"] = "image/gif";   
    MIME[".mp3"] = "audio/mpeg";   
    MIME[".mp4"] = "audio/mpeg";
    MIME[".xml"] = "application/xml";   
    MIME[".pdf"] = "application/pdf";   
    MIME[".txt"] = "text/plain"; 
    MIME["default"] = "text/plain"; 
    
    //=> default 
    // in case excutable or bin file =>  application/octet-stream 
}

 std::string   Response::getHttpResponse() const
 {
    return(HttpResponse);
 }