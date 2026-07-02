/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServ.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/17 05:30:09 by zatais            #+#    #+#             */
/*   Updated: 2026/07/01 08:52:09 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <map>
#include <ctime>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

#include <poll.h>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

//
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>

#include <cctype>
#include <cerrno>
#include <sys/stat.h>




#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"

#include "Config.hpp"
#include "Logger.hpp"
#include "Server.hpp"
#include "Cgi.hpp"




#include "Request.hpp"
#include "ProcessRequest.hpp"
#include "ProcessCgi.hpp"
#include "ServeStaticRq.hpp"
#include "Response.hpp"

#endif // !WEBSERV_HPP
