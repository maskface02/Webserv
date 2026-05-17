/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebServ.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zatais <zatais@student.1337.ma>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/17 05:30:09 by zatais            #+#    #+#             */
/*   Updated: 2026/05/17 05:30:09 by zatais           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <map>
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

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"

#include "Config.hpp"
#include "Logger.hpp"
#include "Server.hpp"

#endif // !WEBSERV_HPP
