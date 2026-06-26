#ifndef  PROCESSREQUEST_HPP
#define PROCESSREQUEST_HPP

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

class ProcessRequest
{
    Request*            request;
    ServerConfig*       srv;
    Location            target_location;
    std::string         resource_path;
    bool                is_file;
  
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

ProcessRequest(Request& request,  ServerConfig& srv);

bool                        check_status();
bool                        match_location(ServerConfig& server);
bool                        check_allowed_method();
void                        define_type();
void                        check_index_file();
void                        extract_file_extension();
bool                        check_redirction();
void                        check_Cgi();
bool                        check_location_extention();

//Getter

std::string                 getExtension()const;
int                         getStatusCode() const ;
std::string                 getResourcePath() const;
std::string                 getIndexFile() const;
Location                    getLocation() const;
std::string                 getCgiPath() const;
ServerConfig               getServer() const;
std::string                 getRedirectUrl() const ;

//seter
void                        setStatusCode(int code);
void                        setRedirectURL(std::string redct_url);
void                        setExtension(std::string _extension);

};


















#endif