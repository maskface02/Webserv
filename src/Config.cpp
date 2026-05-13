/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zatais <zatais@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/08 21:52:23 by zatais            #+#    #+#             */
/*   Updated: 2026/05/13 02:05:16 by zatais           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/Config.hpp"
#include <cerrno>

Config::Config() {}

Config::~Config() {}

Config::Config(const Config& conf){
  servers = conf.servers;
}

/******************************************************************************/
bool Config::isWhitespaceOnly(std::string& s) {
  for (size_t i = 0; i < s.size(); ++i)
    if (!isspace(s[i]))
      return false;
  return true;
}

void Config::appendToken(std::vector<std::string>& out, std::string& token) {
  if (!token.empty())
    out.push_back(token);
}

std::vector<std::string> Config::tokenize(const std::string& input) {
  std::vector<std::string> tokens;
  std::string current;
  bool in_quote = false;
  char quote_char = 0;

  for (size_t i = 0; i < input.size(); ++i) {
    char c = input[i];
    if (in_quote) {
      if (c == quote_char)
        in_quote = false;
      else
        current += c;
      continue;
    }
    if (c == '\'' || c == '"') {
      in_quote = true;
      quote_char = c;
      continue;
    }
    if (isspace(c)) {
      appendToken(tokens, current);
      current.clear();
      continue;
    }
    if (c == '{' || c == '}' || c == ';') {
      appendToken(tokens, current);
      current.clear();
      tokens.push_back(std::string(1, c));
      continue;
    }
    current += c;
  }
  appendToken(tokens, current);
  if (in_quote)
    throw std::runtime_error("config: unclosed quote in config");
  return tokens;
}

bool Config::isDigits(const std::string& s) {
  for (size_t i = 0; i < s.size(); ++i)
    if (!isdigit(s[i]))
      return false;
  return true;
}

int Config::toInt(std::string& s, const std::string& context, int min_value, int max_value) {
  if (!isDigits(s))
    throw std::runtime_error("config: invalid number '" + s + "' for " + context);
  long value = atol(s.c_str());
  if (value > max_value || value < min_value)
      throw std::runtime_error("config: number out of range '" + s + "' for " + context);
  return static_cast<int>(value);
}

size_t Config::toSize(std::string& s, std::string& context) {
  size_t i = 0;
  while (i < s.size() && isdigit(s[i]))
    ++i;
  if (!i)
    throw std::runtime_error("config: invalid size value '" + s + "' for " + context);
  if (!s[i])
      throw std::runtime_error("config: size suffix not found in '" + s + "' for " + context);

  std::string num = s.substr(0, i);
  long value = strtol(num.c_str(), NULL, 0);
  if (errno == ERANGE || value > 5000)
      throw std::runtime_error("config: size value too large for " + context);
  if (i < s.size()) {
    char suffix = static_cast<char>(toupper(s[i]));
    if (suffix == 'K')
      value *= 1024;
    else if (suffix == 'M')
      value *= 1024 * 1024;
    else if (suffix == 'G')
      value *= 1024 * 1024 * 1024;
    else
      throw std::runtime_error("config: invalid size suffix in '" + s + "' for " + context);
    if (i + 1 != s.size())
      throw std::runtime_error("config: invalid size value '" + s + "' for " + context);
  }
  return static_cast<size_t>(value);
}

bool Config::toBool(std::string& s, std::string& context) {
  if (s == "on" || s == "true")
    return true;
  if (s == "off" || s == "false")
    return false;
  throw std::runtime_error("config: invalid boolean '" + s + "' for " + context);
}

std::vector<std::string> Config::collectValues(std::vector<std::string>& tokens, size_t& idx, const std::string& key) {
    std::vector<std::string> values;
    while (idx < tokens.size() && tokens[idx] != ";")
        values.push_back(tokens[idx++]);
    if (values.empty())
        throw std::runtime_error("config: missing value for directive '" + key + "'");
    return values;
}

