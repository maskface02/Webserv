/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ProcessRequest.cpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/16 12:06:38 by lasoubai          #+#    #+#             */
/*   Updated: 2026/07/10 18:07:05 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../../include/WebServ.hpp"

ProcessRequest::ProcessRequest(Client*client,  ServerConfig& _srv):request(client->request)
{

    try{
        init_variable();
        check_status();
        match_location(_srv);// if 2 it match the last one !!!!!!
        check_redirction();
        check_max_body_size(_srv);
        check_allowed_method();
        int code = 0;
        if ( (code = define_type()) != 0)
        {
            if (request->getRequestLine().Method == "POST")//FIXED
            {
                extract_file_extension();
                check_Cgi();
                if (!is_CgiRq)
                    is_Static = true;
                return;
            }
                
            throw HttpError(code);
        }
        extract_file_extension();
        check_Cgi();
        if (!is_CgiRq)
            is_Static = true;
    }
    catch( HttpError& e)
    {
        status_code = e.getErrorCode();
    }
}

void ProcessRequest::check_status()
{
    if (request->getStatusCode() != 0)
       throw HttpError(request->getStatusCode());
}

void ProcessRequest::match_location(ServerConfig& server)
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
           throw HttpError(404);
        target_location = location.rbegin()->second; 
    }
  //added 
    std::string path_after_location = request->getPath().substr(target_location.path.length());
    if (!path_after_location.empty() && path_after_location[0] == '/')
        path_after_location = path_after_location.substr(1);
    if (target_location.root.rfind("/") == target_location.root.length() - 1)
        resource_path = target_location.root + path_after_location;
    else
        resource_path = target_location.root + "/" + path_after_location;
   
      std::cout<<"Target location == "<<target_location.path<<std::endl;
      std::cout<<"Resource path == "<<resource_path<<std::endl;
 //added 
}

// normalize
//* we remove the slash at the end of the location 
//* to make the prefix match the location even if there is a slash at the end 
//* exmple 
// config has:   /api/
// prefix list:  /  ,  /api  ,  /api/v1
//  "/api" == "/api"  ?  YES 
// "/api/" == "/api"  ?  NO  → missed without normalization


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

void ProcessRequest::check_redirction()
{
    if (target_location.redirect.enabled)
    {
      is_RedirecRq = true;
      redirect_url = target_location.redirect.url;
      throw HttpError(target_location.redirect.code);
    }
}

void    ProcessRequest::check_max_body_size(ServerConfig& srv)
{
    if(!request->getBody().empty())
    {
        size_t& max_size =  target_location.client_max_body_size;
        if (!max_size)
            max_size = srv.client_max_body_size;
        if (request->getBody().size() > max_size)
            throw HttpError(CONTENT_TOO_LARGE );
    }
}

void ProcessRequest::check_allowed_method()
{
    size_t i = 0;

    while (i < target_location.allow_methods.size())
    { 
        if (request->getRequestLine().Method == target_location.allow_methods[i])
            return;
        i++;
    }
   throw HttpError(METHOD_NOT_ALLOWED);
}

int ProcessRequest::define_type()
{
   struct stat pathStat;

    if (!stat(resource_path.c_str(),&pathStat) )
    {
        if (S_ISDIR(pathStat.st_mode))
        {
            if (access(resource_path.c_str(), F_OK | X_OK)) 
                 return(FORBIDDEN);
            else
            {
                is_dir = true;
                check_index_file();
                
                size_t pos = resource_path.rfind("/");
                if (pos != resource_path.length() - 1)
                {                    
                    if(request->getRequestLine().Method == "GET")
                    {
                        if (Index_file.empty() && !target_location.autoindex)
                            return(NOT_FOUND);
                        std::stringstream port_ss;
                        port_ss << request->getPort();
                        redirect_url = "http://" + request->getHost() + ":" + port_ss.str() + request->getPath() + "/";
                        return(MOVED_PERMANENTLY);
                    }
                }
            }
        }
        else if(S_ISREG(pathStat.st_mode))
        {
            is_file = true;
        }
    }
    else 
    {
        if (errno == EACCES)
           return(FORBIDDEN);
        else
           return(NOT_FOUND);
    }
    return(0);
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
       throw HttpError(FORBIDDEN);
    while ((read_dir = readdir(op_dir)) != NULL)
    {
        files.push_back(read_dir->d_name);
    }
    closedir(op_dir);
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
    //TO DO lower  the extensionm serve post no boundary
    extension = "default";
  
      
    if (is_dir)
    {
        size_t pos = Index_file.rfind(".");
        if (pos != std::string::npos)
            extension = Index_file.substr(pos);
    }  
    else//FIXED
    { 
         size_t pos1 = resource_path.rfind(".");
        size_t pos2 = resource_path.rfind("/");
        if (pos1 != std::string::npos && pos2 != std::string::npos && pos1 > pos2 )
            extension = resource_path.substr(pos1);
        }
    std::cout<<"\n===this is the extension=====  "<<extension<<"\n";
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

void ProcessRequest::init_variable()
{
    is_file = false;
    is_Static = false;
    status_code = 0;
    is_CgiRq = false;
    is_dir = false;  
    is_RedirecRq = false;
}

std::string ProcessRequest::getExtension() const
{
    return(extension);
}

int       ProcessRequest::getStatusCode() 
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

void        ProcessRequest::setRedirctUrl(std::string& url)
{
    redirect_url = url;
}
