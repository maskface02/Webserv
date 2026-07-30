/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServeStaticRq.cpp                                       :+:      :+:    :+:
 */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/17 20:21:25 by lasoubai          #+#    #+#             */
/*   Updated: 2026/06/18 17:07:12 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/WebServ.hpp"

ServeStaticRq::ServeStaticRq(Client *_client, ServerConfig &srv)
    : client(_client), serv(srv) 
{
  try {
    if (client->processRq->getStatusCode() != 0)
      throw HttpStatus(client->processRq->getStatusCode());
    if (client->request->getRequestLine().Method == "GET")
      serveGetRequest(client->processRq->getResourcePath());
    else if (client->request->getRequestLine().Method == "DELETE")
      serveDeleteRq();
    else if (client->request->getRequestLine().Method == "POST")
      servePostRq();
    client->processRq->setStatusCode(OK);
  } 
  catch (HttpStatus &e) 
  {
    int status_code = e.getErrorCode();
    client->processRq->setStatusCode(status_code);
  }
}

void ServeStaticRq::serveGetRequest(std::string resource_path) {
  std::string path_indexFile;
  if (client->processRq->is_dir) 
  {
    if ((path_indexFile = client->processRq->getIndexFile()) != "")
    {
      path_indexFile = resource_path + path_indexFile;
      resp_body = servFile(path_indexFile);
    } else
      check_AutoIndex();
  } else
    resp_body = servFile(resource_path);
}

std::string ServeStaticRq::servFile(std::string &path) {
  std::stringstream line;
  std::ifstream local_file(path.c_str());
  if (local_file.is_open()) {
    line << local_file.rdbuf();
    local_file.close();
  } 
  else
    throw HttpStatus(NOT_FOUND);
  return (line.str());
}

void ServeStaticRq::check_AutoIndex() {
  if (client->processRq->getLocation().autoindex == true)
    html_list_dir();
  else
    throw HttpStatus(FORBIDDEN);
}


void ServeStaticRq::html_list_dir() {
  std::stringstream str;
  std::vector<std::string> files;
  std::string path = client->processRq->getResourcePath();
  std::string toFile;
  files = directory_files(path);
  struct stat structStat;
  str << "<!DOCTYPE html>\n";
  str << "<html>\n";
  str << "<head>\n"
         "<title>Index of /path/</title>\n"
         "<style>\n"
         "body { font-family: monospace; margin: 20px; }\n"
         "       table { border-collapse: collapse; width: 100%; }\n"
         "th, td { text-align: left; padding: 4px 15px; }\n"
         "th { border-bottom: 1px solid #ccc; }\n"
         "tr:hover { background: #f5f5f5; }\n"
         "a { text-decoration: none; color: #0645ad; }\n"
         " </style>\n"
         "</head>\n";
  str << "<body>\n"
         "<table>\n"
         "<tr><th>Name</th><th>Last Modified</th><th>Size</th></tr>\n";
  str << "<h2>List files</h2>\n";

  str << "<tr><td><a href=\"../\">../</a></td><td>-</td><td>-</td></tr>\n";
  size_t i = 0;
  while (i < files.size()) {
    toFile = path + files[i];
    if (!stat(toFile.c_str(), &structStat) && files[i] != "." &&
        files[i] != "..") {
      if (S_ISDIR(structStat.st_mode))
        files[i] += "/";
      std::string last_modif = last_modif_time(structStat);
      size_t file_size = structStat.st_size;
      str << "<tr><td><a href=\"" << files[i] << "\">" << files[i] << "</a>";
      str << "</td><td>  " << last_modif << "</td>";
      if (S_ISDIR(structStat.st_mode))
        str << "<td>  " << "-" << "</td></tr>\n";
      else
        str << "<td>  " << file_size << "</td></tr>\n";
      toFile.clear();
    }
    i++;
  }
  str << "</table>\n"
         "</body>\n"
         "</html>\n";
  resp_body = str.str();
  client->processRq->setExtension(".html");
}

std::string ServeStaticRq::last_modif_time(struct stat s) {
  char buff[25];
  time_t _time = s.st_mtime;
  struct tm *time_modif = localtime(&_time);
  strftime(buff, sizeof(buff), " %d-%b-%Y  %H:%M", time_modif);
  return (buff);
}

void ServeStaticRq::serveDeleteRq() {

  std::string resource_path = client->processRq->getResourcePath();
  if (client->processRq->is_dir) {
    size_t pos = resource_path.rfind("/");
    if (pos == std::string::npos)
      throw HttpStatus(BAD_REQUEST); 

    pos = resource_path.rfind("/", pos - 1);
    std::string parnt_dir_path ;
    parnt_dir_path = client->processRq->getResourcePath().substr(0, pos); 
    if (access(parnt_dir_path.c_str(), F_OK | W_OK | X_OK))
      throw HttpStatus(FORBIDDEN);
    std::vector<std::string> files = directory_files(resource_path);
    delete_files(files);
  }
  if (std::remove(resource_path.c_str()) == -1) {
    throw HttpStatus(FORBIDDEN);
  }
}

void ServeStaticRq::delete_files(std::vector<std::string> files) {
  size_t i = 0;
  std::string path = client->processRq->getResourcePath();
  while (i < files.size()) {
    path += files[i];
    if (std::remove(path.c_str()) == -1)
       throw HttpStatus(FORBIDDEN);
    path.clear();
    path = client->processRq->getResourcePath();
    i++;
  }
}

void ServeStaticRq::servePostRq() {
  if (client->processRq->getLocation().upload_enabled) {
    size_t pos = 0;
    file_path = client->processRq->getLocation().upload_store;
    if ((pos = file_path.rfind("/")) != file_path.length() - 1)
      file_path += "/";
     struct stat pathStat;

    if (stat(file_path.c_str(),&pathStat) )
    {
      throw HttpStatus(INTERNAL_SERVER_ERROR);
    } 
    if (client->request->is_boundry)
    {
      upload_files();
    }
    else {
      if (!client->processRq->is_dir)
      {
        struct stat pathStat;
        if ((pos = client->processRq->getResourcePath().rfind("/")))
            file_path += client->processRq->getResourcePath().substr(pos + 1);
        if (!stat(file_path.c_str(),&pathStat))
            throw HttpStatus(FORBIDDEN );
        std:: ofstream file (file_path.c_str());
        if (file.is_open())
            file << client->request->getBody();
        else
            throw HttpStatus(FORBIDDEN );
        file.close();
        throw HttpStatus(CREATED);
      }
    }
  }
  else
    throw HttpStatus(METHOD_NOT_ALLOWED);
}

void ServeStaticRq::upload_files() {
  std::map<std::string, std::string> boundry = client->request->getBoundryMap();
  std::map<std::string, std::string>::iterator it;
  std::string path = file_path;
  std::vector<std::string> files = directory_files(file_path);
  it = boundry.begin();
  if (boundry.empty())
    throw HttpStatus(NOT_FOUND);
  while (it != boundry.end()) {
    path += it->first;
    if (check_exist_file(it->first, files))
        throw HttpStatus(FORBIDDEN);
    std::ofstream file(path.c_str());
    if (file.is_open())
      file << it->second;
    else
      throw HttpStatus(FORBIDDEN);
    file.close();
    path.clear();
    path = file_path;
    it++;
  }
  client->processRq->setRedirctUrl(file_path);
  throw HttpStatus(CREATED);
}
std::vector<std::string> ServeStaticRq::directory_files(std::string &path) {
  std::vector<std::string> files;
  DIR *op_dir;
  struct dirent *read_dir;

  op_dir = opendir(path.c_str());
  if (!op_dir)
    throw HttpStatus(FORBIDDEN);
  while ((read_dir = readdir(op_dir)) != NULL) {
    files.push_back(read_dir->d_name);
  }
  closedir(op_dir);
  return (files);
}

int ServeStaticRq::check_exist_file(std::string new_file,
                                     std::vector<std::string> &files) {
  size_t i = 0;
  while (i < files.size()) {
    if (files[i] == new_file)
      return(1);
    i++;
  }
  return(0);
}

std::string ServeStaticRq::serveError(int status_code, ServerConfig &srv, Client* client) {
  client->processRq->setExtension(".html");
  std::map<int, std::string>::iterator it;
  it = srv.error_pages.find(status_code);
  if (it != srv.error_pages.end())
  {
     try
        {
          return (ServeStaticRq::servFile(it->second));
        }
        catch( HttpStatus& e)
        { 
          return(ServeStaticRq::html_Error_page(status_code, Logger::statusText(status_code)));
        }
  }

  return (
      ServeStaticRq::html_Error_page(status_code, Logger::statusText(status_code)));
}

std::string ServeStaticRq::html_Error_page(int status_code, std::string stat) {

  std::stringstream body;
  body << "<html>\n";
  body << "<head><title>" << status_code << " " << stat << "</title></head>\n";
  body << "<body>\n";
  body << "<center><h1>" << status_code << " " << stat << "</h1></center>\n";
  body << "<hr><center>" << "WebServer/1.1" << "</center>\n";
  body << "</body>\n";
  body << "</html>\n";

  return body.str();
}

std::string ServeStaticRq::getRespBody() const { return (resp_body); }


void ServeStaticRq::setResponseBody(std::string body) { resp_body = body; }

ServeStaticRq::~ServeStaticRq(){}