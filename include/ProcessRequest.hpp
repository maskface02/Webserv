#ifndef  PROCESSREQUEST_HPP
#define  PROCESSREQUEST_HPP

#include "WebServ.hpp"

class ProcessRequest
{
    Request*            request;
    Location            target_location;
    std::string         resource_path;
   
  
    bool                is_Static;
    int                 status_code;
    std::string         extension;
    std::string         redirect_url;
    std::string         Index_file;
    std::string         cgi_path;
public:
    bool                is_CgiRq;
    bool                is_dir;  
    bool                is_RedirecRq;
    bool                is_file;
    ProcessRequest(Client* client,  ServerConfig& srv);
    void                            init_variable();
    void                            check_status();
    void                            match_location(ServerConfig& server);
    void                            check_allowed_method();
    int                            define_type();
    void                            check_index_file();
    void                            extract_file_extension();
    void                            check_redirction();
    void                            check_Cgi();
    bool                            check_location_extention();
    void                            check_max_body_size(ServerConfig& _srv);
    void                            normlize_location_path(ServerConfig& server);
    //Getter

    std::string                 getExtension()const;
    int                         getStatusCode() ;
    std::string                 getResourcePath() const;
    std::string                 getIndexFile() const;
    Location                    getLocation() const;
    std::string                 getCgiPath() const;
    std::string                 getRedirectUrl() const ;

    //seter
    void                        setStatusCode(int code);
    void                        setRedirectURL(std::string redct_url);
    void                        setExtension(std::string _extension);
    void                        setRedirctUrl(std::string& url);
};

std::string generateHttpDate();
















#endif
