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


ServeStaticRq::ServeStaticRq(Request& rqst, ProcessRequest& PRq,ServerConfig& srv)
:ProcessRq(&PRq),request(&rqst),serv(srv)
{
    
    if (ProcessRq->getStatusCode() != 0)
    {
        ServeError(ProcessRq->getStatusCode());
        return;
    }
    if (request->getRequestLine().Method == "GET")
    {
        ServeGetRequest(ProcessRq->getResourcePath());
    }
    else  if(request->getRequestLine().Method == "DELETE")
        ServeDeleteRq();
    else if (request->getRequestLine().Method == "POST")
        ServePostRq();
    if (ProcessRq->getStatusCode() != 0)
    {
        if (ProcessRq->getStatusCode() != 201)
            ServeError(ProcessRq->getStatusCode());
    }
    else
    {
        ProcessRq->setStatusCode(200);
        status();
    }
}

void    ServeStaticRq::ServeGetRequest(std::string resource_path)
{
    std::string path_indexFile;
    if (ProcessRq->is_dir)
    {
        if ((path_indexFile = ProcessRq->getIndexFile()) != "")
        { 
            path_indexFile = resource_path + path_indexFile ;
            servFile(path_indexFile);
        }
        else
            check_AutoIndex();
    }
    else
        servFile(resource_path);
}

void ServeStaticRq::servFile(std::string& path)
{
    std::string line;
    std::ifstream local_file (path.c_str());
    if (local_file.is_open())
    {
        while(getline(local_file,line))
        {
             resp_body+= line + "\n";//added
        }
        local_file.close();
    }
    else
        ProcessRq->setStatusCode(404);
}

void ServeStaticRq::check_AutoIndex()
{
    if (ProcessRq->getLocation().autoindex)
        html_list_dir();
    else
        ServeError(403);
}

void ServeStaticRq::html_list_dir()
{
    //if folder list the files // javascript
    DIR* op_dir;
    struct dirent* read_dir;
    std::stringstream str(resp_body);

    op_dir = opendir(ProcessRq->getResourcePath().c_str());
    if (!op_dir)
        ServeError(403);
    str << "<!DOCTYPE html>\n";
    str<< "<html>\n";
    str << "<body>\n";
    str << "<h2>List fils</h2>\n";
    str << "<ul>\n";
    while ((read_dir = readdir(op_dir)) != NULL)
        if(read_dir->d_name[0] != '.')
             str << " <li>"<<read_dir->d_name<<"</li>\n";
    str << "</ul>\n"  
           "</body>\n"
           "</html>\n";
    resp_body = str.str();
    ProcessRq->setExtension(".html");
    closedir(op_dir);// check 

}

void ServeStaticRq::ServeDeleteRq()
{
    if (ProcessRq->is_dir)
    {
        size_t pos = ProcessRq->getResourcePath().rfind("/");
        if (pos == std::string::npos)
             ServeError(403);

        // pos = ProcessRq->getResourcePath().rfind("/",pos - 1);

        std::string dir_path = ProcessRq->getResourcePath() ;//.substr(0,pos);
        if (access(dir_path.c_str(), F_OK))
        {
            ServeError(403);
            return;
        }
    }
    std::string cmd = "rm -rf " + ProcessRq->getResourcePath();/// if no premis
    std::system(cmd.c_str());
}

void    ServeStaticRq::ServePostRq()
{
    if (ProcessRq->getLocation().upload_enabled)
    {   
        size_t pos = 0;
        file_path = ProcessRq->getLocation().upload_store;
        if ((pos = file_path.rfind("/")) == file_path.length() - 1)
            file_path = file_path.substr(0, pos); 
        
        file_path += request->getPath();
        if (ProcessRq->is_dir)
        { 
            if (request->is_boundry)
                upload_files();
            else ProcessRq->setStatusCode(405);
        }
        else  ProcessRq->setStatusCode(405);
        // {
        //     std:: ofstream file (file_path.c_str());
        //     if (file.is_open())
        //     {
        //         file << request->getBody();
        //     }
        //     file.close();
        //     ProcessRq->setStatusCode(201);
        // }
    }
    else  ServeError(403);
}

void ServeStaticRq::upload_files()
{
    std::map<std::string, std::string> boundry = request->getBoundryMap();
    std::map<std::string, std::string>::iterator it;
    std::string path = file_path;
    it = boundry.begin();
    while(it != boundry.end())
    {
        path += it->first;   
        std:: ofstream file (path.c_str());
        if (file.is_open())
        {
            file << it->second;
        }
        else
        {
            ServeError(403);
            return;
        }
        file.close();
        path.clear();
        path = file_path;
        it++;
    } 
    ProcessRq->setStatusCode(201);
}
void ServeStaticRq::ServeError(int status_code)
{
    status(); 
    ProcessRq->setExtension(".html");
    std::map<int, std::string>::iterator it;
    it = serv.error_pages.find(status_code);
    if (it !=  serv.error_pages.end())
    {
        servFile(it->second);
        return;
    }
    resp_body = html_Error_page(status_code, Status);
}


// <html>
// <head><title>404 Not Found</title></head>
// <body>
// <center><h1>404 Not Found</h1></center>
// <hr><center>nginx/1.31.2</center>
// </body>
// </html>

std::string  ServeStaticRq::  html_Error_page(int status_code, std::string stat)
{
    
    std::stringstream body;
    body << "<html>\n";
    body << "<head><title>" << status_code << " " << stat << "</title></head>\n";
    body << "<body>\n";
    body <<"<hr><center>" << "WebServer/1.1"<< "</center>\n";
    body<<"</body>\n";
    body<<"</html>\n";

  return body.str();
}

void ServeStaticRq::status()
{
    // make it a map?
    int status_code = ProcessRq->getStatusCode();
    switch(status_code)
    {
        case 201:
            Status = "Created";
            break;
        case 301:
            Status = "Moved Permanently";
            break;
        case 400:
            Status = "Bad Request";
            break;
        case 403:
            Status = "Forbidden";
            break;
        case 404:
            Status = "Not Found";
            break;
       case 405:
            Status = "Method Not Allowed";
            break;
        case 411:
            Status = "Length Required";
            break;
        case 413:
            Status = "Content Too Large";
            break;
        case 414:
            Status = "URI Too Long";
            break;
        case 500:
            Status = "Internal Server Error";
            break;
        case 501:
            Status = "Not Implemented";
            break;
        case 505:
            Status = "HTTP Version Not Supported";
            break;
        case 502:
            Status = "Bad Gateway";
            break;
        default:
            Status = "OK";
            break;
    }
}

std::string  ServeStaticRq::getRespBody() const
{
    return(resp_body);
}
std::string   ServeStaticRq::getStatus() const
{
    return (Status);
}
