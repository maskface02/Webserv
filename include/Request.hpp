/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 19:54:51 by lasoubai          #+#    #+#             */
/*   Updated: 2026/07/30 10:35:59 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include "WebServ.hpp"


#define Buffer_Size 4096
#define MAX_BODY_SIZE 1048576 

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
        std::map<std::string , std::string>                 headerMap;
        reqLine                                             requestLine;
        int                                                 port;
        std::string                                         host;
        size_t                                              content_lenght;
        std::string                                         body;
        std::string                                         connection;
        bool                                                isChunked;
        std::string                                         content_type;
        int                                                 status_code;
        std::map<std::string, std::string>                  boundry_map;
        std::string                                         cookies_header;
        Client*                                             client;
        
        Request();
        Request(Request& other);
        Request& operator=(Request& other);
    public:
       
        Request(Client* client, std::string& rq, size_t request_size);
        ~Request(); 

        bool                                                is_boundry;
        std::map<std::string , std::string>                 getHeaderMap() const ;
        reqLine                                             getRequestLine() const;
        int                                                 getPort() const;
        std::string                                         getHost() const;
        std::string&                                        getBody();
        std::string                                         getConnection() const;
        std::string                                         getContentType()const;
        int                                                 getContentLenght() const;
        bool                                                getIsChunked() const;
        int                                                 getStatusCode() ;
        std::string                                         getPath() const;
        std::map<std::string, std::string>&                 getBoundryMap() ;
        void                                                setCookeisHeader(std::string str);
        size_t                                              pars_lineRequest(std::string  &header, size_t LineEnd);
        void                                                pars_Headers(std::string  &Map, size_t HeadersSrart,size_t HeadersEnd);
        void                                                pars_Body(std::string &body, size_t HeaderStart,size_t req_size);
        void                                                pars_boundry(size_t& pos);
        void                                                pars_chunked_body(const std::string& chunck_body,size_t bodyStart, size_t req_size);
        std::vector<std::string>                            split_boundary_part(std::string& boundary);
        std::string                                         find_file_name(std::string& part);
        std::string                                         find_boundry_body(std::string& part);
        void                                                store_path_query();
        void                                                store_variable(std::string& key, std::string& value);
        void                                                store_cont_lenght(const std::string& lenght);
        void                                                store_host_port(std::string &str);
        void                                                define_session_id();
        void                                                check_valid_nbr_space(std::string  &Rqline, size_t EndLine);
        void                                                check_valid_line();
        void                                                check_valid_Method();
        void                                                check_valid_HttpV();
        void                                                check_duplic(std::string& key);
        void                                                check_existe(std::string key);
        void                                                check_Post();
        void                                                check_valid_URI();
        void                                                is_valid_char (std::string& URI);
        bool                                                is_reserved(char c);
        std::string                                         normalize_URI(std::string& url);
        std::string                                         remove_white_space( std::string str);
        int                                                 strIsDigits(const std::string& str);
};
        std::string                                         generateSessionId();

class HttpStatus : public std::exception
{
    private:
        int         code;
        HttpStatus();
    
    public:
     
        HttpStatus(int statusCode);
        int getErrorCode();
};

#endif
