#ifndef PROCESSCGI_HPP
#define PROCESSCGI_HPP

#include "WebServ.hpp"

class ProcessCgi
{
  private:
    std::map <std::string,std::string>          env_map;
    char**                                      env;
    std::string                                 cgi_path; 
    std::string                                 connection;
    Client *                                    _client;
    std::string                                 script_path; 
    std::map<std::string, Session>*             sessions;
    ProcessCgi();
    ProcessCgi(const ProcessCgi&);
    ProcessCgi& operator=(const ProcessCgi&);
  public:
    ProcessCgi(Client *client, ProcessRequest &processRq, std::map<std::string, Session>& _sessions);
    ~ProcessCgi();
    void                                        envMap();
    void                                        envArray();
    void                                        generetCgiResponse();
    void                                        errorResponse(int status_code,std::string status_messg);
    std::string                                 check_valid_status(std::string& value);
    void                                        defineStatusHeader(std::string& Head,std::string& Line);
    void                                        checkClearClientSession(std::string& addHeader);
    char**                                      getEnv() const;
    std::string&                                getCgiPath();
};
 char                env_meta_data(char c);



#endif
