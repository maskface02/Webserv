/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httprequest_utl.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 19:54:39 by lasoubai          #+#    #+#             */
/*   Updated: 2026/07/30 12:44:06 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/WebServ.hpp"

void Request::check_valid_nbr_space(std::string  &Rqline, size_t EndLine)
{
    size_t         i = 0;
    size_t         countSpace = 0;
    std::string    line = Rqline.substr(0,EndLine);
    
    while (i < EndLine)
    {
        
        if ((line[i] >= 0 && line[i] <= 31) || line[i] == 127)
            throw(HttpStatus(BAD_REQUEST));
        if (countSpace > 2)
            throw(HttpStatus(BAD_REQUEST));
        if (line[i] == ' ')
        {
            if ((i + 1) < EndLine && line[i + 1] == ' ')
                throw(HttpStatus(BAD_REQUEST));
            countSpace++;
        }
        i++;
    }
    if (countSpace != 2)
       throw(HttpStatus(BAD_REQUEST));
}

void Request::check_valid_line()
{
    check_valid_Method();
    check_valid_URI();
    check_valid_HttpV();
}

void Request::check_valid_Method()
{   
  if (requestLine.Method != "GET" && requestLine.Method != "POST" && requestLine.Method != "DELETE")
    throw(HttpStatus( METHOD_NOT_ALLOWED));
}

void Request::check_valid_URI()
{

    if (requestLine.URI[0] != '/' || requestLine.URI.length() > 8000)
        throw HttpStatus(URI_TOO_LONG);
    requestLine.URI = normalize_URI(requestLine.URI);
    is_valid_char(requestLine.URI);
}

std::string   Request::normalize_URI(std::string& url)
{
    std::string new_URI;
    size_t i = 0;

    while(i  < url.size())
    {
        if (i + 1 < url.size() && url[i] == '.' && url[i + 1] == '/')
        {
            i += 2;
            continue;
        }
        while ( url.size() > i + 2 && url[i] == '.' 
            && url[i + 1] == '.' && url[i + 2] == '/')
        {
            i += 3;
        }
        if ( i < url.size())
            new_URI += url[i];
        i++;
    }
    return(new_URI);
}

void    Request::is_valid_char(std::string& URI)
{
    size_t i = 0;
    while (i < URI.size())
    {
        if (!isalpha(URI[i]) && !isdigit(URI[i]) && !is_reserved(URI[i]))
            throw HttpStatus(BAD_REQUEST);
        i++;
    }
}

bool Request::is_reserved(char c)
{
    std::string reserved_chars = "_-.:/?#[]@!$&'()*+,;=%~"; 
    size_t j = 0;
    while (j < reserved_chars.size())
    {
        if (c == reserved_chars[j])
          return(true);  
        j++;
    }
   return (false);
}


void Request::check_valid_HttpV()
{ 
    if (requestLine.HttpVers != "HTTP/1.1" && requestLine.HttpVers !=  "HTTP/1.0")
        throw(HttpStatus(HTTP_VERSION_NOT_SUPPORTED));
}

void Request::store_path_query()
{
    size_t sp = 0;
    if ((sp = requestLine.URI.find("?")) != std::string::npos)
    {
        requestLine.Path = requestLine.URI.substr(0, sp);
        requestLine.Query = requestLine.URI.substr(sp + 1);
    }
    else
        requestLine.Path =  requestLine.URI;
}


std::string Request::remove_white_space(std::string str)
{
    size_t  i = 0;
    while(i < str.size() && ((str[i] <=31 && str[i] >= 0) || str[i] == 127) )
        i++;
    return(str.substr(i));
}

void Request::check_duplic(std::string& key)
{
  
    std::map<std::string, std::string>::iterator it = headerMap.find(key.c_str());
    if (it != headerMap.end())
        throw HttpStatus(BAD_REQUEST);
}

void Request::check_existe(std::string key)
{
  
    std::map<std::string, std::string>::iterator it = headerMap.find(key.c_str());
    if (it == headerMap.end())
        throw HttpStatus(BAD_REQUEST);
}

void Request::store_variable(std::string& key, std::string& value)
{
    std::string key_lower = lowerString(key);
   
    if (key_lower == "connection")
        connection = value;
    if (key_lower == "content-length")
        store_cont_lenght(value);
    if (key_lower == "transfer-encoding" &&  value == "chunked" )
        isChunked = true;
    if (key_lower == "host")
        store_host_port(value);
    if (key_lower == "content-type")
        content_type = value;
    if (key_lower == "cookie")
         cookies_header = value;
}

void Request::store_cont_lenght(const std::string &lenght)
{   
    if (lenght.empty() || !strIsDigits(lenght))
        throw(HttpStatus(BAD_REQUEST));
    content_lenght = std::atoi(lenght.c_str());
}

void Request::store_host_port(std::string &str)
{
    size_t      p = 0;
    std::string strNbr;

    p = str.find(":");
    if (p != std::string::npos)
    {
        host = str.substr(0, p);
        strNbr = str.substr(p + 1);
        if (!strNbr.empty() && strIsDigits(strNbr))
            port = std::atoi(strNbr.c_str());
        else
            throw HttpStatus(BAD_REQUEST);
    }
    else    throw HttpStatus(BAD_REQUEST);
}
 
void        Request::define_session_id()
{
    if (!cookies_header.empty())
    {
        size_t id_pos = 0;
        id_pos = cookies_header.find("session_id=");
        if (id_pos != std::string::npos)
        {
            id_pos += 11;
            if (id_pos + 1 > cookies_header.size())
            {
                client->session_id = generateSessionId(); 
                client->is_new = true;   
                return;
            }
            size_t id_end = 0;
            id_end  = cookies_header.find(";", id_pos);
            if (id_end  != std::string::npos)
            {
                client->session_id = cookies_header.substr(id_pos, id_end - id_pos);
                client->is_new = false;
            }
            else
            {
                client->session_id =  cookies_header.substr(id_pos);
                client->is_new = false;
            }
        }
        else 
        {
            client->is_new = true;
            client->session_id = generateSessionId(); 
        } 
    }
    else 
    {
        client->is_new = true;
        client->session_id = generateSessionId();
    }
}


void Request::check_Post()
{
    std::map<std::string, std::string>::iterator it1 = headerMap.find("Content-Length");
    std::map<std::string, std::string>::iterator it2 = headerMap.find("Content-Type");
    std::map<std::string, std::string>::iterator it3 = headerMap.find("Transfer-Encoding");
    if (it1 != headerMap.end() && it3 != headerMap.end())
        throw HttpStatus(BAD_REQUEST);
    if ( it1 == headerMap.end() &&  it3 == headerMap.end())
        throw HttpStatus(BAD_REQUEST);
    if (it2 ==  headerMap.end())
        throw HttpStatus(BAD_REQUEST);
}

int Request::strIsDigits(const std::string& str)
{
    size_t  i = 0;

    while (i < str.size())
    {
        if (!std::isdigit(str[i]))
            return (0);
        i++;
    }
    return(1);
}

HttpStatus::HttpStatus(int ErrorCode): code(ErrorCode){}

int HttpStatus::getErrorCode()
{
    return(code);
}

std::map<std::string , std::string>  Request::getHeaderMap() const
{
    return(headerMap);
}

reqLine Request::getRequestLine() const
{
    return(requestLine);
}
int  Request::getPort() const
{
    return(port);
}
std::string    Request::getHost() const
{
    return (host);
}
std::string&   Request::getBody()
{
    return(body);
}
std::string   Request::getConnection() const
{
    return (connection);
}
std::string   Request::getContentType()const
{
    return (content_type);
}
int   Request::getContentLenght() const
{
    return(content_lenght);
}
bool  Request::getIsChunked() const
{
    return (isChunked);
}

int  Request::getStatusCode() 
{
    return (status_code) ;
}

std::string Request::getPath() const 
{
    return(requestLine.Path);
}

 std::map<std::string, std::string>&       Request::getBoundryMap() 
 {
    return(boundry_map);
 }

void Request::setCookeisHeader(std::string str)
{
     cookies_header = str;
}
