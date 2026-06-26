/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/09 11:59:47 by lasoubai          #+#    #+#             */
/*   Updated: 2026/06/25 21:44:34 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/WebServ.hpp"
#include<stdio.h>
int main()
{
    // location problem 
    std::string configPath = "testconf.conf";
    // std::string req = "POST /stream HTTP/1.1\r\nHost: example.com\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nHello\r\n7\r\nWorld!\r\n2b\r\nStreaming raw data.\r\n0\r\n\r\n";

    std::string req ="GET /html/ HTTP/1.1\r\nHost:  127.0.0.1:8080\r\n\r\n";
    
    try 
    {
      Config config;
      config.load(configPath);   
        std::cout <<config.getServers()[0].locations[0].redirect.code <<"\n";
    //   printf("im here \n");
    //   std::cout<<config.getServers()[0].error_pages[404]<<std::endl;
      Request R(req);
      ProcessRequest PS(R,config.getServers()[0]);
      ServeStaticRq Sc(R,PS,config.getServers()[0]);
        Response Rp (PS,Sc,R);
    //   std::cout <<Sc.getRespBody()<< std::endl;
      std::cout <<Rp.getHttpResponse()<< std::endl;
      
      
    } 
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}




// in conf 

// location /home {
//   route /.....;
//   index index.html index.py;
// }