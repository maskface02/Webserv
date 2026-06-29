/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 19:54:51 by lasoubai          #+#    #+#             */
/*   Updated: 2026/06/28 18:37:20 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include "WebServ.hpp"
#include <sstream>



#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <cctype>


#define Buffer_Size 4096
#define MAX_BODY_SIZE 1048576 //1Mb default in ngx 

struct reqLine
{
    std::string Method;
    std::string URI;
    std::string Path;
    std::string Query;
    std::string HttpVers;
};


class Request
{
    private:
        std::map<std::string , std::string>                 HeaderMap;
        std::map<std::string , std::vector<std::string> >   Query;
        reqLine                                             RequestLine;
        int                                                 port;
        std::string                                         Host;
        size_t                                              content_lenght;
        std::string                                         body;
        std::string                                         connection;
        bool                                                isChunked;
        std::string                                         content_type;
        int                                                 status_code;
        std::map<std::string, std::string>                  boundry_map;
        // Request();
    public:
        bool                                                is_boundry;
        Request(Client* client, std::string& header);
        // Request(Request& other);
        // Request& operator=(Request& other);
        // ~Request(); 
    //geters
    
        std::map<std::string , std::string>                 getHeaderMap() const ;
        reqLine                                             getRequestLine() const;
        int                                                 getPort() const;
        std::string                                         getHost() const;
        std::string                                         getBody() const;
        std::string                                         getConnection() const;
        std::string                                         getContentType()const;
        int                                                 getContentLenght() const;
        bool                                                getIsChunked() const;
        std::map<std::string , std::vector<std::string> >   getQuery() const;
        int                                                 getStatusCode() const;
        std::string                                         getPath() const;
        std::map<std::string, std::string>&                 getBoundryMap() ;
    //pars
        size_t                                              pars_lineRequest(std::string  &header, size_t LineEnd);
        void                                                pars_Headers(std::string  &Map, size_t HeadersSrart,size_t HeadersEnd);
        void                                                pars_Body(std::string &body, size_t HeaderStart);
        void                                                pars_query();
        void                                                pars_boundry(size_t& pos);
        void                                                pars_chunked_body(std::string& chunck_body);
       std::vector<std::string>                             split_boundary_part(std::string& boundary);
        std::string                                         find_file_name(std::string& part);
        std::string                                         find_boundry_body(std::string& part);
        //stor varible
       
        void                                                store_path_query();
        void                                                store_variable(std::string& key, std::string& value);
        void                                                store_cont_lenght(const std::string& lenght);
        void                                                store_host_port(std::string &str);
    //check
        void                                                check_valid_nbr_space(std::string  &Rqline, size_t EndLine);
        void                                                check_valid_line();
        void                                                check_valid_Method();
        void                                                check_valid_HttpV();
        void                                                check_duplic(std::string& key);
        void                                                check_existe(std::string key);
        void                                                check_Post();
        // void                                                check_valid_URL();//TO DO
    // util
        std::string                                         remove_white_space( std::string str);
        int                                                 strIsDigits(const std::string& str);
        int                                                 HexStr_to_Int(const char c);
    ///////////////////////////////////////////////////////////
       
};

class HttpError : public std::exception
{
    private:
        int         code;
        HttpError();
    
    public:
     
        HttpError(int statusCode);
        int getErrorCode() const ;//not implemented
        // virtual const char * what() const throw();
};

#endif
