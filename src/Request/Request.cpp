/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 10:02:50 by lasoubai          #+#    #+#             */
/*   Updated: 2026/07/26 16:53:19 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/WebServ.hpp"



Request::Request(Client* _client, std::string& Rq ,size_t request_size)
:port(8080),content_lenght(0),connection("close"), isChunked(false), status_code(0)
,client(_client),is_boundry(false)

{
    size_t          LineEnd = 0;
    size_t          header_end = std::string::npos;   

    try
    {
       
        LineEnd = Rq.find("\r\n");
        if (LineEnd != std::string::npos && LineEnd < request_size)
        {
            header_end = pars_lineRequest(Rq, LineEnd);
            if (header_end !=  std::string::npos && header_end < request_size)
                pars_Headers(Rq, LineEnd + 2,header_end + 2);
            else    throw HttpError(BAD_REQUEST);
            if (connection == "keep-alive")
                client->keep_alive = true;//check init
            if ((header_end + 4) <  request_size &&  RequestLine.Method == "POST")
            {
                check_Post();
                pars_Body(Rq, header_end + 4,request_size);
            }
            // if (RequestLine.Method == "POST" && body.empty() && !isChunked)
            //     throw(HttpError(BAD_REQUEST));
            // ==> check content lenth => check if it passs in cgi
        }
        else   throw(HttpError(BAD_REQUEST));  
    }
    catch( HttpError& e)
    {
        status_code = e.getErrorCode();
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


void  Request::pars_Headers(std::string& Rq, size_t HeadersSrart ,size_t HeadersEnd)
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
    if (Headers.empty())
        throw HttpError(BAD_REQUEST);
    while ((lineEnd = Headers.find("\r\n", lineStart)) != std::string::npos)
    { 
        line = Headers.substr(lineStart, lineEnd - lineStart);
        pos = line.find(":");
        if (pos != std::string::npos)
        { 
            Key = line.substr(0, pos);
            pos++;
            while (line[pos] == ' ')
                pos++;
            Value = line.substr(pos);
            check_duplic(Key);
            store_variable(Key,Value);
            HeaderMap[Key] = Value;
            std::map<std::string,std::string> ::iterator it;
            it = HeaderMap.begin();
        }
        else
            throw HttpError(BAD_REQUEST);
        lineStart = lineEnd + 2;
        countHead++;
        if (countHead > 100)
            throw HttpError(BAD_REQUEST);
    }
    check_existe("Host");
    define_session_id();
}



void Request::pars_Body(std::string& RqBody, size_t bodyStart,size_t req_size)
{
    size_t pos = 0; 

    if (isChunked == false && (req_size - bodyStart) < content_lenght)
        throw(HttpError(BAD_REQUEST));     
    if (isChunked)
    {
        pars_chunked_body(RqBody,bodyStart,req_size);
        if (body.size() < content_lenght)
            throw(HttpError(BAD_REQUEST));
    }
      
    else
        body = RqBody.substr(bodyStart, content_lenght);
    
    if ((pos = content_type.rfind("boundary")) != std::string::npos)
    {
      pars_boundry(pos);
      is_boundry = true; 
    }   
}

void Request::pars_chunked_body(const std::string& chnk_body,size_t body_start, size_t req_size)
{
    int i = 0;
    size_t indx = body_start;
    int Val = 0;
    size_t start = 0;
    std::stringstream str;
    while (indx < req_size)
    {
        start = indx;
        Val = 0;
        while (indx + 1 < req_size && chnk_body[indx] != '\r' && chnk_body[indx + 1] != '\n')
            indx++;
        str << chnk_body.substr(start, indx - start ); 
        str >>std::hex >> Val ;
        if (Val == 0 )
            break;
        str.str("");
        str.clear();
        if (indx + 1 < req_size || (chnk_body[indx] != '\r' && chnk_body[indx + 1] != '\n'))
            break;
        indx+= 2;
        i = 0;
        while (i < Val && indx < req_size )
        {
            body.append(1,chnk_body[indx]);
            i++;
            indx++;
        }
        if (indx + 1 < req_size || (chnk_body[indx] != '\r' && chnk_body[indx + 1] != '\n'))
            break;
        indx+= 2;
        if (body.size() > content_lenght)
            break;
    }
    // std::cout<<"\n"<<body<<"\n";
}


void Request:: pars_boundry(size_t& pos)
{
    size_t bound_pos = content_type.find("=",pos);
    size_t bound_end = content_type.find("\r\n",bound_pos);
    std::string boundry = "--" + content_type.substr(bound_pos + 1, bound_end - (bound_pos + 1));
    std::vector<std::string> parts;
    parts = split_boundary_part(boundry);
    size_t i = 0;
    while (i < parts.size())
    {
        std::string file_name = find_file_name(parts[i]);
        if(!file_name.empty())
        {
            std::string boundry_body = find_boundry_body(parts[i]);
            boundry_map[file_name] = boundry_body;
        }
        i++;
    }
}

std::vector<std::string> Request:: split_boundary_part(std::string& boundary)
{
    std::vector<std::string> split_part;
    size_t start = body.find(boundary) + boundary.length();
    std::string last_boundry = boundary + "--";
    size_t end = 0;
    while((end = body.find(boundary,start)) != std::string::npos)
    {
       split_part.push_back(body.substr(start,end - start));
        start = end + boundary.length();
    }
    return(split_part);
}

std::string Request::find_file_name(std::string& part)
{
    size_t pos_file = part.find("filename");
    if (pos_file == std::string::npos)
        return("");
    size_t pos_name =  part.find("=",pos_file);
    if(pos_name == std::string::npos)
        return("");
    size_t endLine = part.find("\r\n",pos_name) ;
    if (endLine == std::string::npos)
        return("");
    if (endLine > 0)
        endLine -= 1;
    if (pos_name + 2 >= endLine)
        return("");
    return (part.substr(pos_name + 2,endLine - (pos_name + 2)));
}

std::string Request::find_boundry_body(std::string& part)
{
    size_t body_start = part.find("\r\n\r\n");
    if (body_start == std::string::npos)
          throw(HttpError(BAD_REQUEST));
    return(part.substr(body_start + 4));
}

