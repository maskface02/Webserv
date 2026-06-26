/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ProcessRequest.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/16 12:06:38 by lasoubai          #+#    #+#             */
/*   Updated: 2026/06/26 11:54:53 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../../include/WebServ.hpp"

// done for basics check
ProcessRequest::ProcessRequest(Request& req,  ServerConfig& _srv)
:request(&req),srv(&_srv),status_code(0),is_CgiRq(0),is_dir(0),is_RedirecRq(0)
{

    if (check_status())
        return;
    if (!match_location(_srv))
    {
        status_code = 404; std::cout <<"1"<<std::endl;
        return;
    } 
    // home 
    if (check_redirction())
        return;
    if (!check_allowed_method())
        return; 
    define_type();
    if(status_code)
        return;
    // if is a dir i need always to check indx file for all method caz of cgi
    //if not cgi i will need it for grt method
    extract_file_extension();// if is dir look for index and get it extension
   
    check_Cgi();
    if (!is_CgiRq)
        is_Static = true;
    // std::cout<<"is cgi :"<< is_CgiRq<<std::endl;
    // std::cout<<"resource path "<<resource_path<<std::endl;
}

bool ProcessRequest::check_status()
{
    if ((status_code = request->getStatusCode()) != 0)
        return(true);
    return (false);
}

bool ProcessRequest::match_location(ServerConfig& server)
{
    std::vector<Location> ::iterator it = server.locations.begin();
    std::map<size_t,Location> location ;
    bool found = false; 
    
    size_t pos = 0;
    std::vector<std::string> prefix ;
    prefix.push_back("/");
    pos = request->getPath().find("/",1);
    while(pos != std::string::npos)
    {
        prefix.push_back(request->getPath().substr(0,pos));
        pos = request->getPath().find("/",pos + 1);
    }
    std::vector<std::string> ::iterator vec_it = prefix.begin();
    while(it != server.locations.end())
    {
        if (it->path == request->getPath())
        {
            target_location = *it;
            found = true;
            break;
        }
        while (vec_it != prefix.end())
        {
            if (*vec_it == it->path)
                location[it->path.length()] = *it;
            vec_it++;
        }
        it++;
    }
    if (!location.empty() && !found)
        target_location = location.rbegin()->second; 
     // 
    if (target_location.root.rfind("/") == target_location.root.length() - 1)
            resource_path = target_location.root + request->getPath().erase(0,1);
    else
        resource_path = target_location.root + request->getPath();   
    if (location.empty() && !found)
    {
         
    status_code = 404;
     return(false);
    }
      
  return (true);
}

bool ProcessRequest::check_allowed_method()
{
    size_t i = 0;

    while (i < target_location.allow_methods.size())
    { 
        if (request->getRequestLine().Method == target_location.allow_methods[i])
            return(true);
        i++;
    }
    status_code = 405;
    return(false);
}
// /if (request->getRequestLine().Method == "GET")
                        // check_index_file()/;

void ProcessRequest::define_type()
{
   struct stat pathStat;
    
//define resource path here
   
    if (!stat(resource_path.c_str(),&pathStat))
    {
        if (S_ISDIR(pathStat.st_mode))
        {
            if (access(resource_path.c_str(), F_OK | R_OK | X_OK)) 
            {
                status_code = 403;
                return;
            }
            else
            { 
                is_dir = true;
                size_t pos = resource_path.rfind("/");
               
                if (pos != resource_path.length() - 1)
                {
                    if(request->getRequestLine().Method == "GET")
                    {
                        status_code = 301;
                        redirect_url = "http:/" + request->getPath() + "/";
                    }
                }
                // else (!check_index_file())
                // {
                    
                // }
                    
            }
        }
        else if(S_ISREG(pathStat.st_mode))
            is_file = true;

    }
    
    else 
    {//case () => 
        if (errno == EACCES)
           status_code = 403;
        else
            status_code = 404;
    }

    
}

//*******index****
//
// find the first file match with the indexs in location
//
//********* ************/

void ProcessRequest::check_index_file()
{
   size_t i = 0;
   size_t j = 0;
    std::vector<std::string> IndexVect = target_location.index;
    std::vector<std::string> files;
    DIR* op_dir;
    struct dirent* read_dir;

    op_dir = opendir(resource_path.c_str());
    if (!op_dir)
    {
        status_code = 403;
        return;
    }
    while ((read_dir = readdir(op_dir)) != NULL)
    {
        files.push_back(read_dir->d_name);
    }
    while (i < IndexVect.size())
    {
        j = 0;
        while (j < files.size())
        {
            if (files[j] == IndexVect[i])
            {
                Index_file = IndexVect[i];
                return;
            }
            j++;
        }
        i++;
    }
}

void    ProcessRequest::extract_file_extension()
{
    if (is_file)
    {
        size_t pos1 = resource_path.rfind(".");
        size_t pos2 = resource_path.rfind("/");
        if (pos1 != std::string::npos && pos2 != std::string::npos && pos1 > pos2 )
            extension = resource_path.substr(pos1);
    }
    if (is_dir)
    {
        size_t pos = Index_file.rfind(".");
        if (pos != std::string::npos)
            extension = Index_file.substr(pos);
    }
}

bool ProcessRequest::check_redirction()
{
    if (target_location.redirect.enabled)
    {
        status_code = target_location.redirect.code;
  
        redirect_url = target_location.redirect.url;

        return(true);
    }
    return(false);
}

void ProcessRequest::check_Cgi()
{
    // if (!extension.empty())
        if (check_location_extention())
            is_CgiRq = true;
}

bool ProcessRequest::check_location_extention()
{
    std::map<std::string, std::string> ::iterator it;
    it = target_location.cgi.find(extension.c_str());
    if (it == target_location.cgi.end())
        return(false);
    cgi_path = it->second;
    return(true);
}

std::string ProcessRequest::getExtension() const
{
    return(extension);
}

int       ProcessRequest::getStatusCode() const
{
     return (status_code) ;
}

std::string ProcessRequest::getResourcePath() const
{
    return (resource_path);
}

std::string ProcessRequest::getIndexFile() const
{
    return (Index_file);
}

std::string ProcessRequest::getCgiPath() const
{
    return (cgi_path);
}
                   
Location  ProcessRequest::getLocation() const
{
    return(target_location);
}

// ServerConfig   ProcessRequest::getServer() const
// {
//     std::cout<<srv->error_pages[404];
//     return(*srv);
// }
std::string     ProcessRequest::getRedirectUrl() const 
{
    return(redirect_url);
}

void        ProcessRequest::setStatusCode(int code)
{
    status_code = code;
}

void        ProcessRequest::setRedirectURL(std::string redrct_url)
{
   redirect_url = redrct_url;
}

void        ProcessRequest::setExtension(std::string _extension)
{
    extension = _extension;
}
