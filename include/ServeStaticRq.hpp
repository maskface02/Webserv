
#ifndef SERVESTATICRQ_HPP
#define SERVESTATICRQ_HPP

#include "WebServ.hpp"

class ServeStaticRq
{
   Client*                              client;
    std::string                         status_messg;
    std::string                         resp_body;
    ServerConfig&                       serv;
    std::string                         file_path;
    public:
    ServeStaticRq(Client* client ,ServerConfig& srv);

    std::string                        servFile(std::string& path);
    void                               _ServeGetRequest(std::string resource_path);
    void                               check_AutoIndex();
    void                               html_list_dir();
    void                               redirect_path();
    void                               _ServeDeleteRq();
    void                               _ServePostRq();
    void                               _ServeError(int status_code);
   
    static std::string                 html_Error_page(int status_code, std::string stat);
    void                               upload_files();
    std::vector<std::string>            directory_files();
    void                                check_exist_file(std::string path, std::vector<std::string>& files);
    void                                check_valid_path(std::string path);
    ///geters

    std::string         getRespBody() const;
    std::string         getStatus() const;

    //
    void                   setResponseBody(std::string rsp_body);
};


#endif


///serving errors