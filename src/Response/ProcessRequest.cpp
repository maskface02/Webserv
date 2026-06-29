/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ProcessRequest.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/16 12:06:38 by lasoubai          #+#    #+#             */
/*   Updated: 2026/06/28 18:49:57 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../../include/WebServ.hpp"

ProcessRequest::ProcessRequest( Request& req,  ServerConfig& _srv)
:request(&req),srv(&_srv),status_code(0),is_CgiRq(0),is_dir(0),is_RedirecRq(0)
{

    if (check_status())
        return;
    if (!match_location(_srv))
    {
        status_code = 404;
        return;
    } 
    if (check_redirction())
        return;
    if (!check_max_body_size())
        return;    
    if (!check_allowed_method())
        return; 
    define_type();  
    if(status_code)
        return;
    extract_file_extension();
    check_Cgi();
    if (!is_CgiRq)
        is_Static = true;
}

bool ProcessRequest::check_status()
{
    if ((status_code = request->getStatusCode()) != 0)
        return(true);
    return (false);
}

bool ProcessRequest::match_location(ServerConfig& server)
{
    normlize_location_path(server);
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
         vec_it = prefix.begin();
        while (vec_it != prefix.end())
        {
            if (*vec_it == it->path)
                location[it->path.length()] = *it;
            vec_it++;
        }
        it++;
    }
    if ( !found)
    {
        if (location.empty()) 
        {
            status_code = 404;
            return(false);
        }
        target_location = location.rbegin()->second; 
    }
    if (target_location.root.rfind("/") == target_location.root.length() - 1)
           resource_path = target_location.root + request->getPath().substr(1);
    else
        resource_path = target_location.root + request->getPath();   
   
    //   std::cout<<"Target location == "<<target_location.path<<std::endl;
    //     std::cout<<"Resource path == "<<resource_path<<std::endl;
    return (true);
}
// normalize
//* we remove the slash at the end of the location 
//* to make the prefix match the location even if there is a slash at the end 
//* exmple 
// config has:   /api/
// prefix list:  /  ,  /api  ,  /api/v1
//  "/api" == "/api"  ?  YES 
// "/api/" == "/api"  ?  NO  → missed without normalization
//TO TEST
void ProcessRequest::normlize_location_path(ServerConfig& server)
{
    std::vector<Location> ::iterator it = server.locations.begin();
    while (it != server.locations.end())
    {
        size_t pos = 0;
        if (it->path.length() > 1 
            &&( pos = it->path.find("/",it->path.length() - 1)) != std::string::npos)
            it->path.erase(pos, 1);
        it++;
    }
}

bool    ProcessRequest::check_max_body_size()
{
    if(!request->getBody().empty())
    {
        size_t max_size =  target_location.client_max_body_size;
        if (!max_size)
            max_size = srv->client_max_body_size;
        if (request->getBody().size() > max_size)
        {
            status_code = 413;
            return(false);
        }
    }
    return(true);
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

void ProcessRequest::define_type()
{
   struct stat pathStat;

    if (!stat(resource_path.c_str(),&pathStat))
    {
        if (S_ISDIR(pathStat.st_mode))
        {
            if (access(resource_path.c_str(), F_OK | R_OK | X_OK)) 
            {
                status_code = 404;
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
                check_index_file();// also for post in case of  CGI
            }
        }
        else if(S_ISREG(pathStat.st_mode))
            is_file = true;
    }
    else 
    {
        // if (errno == EACCES)
        //    status_code = 403;
        // else
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
        status_code = 404;
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
< HTTP/1.1 403 Not Found

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
