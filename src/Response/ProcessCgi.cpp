/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ProcessCgi.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/17 12:24:10 by lasoubai          #+#    #+#             */
/*   Updated: 2026/07/30 11:18:13 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/WebServ.hpp"

ProcessCgi::ProcessCgi(Client *client, ProcessRequest &processRq,std::map<std::string, Session>& _sessions)
    : env(NULL), cgi_path(processRq.getCgiPath()), _client(client),
      script_path(processRq.getResourcePath()),sessions(&_sessions)
{
  if (_client->request->getRequestLine().Method == "POST" )
    client->cgi_input_buffer = &_client->request->getBody();
  client->cgi_input_offset = 0;
  envMap();
  envArray();
  connection = _client->request->getConnection();
  client->processCgi = this;
}

void ProcessCgi::envMap()
{
  if (_client->request->getContentLenght() > 0)
  {
    std::stringstream ss;
    ss << _client->request->getContentLenght();
    env_map["CONTENT_LENGTH"] = ss.str();
    env_map["CONTENT_TYPE"] = _client->request->getContentType();
  }  
  std::stringstream port;
  port << _client->request->getPort();
  env_map["SERVER_PORT"] = port.str();
  env_map["GATEWAY_INTERFACE"] = ("CGI/1.1");
  env_map["QUERY_STRING"] = _client->request->getRequestLine().Query;
  env_map["REMOTE_ADDR"] = _client->ip;
  env_map["REQUEST_METHOD"] = (_client->request->getRequestLine().Method);
  env_map["SCRIPT_NAME"] = _client->request->getPath(); 
  env_map["SCRIPT_FILENAME"] = script_path; 
  env_map["PATH_INFO"] = _client->request->getPath(); 
  env_map["REQUEST_URI"] = _client->request->getRequestLine().URI; 
  env_map["SERVER_NAME"] = _client->request->getHost();
  env_map["SERVER_PROTOCOL"] = _client->request->getRequestLine().HttpVers;
  env_map["SERVER_SOFTWARE"] = "Webserver/1.1";
  std::map<std::string, Session>::iterator it;
  it = sessions->find(_client->session_id);
  if (it != sessions->end())
  {
    port.str("");
    port.clear();
    port << it->second.counter;
    env_map["SESSION_COUNTER"] = port.str(); 
  }
  std::map<std::string, std::string>::iterator it_headr;
  std::map<std::string, std::string> header = _client->request->getHeaderMap();
  it_headr = header.begin();
  while (it_headr != header.end()) 
  {
    std::string env_name = "HTTP_" + it_headr->first;
    
    std::transform(env_name.begin(), env_name.end(), env_name.begin(), env_meta_data);
    env_map[env_name] = it_headr->second;
    it_headr++;
  }
}

void ProcessCgi::envArray() 
{
  size_t i = 0;
  env = new char *[env_map.size() + 1];
  std::map<std::string, std::string>::iterator it = env_map.begin();
  
  while (it != env_map.end()) {
    std::string value = (it->first) + "=" + (it->second);
    env[i] = new char[value.size() + 1];
    std::strcpy(env[i], value.c_str());
    i++;
    it++;
  }
  env[i] = NULL;
}

void ProcessCgi::errorResponse(int status_code,std::string status_messg)
{
  std::stringstream ErrorHead;
  ErrorHead << "HTTP/1.1 " <<status_code<< " "<<status_messg<<"\r\n";
  ErrorHead << "Server: Webserver/1.1\r\n";
  ErrorHead << "Content-Type: text/html\r\n";
  ErrorHead << "Date: " << generateHttpDate() << "\r\n";
  ErrorHead << "Connection: " << connection << "\r\n";
  
  _client->processRq->setStatusCode(status_code);
  _client->write_buffer.append(
    ServeStaticRq::html_Error_page(status_code, status_messg));
    ErrorHead << "Content-Length: " << _client->write_buffer.size() << "\r\n";
    _client->processRq->setStatusCode(status_code);
    _client->write_buffer.insert(0, ErrorHead.str() + "\r\n");
    _client->write_offset = 0;
  }
  
  void ProcessCgi::generetCgiResponse() 
  {
    std::string addHeader;
    std::string addLine;
    std::string &cgi_output = _client->cgi_output_buffer;
    std::string lower_header;
    
    addLine = "HTTP/1.1 200 OK\r\n";  
    _client->processRq->setStatusCode(200);
    
    if (_client->state == STATE_CGI_ERROR) {
      errorResponse(500,"Internal Server Error");
      return;
    }
    size_t p_body = 0;
    if ((p_body = cgi_output.find("\r\n\r\n")) == std::string::npos) 
    {
      errorResponse(BAD_GATEWAY,"Bad Gateway");
      return;
    }
    addHeader = cgi_output.substr(0, p_body) + "\r\n";
    lower_header = lowerString(addHeader);
    defineStatusHeader(addHeader,addLine); 
    
    if(_client->is_new)
    addHeader +=  "Set-Cookie: session_id=" + _client->session_id + "; Path=/; HttpOnly;" + "\r\n";
    if (cgi_output.size() > p_body + 4) 
    {
      if (lower_header.find("content-type") == std::string::npos)
      {
        errorResponse(BAD_GATEWAY,"Bad Gateway");
        return;
      }
      _client->write_buffer.append(cgi_output.substr(p_body + 4));
      if (lower_header.find("content-length") == std::string::npos) 
      { 
        std::stringstream str;
        str << "Content-Length: " << _client->write_buffer.size() << "\r\n";
        addHeader.append(str.str());
      }
    }
    checkClearClientSession(addHeader);
    _client->write_buffer.insert(0, addLine + addHeader + "\r\n");
    _client->write_offset = 0;
  }
  
  void ProcessCgi::defineStatusHeader(std::string& addHeader,std::string& addLine)
  {
    size_t p_stat = addHeader.find("Status:");
    if (p_stat != std::string::npos) 
    {
      size_t lineEnd = 0;
      if ((lineEnd = addHeader.find("\r\n", p_stat)) != std::string::npos) 
      {
        size_t pos_value = 0;
        if ((pos_value = addHeader.find(":", p_stat)) != std::string::npos)
        {
          pos_value++;
          while (pos_value < lineEnd && addHeader[pos_value] == ' ')
          pos_value++;
          std::string status_value =
          addHeader.substr(pos_value, lineEnd - (pos_value));
          status_value =  check_valid_status(status_value);
          addLine = "HTTP/1.1 " + status_value + "\r\n";
          addHeader.erase(p_stat, (lineEnd - p_stat) + 2);
        }
      }
    }
  }
  
  std::string ProcessCgi::check_valid_status(std::string& value)
  {
    size_t i = 0;
    bool is_char = false;
    while(i < value.size())
    {
      if (std::isalpha(value[i]))
      {
        is_char = true; 
        break;      
      }
      i++;
    }
    if (!is_char)
    {
      int code;
      std::stringstream str;
      std::string new_val;
      str << value;
      getline(str,new_val, ' ');
      code = std::atoi(value.c_str());
      _client->processRq->setStatusCode(code);
      return(new_val + " " + Logger::statusText(code));
    }
    return(value);
  }
  
  void  ProcessCgi::checkClearClientSession(std::string& addHeader)
  {
    size_t pos = 0;
    if ((pos = addHeader.find("Set-Cookie")) != std::string::npos)
    {
      if (addHeader.find("Max-Age=0") != std::string::npos)
      {
        SessionManager s(_client,*sessions);
        s.destroySession(_client->session_id);
      }
    }
  }

char env_meta_data(char c) 
{
  c = std::toupper(c);
  if (c == '-')
    c = '_';
  return (c);
}
  
  ProcessCgi::~ProcessCgi() 
  {
    if (env != NULL) {
      for (size_t i = 0; env[i] != NULL; ++i)
        delete[] env[i];
      delete[] env;
    }
  }
  
  char **ProcessCgi::getEnv() const { return (env); }
  std::string &ProcessCgi::getCgiPath() { return (cgi_path); }
  