#ifndef SESSIONMANAGER_HPP
#define SESSIONMANAGER_HPP

#include "WebServ.hpp"
#define EXP 1200


class SessionManager
{
    private:
        std::map<std::string, Session>*                          sessions;         
        Client*                                                  client;         
        SessionManager(const SessionManager&);
        SessionManager& operator=(const SessionManager&);
    public:
        SessionManager();
        SessionManager(Client *client,std::map<std::string, Session>& s);
        ~SessionManager();

        void                                                      creatSession(std::string id);
        void                                                      destroySession(std::string id);
        void                                                      isExpired();
        void                                                      check_session();
        void                                                      updateSession(std::string id);
   
   
};

#endif

