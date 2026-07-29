/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SessionManager.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/04 17:34:31 by lasoubai          #+#    #+#             */
/*   Updated: 2026/07/29 23:41:43 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/WebServ.hpp"


SessionManager::SessionManager(){}

SessionManager::SessionManager(Client* _client,std::map<std::string, Session>& _sessions)
:sessions(&_sessions),client(_client)
{
    isExpired();
    if (client->is_new)
        creatSession(client->session_id);
    else if (!client->is_new)
        check_session(); 
}

void    SessionManager::check_session()
{
    std::map<std::string, Session>::iterator it;
    it = sessions->find(client->session_id);
    if (it == sessions->end())
    {

        std::string new_id = generateSessionId();
        creatSession(new_id);
        client->session_id = new_id;
        client->is_new = true;
    }
    else
        updateSession(client->session_id);
}


void    SessionManager::creatSession(std::string id)
{
    Session new_session;
    new_session.last_activity = time(NULL);
    new_session.counter = 0;
    (*sessions)[id] = new_session;
}

void    SessionManager::isExpired()
{
    std::map<std::string,Session>::iterator it;
    it = sessions->begin();
    while(it != sessions->end())
    {
        time_t last_activity = it->second.last_activity;
        if (time(NULL) - last_activity > EXP)
        {
            std::string id = it->first;
            it++;
            destroySession(id);
        }
        else
            it++;
    }
}

void    SessionManager::updateSession(std::string id)
{
    std::map<std::string,Session>::iterator it;
    it = sessions->find(id);
    if (it != sessions->end())
    {
        it->second.last_activity = time(NULL);
        it->second.counter += 1 ;
    }
}

void    SessionManager::destroySession(std::string id)
{
    if (!sessions->empty())
        sessions->erase(id);
}

std::string  generateSessionId()
{
    
    std::string str = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                      "abcdefghijklmnopqrstuvwxyz"
                      "0123456789";
    std::string random_str ;
    size_t i = 0;
    random_str = str[rand() % (str.size() - 10)];
    while (i < 9)
    {
        random_str.append(1,str[rand() % str.size()]);
        i++;
    }
    return(random_str);
}


SessionManager::~SessionManager(){}


