#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "WebServ.hpp"
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

class Response
{
    ServeStaticRq*                              Static;
    ProcessRequest*                             ProcessRq;
    Request*                                    request;
    std::string                                 HttpResponse;
    std::string                                 RespHeaders;
    std::string                                 RespLine;
    std::map<std::string, std::string>          MIME;
    bool                                        is_Error;
    
    public:
    Response(ProcessRequest& ProcessRq, ServeStaticRq& Static, Request& req);
    void                            CgiResponse();
    void                            StaticRespHeaders();
    void                            MIME_Types();
    void                            ResponseLine();
    std::string                     MatchMimeType(std::string extension);
    void                            _Response();
    
    std::string                     getHttpResponse() const;

    

};




#endif
