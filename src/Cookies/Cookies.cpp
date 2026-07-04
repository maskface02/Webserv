/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cookies.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/04 17:34:31 by lasoubai          #+#    #+#             */
/*   Updated: 2026/07/04 18:12:19 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/WebServ.hpp"

// map<stdstring id ,map<key,value>> sessions
Cookies::Cookies(std::string& id)
{
    creatSession(id);
    // destroySession(id);
    //isExpired(id);
    //updateLastActivity(id);
    //getLastActivity(id); 

}


void Cookies::creatSession(std::string id)
{
    
}

Cookies::~Cookies()
{
       
}