#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "WebServ.hpp"


enum statusMsg
{
    OK                              = 200,
    CREATED                         = 201,
    MOVED_PERMANENTLY               = 301,
    BAD_REQUEST                     = 400,
    FORBIDDEN                       = 403,
    NOT_FOUND                       = 404,
    METHOD_NOT_ALLOWED              = 405,
    LENTH_REQUIRED                  = 411,
    CONTENT_TOO_LARGE               = 413,
    URI_TOO_LONG                    = 414,
    INTERNAL_SERVER_ERROR           = 500,
    NOT_IMPLEMENTED                 = 501,
    HTTP_VERSION_NOT_SUPPORTED      = 505,
    BAD_GATEWAY                     = 502,
};

class Response
{
    ServeStaticRq*                                      serveStaticRq;
    Client*                                             client;
    std::string                                         httpResponse;
    std::string                                         respHeaders;
    std::string                                         respLine;
    std::map<std::string, std::string>                  mime_map;
    Response(const Response&);
    Response& operator=(const Response&);
    public:
    Response(Client* _client, ServeStaticRq& Static, ServerConfig& srv);
    ~Response();
    void                                                staticRespHeaders();
    void                                                mime_Types();
    void                                                responseLine();
    std::string                                         matchMimeType(std::string extension);
    void                                                response();

    // geter                                
    std::string                                         getHttpResponse() const;
    std::string&                                        getStatusMsg(int code);
    
  

};

#endif
