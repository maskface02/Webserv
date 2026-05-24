/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   httpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 10:02:50 by lasoubai          #+#    #+#             */
/*   Updated: 2026/05/24 23:50:09 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/httprequest.hpp"

Request::Request(std::string& Rq):port(80),content_lenght(0),connection("close"), isChunked(false)
{
    size_t          LineEnd = 0;
    size_t          header_end = std::string::npos;   
    
    LineEnd = Rq.find("\r\n");
    if (LineEnd != std::string::npos)
    {
        header_end = pars_lineRequest(Rq, LineEnd);
        if (header_end !=  std::string::npos)
            pars_headerMap(Rq, LineEnd + 2,header_end + 2);
        else    throw Request::HttpError(400,"Bad Request");
        if ((header_end + 4) <  Rq.length() &&  RequestLine.Method == "POST")
        {
            check_Post();
            pars_headerBody(Rq, header_end + 4);   
        }
    }
    else    throw Request::HttpError(400,"Bad Request");
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
        pos = pairs[i].find("=");// need to handl if empty "key=""
        if (pos != std::string::npos)
        {
            key = pairs[i].substr(0, pos);
            value = pairs[i].substr(pos + 1);
            Query[key].push_back(value);
        }
        else
            throw Request::HttpError(400,"Bad Request");
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
        pos = line.find(":");
        if (pos != std::string::npos)
        {
            Key = line.substr(0, pos);
            Value = remove_white_space(line.substr(pos + 1));
            check_duplic(Key);
            store_variable(Key,Value);
            HeaderMap[Key] = Value;
        }
        else
            throw Request::HttpError(400,"Bad request");
        lineStart = lineEnd + 2;
        countHead++;
        if (countHead > 100)
            throw Request::HttpError(400,"Bad request");
    }
    Headers.swap(emptyStr);
    check_existe("Host");
}

void Request::pars_headerBody(std::string& RqBody, size_t bodyStart)
{
    std::string emptystr;
    std::string Text_body = RqBody.substr(bodyStart);
    if (isChunked == false && (Text_body.size() < content_lenght || Text_body.size() > MAX_BODY_SIZE) )
        throw(HttpError(400,"1111Bad Request"));
    if (isChunked)
    {
        store_chunked_body(Text_body);
        if (body.size() > MAX_BODY_SIZE)
            throw(HttpError(400,"Bad Request"));
    }
    else
        body = RqBody.substr(bodyStart, content_lenght);
}

void Request::store_chunked_body(std::string& chnk_body)
{
    int INT = 0;
    int i = 0;
    size_t indx = 0;
    int HexVal = 0;
    while (indx < chnk_body.length())
    {
        INT = 0;
        while ((indx + 1) < chnk_body.length() && chnk_body[indx] != '\r' && chnk_body[indx + 1] != '\n')
        {
            HexVal = HexStr_to_Int(chnk_body[indx]);
            if (HexVal == -1)
                throw(HttpError(400,"Bad Request"));
            INT = INT * 16 +   HexVal;
            indx++;
        }
        if (INT > MAX_BODY_SIZE)
             throw(HttpError(400,"Bad Request"));
        indx+= 2;
        if (INT == 0 || INT < 0 )
            break;
        i = 0;
        while (i < INT && (indx + 1) < chnk_body.length() && chnk_body[indx ] != '\r' && chnk_body[indx + 1] != '\n')
        {
            body = body + chnk_body[indx];
            i++;
            indx++;
        }
        indx+= 2;
    }
}
