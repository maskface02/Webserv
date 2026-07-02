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

ProcessCgi::ProcessCgi(Client *client, ProcessRequest &ProcessRq, Request& request)
:cgi_path(ProcessRq.getCgiPath()),_client(client)
{

    if (request.getRequestLine().Method == "POST")
        client->cgi_input_buffer = request.getBody();
    EnvMap(request,client->ip);
    EnvArray();
    connection = request.getConnection();
    client->processCgi = this;
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
    //cookies env
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


// Status: 201 Created\r\n
// Content-Type: application/json\r\n
// \r\n
// {
//     "status": "success",
//     "message": "File 'profile_pic.png' uploaded successfully.",
//     "size_bytes": 102432
// }




// HTTP/1.1 403 Forbidden
// Server: nginx/1.24.0 (Ubuntu)
// Date: Mon, 22 Jun 2026 15:52:56 GMT
// Content-Type: text/html
// Content-Length: 162
// Connection: keep-alive

// <html>
// <head><title>403 Forbidden</title></head>
// <body>
// <center><h1>403 Forbidden</h1></center>
// </body>
// </html>


 void ProcessCgi::GeneretCgiResponse()
{
    std::string body;
    std::string addHeader;
    std::string addLine;
    std::stringstream str;
    std::stringstream ErrorHead;
    std::string cgi_output = _client->cgi_output_buffer;
    ErrorHead<<"HTTP/1.1 502 Bad Gateway\r\n";
    ErrorHead << "Server: Webserver/1.1\r\n";
    ErrorHead <<"Content-Type: text/html\r\n";
    ErrorHead << "Date: " << generateHttpDate();
    ErrorHead << "Connection: "<<connection<<"\r\n";
    size_t p_body = 0;
    if ((p_body = cgi_output.find("\r\n\r\n")) == std::string::npos)
    {
        Cgi_resp = ServeStaticRq::html_Error_page(502, "Bad Gateway");
        ErrorHead << "Content-Length: " << Cgi_resp.size() << "\r\n";
        Cgi_resp = ErrorHead.str() + "\r\n "+Cgi_resp;
        return;
    }
    addHeader =  cgi_output.substr(0, p_body) + "\r\n";
    addLine = "HTTP/1.1 200 OK\r\n";
    size_t p_stat =  cgi_output.find("Status:");
    if (p_stat != std::string::npos)
    {
        size_t lineEnd = 0;
        if ((lineEnd = cgi_output.find("\r\n",p_stat)) != std::string::npos)
        {
            size_t pos_value = 0;
            if ((pos_value =  cgi_output.find(":", p_stat)) != std::string::npos)
            {
                pos_value++;
                while (pos_value < lineEnd && cgi_output[pos_value] == ' ')
                    pos_value++;
                std::string status_value = cgi_output.substr(pos_value , lineEnd - (pos_value));
                addLine = "HTTP/1.1 " + status_value + "\r\n";
                addHeader = addHeader.erase(p_stat,(lineEnd - p_stat) + 2);
            }
        }
    }
     body = cgi_output.substr(p_body + 4);
    if (!body.empty())
    {  if (cgi_output.find("Content-Type") == std::string::npos)
        {
            Cgi_resp = ServeStaticRq::html_Error_page(502, "Bad Gateway");
            ErrorHead << "Content-Length: " << Cgi_resp.size() << "\r\n";
            Cgi_resp = ErrorHead.str() + "\r\n "+ Cgi_resp;
            return;
        }
        if (cgi_output.find("Content-Length") == std::string::npos)
        {
            str << "Content-Length: " << body.size() << "\r\n"; 
            addHeader += str.str();
        }
    }
   Cgi_resp = addLine + addHeader + "\r\n" ;
   if (!body.empty())
        Cgi_resp +=body;
    _client->write_buffer = Cgi_resp;
}

char**   ProcessCgi::getEnv() const
{
    return(env);
}
std::string&     ProcessCgi::getCgiPath() 
{
    return(cgi_path);
}
