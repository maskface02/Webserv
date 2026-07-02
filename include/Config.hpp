/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zatais <zatais@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/08 11:21:06 by zatais            #+#    #+#             */
/*   Updated: 2026/05/17 05:35:26 by zatais           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "WebServ.hpp"

struct RedirectConfig {
    bool                        enabled;
    int                         code;
    std::string                 url;
};

struct Location {
    std::string                             path;
    std::string                             root;
    std::vector<std::string>                index;
    bool                                    autoindex;
    std::vector<std::string>                allow_methods;
    bool                                    upload_enabled;
    std::string                             upload_store;
    std::map<std::string, std::string>      cgi;
    RedirectConfig                          redirect;
    size_t                                  client_max_body_size;
};

struct ListenConfig {
    std::string                 host;
    int                         port;
};

struct ServerConfig {
    std::vector<ListenConfig>   listens;
    std::map<int, std::string>  error_pages;
    size_t                      client_max_body_size;
    std::vector<Location>       locations;
};

class Config {
  private:
    Config&                     operator=(const Config&);
    std::vector<ServerConfig>   servers;

    int                         toInt(std::string& s, const std::string& context, int min_value, int max_value);
    bool                        isDigits(const std::string& s);
    bool                        isWhitespaceOnly(std::string& s);
    bool                        isValidMethod(std::string& method);
    bool                        toBool(std::string& s, std::string& context);
    bool                        isValidIp(std::string& ip);
    void                        appendToken(std::vector<std::string>& out, std::string& token);
    void                        parseListen(ServerConfig& server, std::vector<std::string>& values);
    void                        assignServerDirective(ServerConfig& server, std::vector<std::string>& tokens, size_t& idx);
    void                        assignLocationDirective(Location& loc, std::vector<std::string>& tokens, size_t& idx);
    size_t                      toSize(std::string& s, std::string& context);
    Location                    parseLocationBlock(std::vector<std::string>& tokens, size_t& idx);
    ServerConfig                parseServerBlock(std::vector<std::string>& tokens, size_t& idx);
    std::vector<std::string>    tokenize(const std::string& input);
    std::vector<std::string>    readAndTokenize(std::string& path);
    std::vector<std::string>    collectValues(std::vector<std::string>& tokens, size_t& idx, const std::string& key);

  public:
    Config();
    ~Config();
    Config(const Config&);
    void                        load(std::string &path);
    std::vector<ServerConfig>&   getServers();
};

#endif // !CONFIG
