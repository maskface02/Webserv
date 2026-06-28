/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httprequest_utl.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 19:54:39 by lasoubai          #+#    #+#             */
/*   Updated: 2026/06/28 03:34:38 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/WebServ.hpp"

////////////////////////////////////////////////////////           Request line         ///////////////////////////////

void Request::check_valid_nbr_space(std::string  &Rqline, size_t EndLine)
{
  
    size_t         i = 0;
    size_t         countSpace = 0;
    std::string    line = Rqline.substr(0,EndLine);
    
    while (i < EndLine)
    {
        
        if ((line[i] >= 0 && line[i] <= 31) || line[i] == 127)
            throw(HttpError(400));
        if (countSpace > 2)
            throw(HttpError(400));
        if (line[i] == ' ')
        {
            if ((i + 1) < EndLine && line[i + 1] == ' ')
                throw(HttpError(400));
            countSpace++;
        }
        i++;
    }
    if (countSpace != 2)
       throw(HttpError(400));

}

void Request::check_valid_line()
{
    check_valid_Method();
    // check_valid_URL();//Not implemented yet 
    check_valid_HttpV();
}

void Request::check_valid_Method()
{   
    if (RequestLine.Method == "OPTIONS" || RequestLine.Method == "HEAD" || RequestLine.Method == "PUT" 
            || RequestLine.Method == "TRACE"  ||  RequestLine.Method == "CONNECT" )
    {
          throw(HttpError(501));
    }
    if (RequestLine.Method != "GET" && RequestLine.Method != "POST" && RequestLine.Method != "DELETE")
        throw(HttpError(400));
}

// void Request::check_valid_URL()
// {
//      * Checks if character is allowed to be in a URI
//  * Characters allowed as specifed in RFC:
//    Alphanumeric: A-Z a-z 0-9
//    Unreserved: - _ . ~
//    Reserved:  * ' ( ) ; : @ & = + $ , / ? % # [ ]
// }

void Request::check_valid_HttpV()
{ 
    if (RequestLine.HttpVers != "HTTP/1.1" && RequestLine.HttpVers !=  "HTTP/1.0")
        throw(HttpError(505));
}

void Request::store_path_query()
{
    size_t sp = 0;
    
    if ((sp = RequestLine.URI.find("?")) != std::string::npos)
    {
        RequestLine.Path = RequestLine.URI.substr(0, sp);
        RequestLine.Query = RequestLine.URI.substr(sp + 1);
        pars_query();
    }
    else
        RequestLine.Path =  RequestLine.URI;
      
}
////////////////////////////////////////////////////      Headers        ////////////////////

std::string Request::remove_white_space(std::string str)
{
    size_t  i = 0;
    
    while(i < str.size() && ((str[i] <=31 && str[i] >= 0) || str[i] == 127) )
        i++;
    return(str.substr(i));
}

void Request::check_duplic(std::string& key)
{
    if(!HeaderMap.empty())
    {
        std::map<std::string, std::string>::iterator it = HeaderMap.find(key.c_str());
        if (it != HeaderMap.end())
            throw HttpError(400);
       
    }
   
}

void Request::check_existe(std::string key)
{
    if(!HeaderMap.empty())
    {
        std::map<std::string, std::string>::iterator it = HeaderMap.find(key.c_str());
        if (it == HeaderMap.end())
            throw HttpError(400);
    }
   
}

void Request::store_variable(std::string& key, std::string& value)
{// lower case
    if (key == "Connection")
        connection = value;
    if (key == "Content-Length")
        store_cont_lenght(value);
    if (key == "Transfer-Encoding" &&  value == "chunked" )
        isChunked = true;
    if (key == "Host")
        store_host_port(value);
    if (key == "Content-Type")
        content_type = value;
}

void Request::store_cont_lenght(const std::string &lenght)
{   
    if (lenght.empty() || !strIsDigits(lenght))
        throw(HttpError(400));
    content_lenght = std::atoi(lenght.c_str());
    // if (content_lenght > MAX_BODY_SIZE)
    //     throw(HttpError(413,"Request Entity Too Large")); => check in in process
}

void Request::store_host_port(std::string &str)
{
    size_t      p = 0;
    std::string strNbr;

    p = str.find(":");
    if (p != std::string::npos)
    {
        Host = str.substr(0, p);
        strNbr = str.substr(p + 1);
        if (!strNbr.empty() && strIsDigits(strNbr))
            port = std::atoi(strNbr.c_str());
        else
            throw HttpError(400);
    }
    // else    throw HttpError(400);
    Host = str;
}

void Request::check_Post()
{
    std::map<std::string, std::string>::iterator it1 = HeaderMap.find("Content-Length");
    std::map<std::string, std::string>::iterator it2 = HeaderMap.find("Content-Type");
    std::map<std::string, std::string>::iterator it3 = HeaderMap.find("Transfer-Encoding");
    if (it1 != HeaderMap.end() && it3 != HeaderMap.end())
        throw HttpError(400);
    if ( it1 == HeaderMap.end() &&  it3 == HeaderMap.end())
        throw HttpError(400);
    if (it2 ==  HeaderMap.end())
        throw HttpError(400);
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

/// exception

HttpError::HttpError(int ErrorCode): code(ErrorCode){}

int HttpError::getErrorCode() const
{
    return(code);
}


/////// Get

std::map<std::string , std::string>  Request::getHeaderMap() const
{
    return(HeaderMap);
}
reqLine Request::getRequestLine() const
{
    return(RequestLine);
}
int  Request::getPort() const
{
    return(port);
}
std::string    Request::getHost() const
{
    return (Host);
}
std::string    Request::getBody() const
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
std::map<std::string , std::vector<std::string> >   Request::getQuery() const
{
    return (Query);
}

int  Request::getStatusCode() const 
{
    return (status_code) ;
}

std::string Request::getPath() const 
{
    return(RequestLine.Path);
}

 std::map<std::string, std::string>&       Request::getBoundryMap() 
 {
    return(boundry_map);
 }
