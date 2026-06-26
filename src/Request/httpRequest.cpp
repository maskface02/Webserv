/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 10:02:50 by lasoubai          #+#    #+#             */
/*   Updated: 2026/06/26 12:43:12 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/WebServ.hpp"


// store conectionne value in the client


Request::Request(std::string& Rq)
:port(8080),content_lenght(0),connection("close"), isChunked(false), status_code(0)
{
    size_t          LineEnd = 0;
    size_t          header_end = std::string::npos;   
    
    try
    {
        LineEnd = Rq.find("\r\n");
        if (LineEnd != std::string::npos)
        {
            header_end = pars_lineRequest(Rq, LineEnd);
            if (header_end !=  std::string::npos)
                pars_headerMap(Rq, LineEnd + 2,header_end + 2);
            else    throw HttpError(400);
            if ((header_end + 4) <  Rq.length() &&  RequestLine.Method == "POST")
            {
                check_Post();
                pars_headerBody(Rq, header_end + 4); 
                std::cout<<body<<std::endl;  
            }
        }
        else   throw(HttpError(400));
    }
    catch(const HttpError& e)
    {
        status_code = e.getErrorCode();
        std::cerr<<status_code<<std::endl;
    }
        
}
   

size_t  Request::pars_lineRequest(std::string& Rq, size_t LineEnd)
{  
    std::stringstream str(Rq.substr(0, LineEnd));
    check_valid_nbr_space(Rq, LineEnd);
    str >> RequestLine.Method >> RequestLine.URI >> RequestLine.HttpVers;
    check_valid_line();
    store_path_query();
    return (Rq.find("\r\n\r\n",LineEnd + 2));
}

void Request::pars_query()
{
    size_t                      pos = 0;
    size_t                      oldpos = 0;
    size_t                      i = 0;
    std::string                 key;
    std::string                 value;
    std::vector<std::string>    pairs;
    
    while ((pos = RequestLine.Query.find("&", oldpos))!= std::string::npos)
    {
        pairs.push_back(RequestLine.Query.substr(oldpos, pos - oldpos));
        oldpos = pos + 1;
    }
    pairs.push_back(RequestLine.Query.substr(oldpos, RequestLine.Query.length() - oldpos));
    while(i < pairs.size())
    {
        pos = pairs[i].find("=");// need to handl if empty "key="" ++ if id must convert to nbr
        if (pos != std::string::npos)
        {
            key = pairs[i].substr(0, pos);
            value = pairs[i].substr(pos + 1);
            Query[key].push_back(value);
        }
        else
            throw HttpError(400);
        i++;
    }
}

void  Request::pars_headerMap(std::string& Rq, size_t HeadersSrart ,size_t HeadersEnd)
{   
    size_t lineStart = 0;
    size_t lineEnd = 0;
    size_t pos = 0;
    int countHead = 0;
    std::string emptyStr;
    std::string line;
    std::string Key;
    std::string Value;
    std::string Headers = Rq.substr(HeadersSrart,HeadersEnd - HeadersSrart);
   
    
    // Headers = lowercase(Rq,HeadersSrart,HeadersEnd);//
    while ((lineEnd = Headers.find("\r\n", lineStart)) != std::string::npos)
    { 
        line = Headers.substr(lineStart, lineEnd - lineStart);
        pos = line.find(":");//Host 127.0.0.1:8080
        if (pos != std::string::npos)
        {
            Key = line.substr(0, pos);
            
            std::stringstream s_value(line.substr(pos + 1));
            s_value >> Value;
            check_duplic(Key);
            store_variable(Key,Value);
            HeaderMap[Key] = Value;
        }
        else
            throw HttpError(400);
        lineStart = lineEnd + 2;
        countHead++;
        if (countHead > 100)
            throw HttpError(400);
    }
    check_existe("Host");
}

void Request::pars_headerBody(std::string& RqBody, size_t bodyStart)
{
    size_t pos;
    std::string Text_body = RqBody.substr(bodyStart);
    if (isChunked == false && Text_body.size() < content_lenght)
        throw(HttpError(400));
    if((pos = content_type.rfind("boundry")) != std::string::npos)
        pars_boundry(pos);
    if (isChunked)
        store_chunked_body(Text_body);
    else
        body = RqBody.substr(bodyStart, content_lenght);
}

void Request::store_chunked_body(std::string& chnk_body)
{
    int i = 0;
    size_t indx = 0;
    int Val = 0;
    size_t start = 0;
    std::stringstream str;
    while (indx < chnk_body.length())
    {
        start = indx;
        Val = 0;
        while ((indx + 1) < chnk_body.length() 
            && chnk_body[indx] != '\r' && chnk_body[indx + 1] != '\n')
            indx++;
        str << chnk_body.substr(start, indx - start );  std::cout<<str.str()<<std::endl;  
        str >>std::hex>> Val ;//str reach end of file caz we read all its bufff
        str.str("");//clear buffer
        str.clear();//re sate error flag =>remove error flag
        indx+= 2;
        if (Val == 0 || Val < 0 )
            break;
        i = 0;
        while (i < Val && (indx + 1) < chnk_body.length() && chnk_body[indx ] != '\r' 
            && chnk_body[indx + 1] != '\n')
        {
            body = body + chnk_body[indx];
            i++;
            indx++;
        }
        indx+= 2;
    }
}

// void Request:: pars_boundry(size_t& pos)
// {
    
//     size_t bound_pos = content_type.find("=",pos);
//     std::string boundry = content_type.substr(bound_pos + 1);
    
// }