bool Config::isValidIp(std::string& ip) {
  std::vector<std::string> parts;
  std::string current;
  for (size_t j = 0; j < ip.size(); ++j) {
    if (ip[j] == '.') {
      if (current.empty())
        return false;
      parts.push_back(current);
      current.clear();
    }
    else if (isdigit(ip[j]))
      current += ip[j];
    else
      return false;
  }
  if (current.empty())
    return false;
  parts.push_back(current);
  if (parts.size() != 4)
    return false;
  for (size_t j = 0; j < 4; ++j) {
    long val = atol(parts[j].c_str());
    if (val < 0 || val > 255)
      return false;
  }
  return true;
}

void Config::parseListen(ServerConfig& server, std::vector<std::string>& values) {
  size_t i = 0;
  while (i < values.size()) {
    ListenConfig listen;
    listen.host = "0.0.0.0";
    listen.port = 80;

    std::string token = values[i];
    std::string host;
    std::string port_str;
    size_t colon = token.find(':');
    if (colon == std::string::npos) {
      if (isDigits(token)) {
        port_str = token;
        ++i;
      }
      else {
        if (token != "localhost")
          throw std::runtime_error("config: invalid host name '" + token + "'");
        host = "127.0.0.1";
        if (i + 1 < values.size() && isDigits(values[i + 1])) {
          port_str = values[i + 1];
          i += 2;
        }
        else
            ++i;
      }
    }
    else {
      host = token.substr(0, colon);
      port_str = token.substr(colon + 1);
      ++i;
      if (host.empty() || port_str.empty())
        throw std::runtime_error("config: listen has empty host or port");
    }

    if (host == "localhost")
        host = "127.0.0.1";
    if (!host.empty()) {
      if (!isValidIp(host))
        throw std::runtime_error("config: listen has invalid IP '" + host + "'");
      listen.host = host;
    }
    if (!port_str.empty())
      listen.port = toInt(port_str, "listen", 1, 65535);

    for (size_t j = 0; j < server.listens.size(); ++j)
      if (server.listens[j].host == listen.host && server.listens[j].port == listen.port)
        throw std::runtime_error("config: listen duplicate host:port " + listen.host + ":" + port_str);
    server.listens.push_back(listen);
  }
}

void Config::assignServerDirective(ServerConfig& server, std::vector<std::string>& tokens, size_t& idx) {
  std::string key = tokens[idx++];
  if (idx == tokens.size())
    throw std::runtime_error("config: missing value for server directive '" + key + "'");

  if (key == "listen") {
    std::vector<std::string> values = collectValues(tokens, idx, key);
    parseListen(server, values);
  }
  else if (key == "error_page") {
    std::vector<std::string> values = collectValues(tokens, idx, key);
    if (values.size() < 2)
      throw std::runtime_error("config: error_page requires code and path");
    std::string path = values[values.size() - 1];
    for (size_t i = 0; i + 1 < values.size(); ++i) {
      int code = toInt(values[i], "error_page", 100, 999);
      server.error_pages[code] = path;
    }
  }
  else if (key == "client_max_body_size")
    server.client_max_body_size = toSize(tokens[idx++], key);
  else
    throw std::runtime_error("config: unknown server directive '" + key + "'");

  if (idx == tokens.size() || tokens[idx] != ";")
    throw std::runtime_error("config: missing ';' after server directive '" + key + "'");
  ++idx;
}

void Config::assignLocationDirective(Location& loc, std::vector<std::string>& tokens, size_t& idx) {
  std::string key = tokens[idx++];
  if (idx == tokens.size())
    throw std::runtime_error("config: missing value for location directive '" + key + "'");

  if (key == "root")
    loc.root = tokens[idx++];
  else if (key == "index")
    loc.index = collectValues(tokens, idx, key);
  else if (key == "autoindex")
    loc.autoindex = toBool(tokens[idx++], key);
  else if (key == "allow_methods") {
    loc.allow_methods = collectValues(tokens, idx, key);
    for (size_t i = 0; i < loc.allow_methods.size(); ++i)
      if (!isValidMethod(loc.allow_methods[i]))
        throw std::runtime_error("config: invalid method '" + loc.allow_methods[i] + "'");
  }
  else if (key == "upload_store") {
    loc.upload_enabled = true;
    loc.upload_store = tokens[idx++];
  }
  else if (key == "cgi") {
    std::vector<std::string> values = collectValues(tokens, idx, key);
    if (values.size() != 2)
      throw std::runtime_error("config: cgi requires extension and path");
    loc.cgi[values[0]] = values[1];
  }
  else if (key == "return") {
    std::vector<std::string> values = collectValues(tokens, idx, key);
    if (values.size() == 1) {
      if (isDigits(values[0]))
        throw std::runtime_error("config: return requires url after status code");
      loc.redirect.enabled = true;
      loc.redirect.code = 302;
      loc.redirect.url = values[0];
    }
    else if (values.size() == 2) {
      loc.redirect.enabled = true;
      loc.redirect.code = toInt(values[0], key, 100, 999);
      loc.redirect.url = values[1];
    }
    else
      throw std::runtime_error("config: return requires code and url or just url");
  }
  else if (key == "client_max_body_size")
    loc.client_max_body_size = toSize(tokens[idx++], key);
  else
    throw std::runtime_error("config: unknown location directive '" + key + "'");
  if (idx == tokens.size() || tokens[idx] != ";")
    throw std::runtime_error("config: missing ';' after location directive '" + key + "'");
  ++idx;
}

