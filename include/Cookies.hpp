#ifndef COOKIES_HPP
#define COOKIES_HPP

#include "WebServ.hpp"

class Cookies
{
    private:
        std::string     session_id;
        std::string     last_activity;
        std::string     login;
    public:
    Cookies(std::string& id);
    ~Cookies();

    void     creatSession(std::string id);
    // destroySession(id);
    //isExpired(id);
    //updateLastActivity(id);
    //getLastActivity(id); 

};



#endif

// SessionStore() => glopale or static member