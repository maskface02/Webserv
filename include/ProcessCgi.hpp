#ifndef PROCESSCGI_HPP
#define PROCESSCGI_HPP

class ProcessCgi
{
    std::map <std::string,std::string>          env_map;
    std::map <std::string,std::string>          Interp_Map;
    std::string                                 Interp;
    char**                                      env; //new used here !!
    std::string                                 cgi_path;
    




    public:
        ProcessCgi(Client& client,ProcessRequest &ProcessRq, Request& request);
        void                EnvMap(Request& request, std::string ClientIp);
        void                EnvArray();
        void                InterpMap();
        void                DefInterp(std::string extension);
        // void                ParsCgiOutput();
        

        std::string         CgiResponse(Client *client);
};




#endif