Location Config::parseLocationBlock(std::vector<std::string>& tokens, size_t& idx) {
  ++idx;
  if (idx == tokens.size() || tokens[idx][0] != '/')
    throw std::runtime_error("config: expected path after location");

  Location loc;
  loc.autoindex = false;
  loc.upload_enabled = false;
  loc.redirect.code = 0;
  loc.path = tokens[idx++];
  loc.client_max_body_size = 0;
  loc.redirect.enabled = false;

  if (idx == tokens.size() || tokens[idx] != "{")
    throw std::runtime_error("config: expected '{' after location path");
  ++idx;

  while (idx < tokens.size() && tokens[idx] != "}")
    assignLocationDirective(loc, tokens, idx);
  if (idx == tokens.size() || tokens[idx] != "}")
    throw std::runtime_error("config: missing '}' after location block");
  ++idx;
  return loc;
}

ServerConfig Config::parseServerBlock(std::vector<std::string>& tokens, size_t& idx) {
  ++idx;
  if (idx == tokens.size() || tokens[idx] != "{")
      throw std::runtime_error("config: expected '{' after server");
  ++idx;

  ServerConfig server;
  server.client_max_body_size = 0;
  bool hasDirective = false;

  while (idx < tokens.size() && tokens[idx] != "}") {
    if (tokens[idx] == "location")
      server.locations.push_back(parseLocationBlock(tokens, idx));
    else {
      assignServerDirective(server, tokens, idx);
      hasDirective = true;
    }
  }
  if (idx == tokens.size() || tokens[idx] != "}")
      throw std::runtime_error("config: missing '}' after server block");
  if (server.locations.empty() && !hasDirective)
      throw std::runtime_error("config: empty server block");
  ++idx;
  return server;
}

std::vector<std::string> Config::readAndTokenize(std::string& path) {
  std::stringstream all;
  std::string line;
  std::ifstream file(path.c_str());

  if (!file.is_open())
    throw std::runtime_error("config: failed to open file '" + path + "'");

  while (std::getline(file, line)) {
    std::string::size_type comment_pos = line.find('#');
    if (comment_pos != std::string::npos)
      line.resize(comment_pos);
    if (isWhitespaceOnly(line))
      continue;
    all << line << '\n';
  }
  return tokenize(all.str());
}


void Config::load(std::string &path) {
  std::vector<std::string> tokens = readAndTokenize(path);
  size_t idx = 0;
  while (idx < tokens.size()) {
    if (tokens[idx] == "server")
      servers.push_back(parseServerBlock(tokens, idx));
    else
      throw std::runtime_error("config: unexpected token '" + tokens[idx] + "'");
  }
  if (servers.empty())
    throw std::runtime_error("config: no server blocks found");

  for (size_t i = 0; i < servers.size(); ++i) {
    if (servers[i].listens.empty()) {
       ListenConfig listen;
       listen.host = "0.0.0.0";
       listen.port = 80;
       servers[i].listens.push_back(listen);
    }
  }
}

std::vector<ServerConfig> Config::getServers() const {
    return servers;
}

bool Config::isValidMethod(std::string& method) {return method == "GET" || method == "POST" || method == "DELETE";}
