/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServeStaticRq.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/17 20:21:25 by lasoubai          #+#    #+#             */
/*   Updated: 2026/06/18 17:07:12 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/WebServ.hpp"


ServeStaticRq::ServeStaticRq(Client* _client,ServerConfig& srv)
:client(_client),serv(srv)
{

    // init_variable();
    try{
        if (client->processRq->getStatusCode() != 0)
            throw HttpError(client->processRq->getStatusCode());
        if (client->request->getRequestLine().Method == "GET")
            _ServeGetRequest(client->processRq->getResourcePath());
        else  if(client->request->getRequestLine().Method == "DELETE")
            _ServeDeleteRq();
        else if (client->request->getRequestLine().Method == "POST")
            _ServePostRq();
        client->processRq->setStatusCode(OK);
    }
      catch( HttpError& e)
    {
        int status_code = e.getErrorCode();
        client->processRq->setStatusCode(status_code);
        // std::cerr<<status_code<<std::endl;
    }
}

void    ServeStaticRq::_ServeGetRequest(std::string resource_path)
{
    std::string path_indexFile;
    if (client->processRq->is_dir)
    {
        if ((path_indexFile = client->processRq->getIndexFile()) != "")
        { 
            path_indexFile = resource_path + path_indexFile ;
            resp_body = servFile(path_indexFile);
        }
        else
            check_AutoIndex();
    }
    else
        resp_body =  servFile(resource_path); 
}

std::string ServeStaticRq::servFile(std::string& path)
{
    std::stringstream line;
    std::ifstream local_file (path.c_str());
    if (local_file.is_open())
    {
        line << local_file.rdbuf();
        local_file.close();
    }
    else
        throw HttpError(NOT_FOUND);
    return (line.str());
}

void ServeStaticRq::check_AutoIndex()
{
    if (client->processRq->getLocation().autoindex ==  true)
         html_list_dir();
    else
       throw HttpError(FORBIDDEN );
}

//TO FIX
void ServeStaticRq::html_list_dir()
{
    //if folder list the files // javascript
    DIR* op_dir;
    struct dirent* read_dir;
    std::stringstream str(resp_body);

    op_dir = opendir(client->processRq->getResourcePath().c_str());
    if (!op_dir)
       throw HttpError(FORBIDDEN );
    str << "<!DOCTYPE html>\n";
    str<< "<html>\n";
    str << "<body>\n";
    str << "<h2>List files</h2>\n";
    str << "<ul>\n";
    while ((read_dir = readdir(op_dir)) != NULL)
        if(read_dir->d_name[0] != '.')
             str <<  " <li><a href=\""<< read_dir->d_name <<  "\">" <<"</a></li>\n";
    str << "</ul>\n"  
           "</body>\n"
           "</html>\n";
    resp_body = str.str();
    client->processRq->setExtension(".html");
    closedir(op_dir);// check 

}

void ServeStaticRq::_ServeDeleteRq()
{
    if (client->processRq->is_dir)
    {
        size_t pos = client->processRq->getResourcePath().rfind("/");
        if (pos == std::string::npos)
              throw HttpError(FORBIDDEN );;

        // pos = client->processRq->getResourcePath().rfind("/",pos - 1);

        std::string dir_path = client->processRq->getResourcePath() ;
        if (access(dir_path.c_str(), F_OK))
           throw HttpError(FORBIDDEN );
            return;
    }
    check_valid_path( client->processRq->getResourcePath());
    std::string cmd = "rm -rf " + client->processRq->getResourcePath();/// if no premis
    std::system(cmd.c_str());
}
void    ServeStaticRq::check_valid_path(std::string path)
{
    for (size_t i = 0; i < path.size(); ++i)
    {
        char c = path[i];
        if (c == ';' || c == '|' || c == '&' || c == '`' || c == '$' || c == '(' || c == ')')
        {
            throw HttpError(400);
        }
    }
}

void    ServeStaticRq::_ServePostRq()
{
    if (client->processRq->getLocation().upload_enabled)
    {   
       size_t pos = 0;
        file_path = client->processRq->getLocation().upload_store;
        if ((pos = file_path.rfind("/")) != file_path.length() - 1)
            file_path += "/"; 
        if (client->request->is_boundry)
            upload_files();
        else throw HttpError(METHOD_NOT_ALLOWED );
    }
    else  throw HttpError(METHOD_NOT_ALLOWED);
}

void ServeStaticRq::upload_files()
{
    std::map<std::string, std::string> boundry = client->request->getBoundryMap();
    std::map<std::string, std::string>::iterator it;
    std::string path = file_path;
    std::vector<std::string> files = directory_files();
    it = boundry.begin();
    while(it != boundry.end())
    {
        path += it->first;
        check_exist_file(it->first,files);
        std:: ofstream file (path.c_str());
        if (file.is_open())
            file << it->second;
        else
           throw HttpError(FORBIDDEN );
        file.close();
        path.clear();
        path = file_path;
        it++;
    }
    client->processRq->setRedirctUrl(file_path);
    throw HttpError(CREATED);;
}
std::vector<std::string> ServeStaticRq:: directory_files()
{
    std::vector<std::string> files;
    DIR* op_dir;
    struct dirent* read_dir;

    op_dir = opendir(file_path.c_str());
    if (!op_dir)
       throw HttpError(403);
    while ((read_dir = readdir(op_dir)) != NULL)
    {
        files.push_back(read_dir->d_name);
    }
    return(files);
}

void ServeStaticRq::check_exist_file(std::string new_file, std::vector<std::string>& files)
{
   size_t i = 0;
   while (i < files.size())
   {
        if (files[i] == new_file)
            throw  HttpError(FORBIDDEN);
        i++;
   }
}

std::string  ServeStaticRq::  html_Error_page(int status_code, std::string stat)
{
    
    std::stringstream body;
    body << "<html>\n";
    body << "<head><title>" << status_code << " " << stat << "</title></head>\n";
    body << "<body>\n";
    body << "<center><h1>" << status_code << " " << stat << "</h1></center>\n";
    body <<"<hr><center>" << "WebServer/1.1"<< "</center>\n";
    body<<"</body>\n";
    body<<"</html>\n";

  return body.str();
}

std::string  ServeStaticRq::getRespBody() const
{
    return(resp_body);
}
std::string   ServeStaticRq::getStatus() const
{
    return (status_messg);
}


 void        ServeStaticRq::setResponseBody(std::string body)
 {
    resp_body = body;
 }
