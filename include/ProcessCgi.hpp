#ifndef PROCESSCGI_HPP
#define PROCESSCGI_HPP

#include "WebServ.hpp"

class ProcessCgi
{
  private:
    std::map <std::string,std::string>          env_map;
    std::map <std::string,std::string>          Interp_Map;
    std::string                                 Interp;
    char**                                      env; //new used here !!
    std::string                                 cgi_path;
    std::string                                 script_path;// added

    std::string                                 cgi_output;
    std::string                                 Cgi_resp;
    std::string                                  connection;



  public:
    ProcessCgi(Client *client, ProcessRequest &ProcessRq, Request& request);
    ~ProcessCgi();
    void                EnvMap(Request& request, std::string ClientIp);
    void                EnvArray();
    // void                ParsCgiOutput();
     std::string         CgiResponse(Client *client);
    char**              getEnv() const;
    std::string&         getCgiPath();
    void                 GeneretCgiResponse();
};




#endif
