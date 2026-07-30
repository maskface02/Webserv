
#ifndef SERVESTATICRQ_HPP
#define SERVESTATICRQ_HPP

#include "WebServ.hpp"

class ServeStaticRq
{
    Client*                             client;
    std::string                         resp_body;
    ServerConfig&                       serv;
    std::string                         file_path;
    ServeStaticRq();
    ServeStaticRq(const ServeStaticRq&);
    ServeStaticRq& operator=(const ServeStaticRq&);
    public:
    ServeStaticRq(Client* client ,ServerConfig& srv);
    ~ServeStaticRq();
    static std::string                 servFile(std::string& path);
    static std::string                 serveError(int status_code, ServerConfig &srv, Client* client);
    void                               serveGetRequest(std::string resource_path);
    void                               check_AutoIndex();
    void                               html_list_dir();
    void                               serveDeleteRq();
    void                               servePostRq();
    void                               delete_files(std::vector<std::string> files);
    static std::string                 html_Error_page(int status_code, std::string stat);
    void                               upload_files();
    std::vector<std::string>           directory_files(std::string& path);
    int                               check_exist_file(std::string path, std::vector<std::string>& files);
    std::string                        last_modif_time(struct stat s);
   
    std::string                        getRespBody() const;

    void                               setResponseBody(std::string rsp_body);
};

#endif