
#ifndef SERVESTATIQRQ_HPP
#define SERVESTATICRQ_HPP

#include "WebServ.hpp"

class ServeStaticRq
{
    ProcessRequest*                     ProcessRq;
    Request*                            request;
    std::string                         Status;
    std::string                         resp_body;
    ServerConfig&                       serv;
    public:
    ServeStaticRq(Request& request, ProcessRequest& PrsRq,ServerConfig& srv);

    void                servFile(std::string& path);
    void                ServeGetRequest(std::string resource_path);
    void                check_AutoIndex();
    void                html_list_dir();
    void                redirect_path();
    void                ServeDeleteRq();
    void                ServePostRq();
    void                ServeError(int status_code);
    void                status();
    static std::string                html_Error_page(int status_code, std::string stat);

    ///geters

    std::string         getRespBody() const;
    std::string         getStatus() const;
};


#endif