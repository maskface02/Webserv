/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ProcessCgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/17 12:24:10 by lasoubai          #+#    #+#             */
/*   Updated: 2026/06/22 23:28:12 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/WebServ.hpp"

ProcessCgi::ProcessCgi(Client& client,ProcessRequest &ProcessRq, Request& request):cgi_path(ProcessRq.getCgiPath())
{
    if (request.getRequestLine().Method == "Post")
        client.cgi_input_buffer = request.getBody();
    EnvMap(request,client.ip);
    InterpMap();
    DefInterp(ProcessRq.getExtension());
    if (Interp.empty())
        ProcessRq.setStatusCode(404);// not sur about this error handling 
    
}
void ProcessCgi::EnvMap(Request& request,std::string ClientIp)
{
    if (request.getRequestLine().Method == "POST")
    {
        env_map["CONTENT_LENGTH"] = (request.getContentLenght());
        env_map["CONTENT_TYPE"] = (request.getContentType());
    }
    
    env_map["GATEWAY_INTERFACE"] = ("CGI/1.1");
    env_map["QUERY_STRING"] = request.getRequestLine().Query;
    env_map["REMOTE_ADDR"] = ClientIp;

    env_map["REQUEST_METHOD"] = (request.getRequestLine().Method);
    env_map["SCRIPT_NAME"] = cgi_path;// =>path of cgi 
    env_map["SCRIPT_FILENAME"] = cgi_path;// => path of cgi 
     env_map["PATH_INFO"] = request.getPath();//=>request path
    env_map["REQUEST_URI"] = request.getRequestLine().URI; //=>  req URI
    env_map["SERVER_NAME"] = request.getHost();
    env_map["SERVER_PORT"] = request.getPort();
    env_map["SERVER_PROTOCOL"] = request.getRequestLine().HttpVers;
    env_map["SERVER_SOFTWARE"] = "Webserver/1.1";
}
void ProcessCgi::EnvArray()
{
    size_t i = 0;
    env = new  char*[env_map.size() + 1];
    std::map<std::string, std::string>::iterator it = env_map.begin();
    
    while (it != env_map.end())
    {
        std::string value = (it->first) + "=" +(it->second);
        env[i] = new char[value.size() + 1];
        std::strcpy(env[i], value.c_str());
        i++;
        it++;
    }
    env[i] = NULL;
}
