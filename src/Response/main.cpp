/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/06/09 11:59:47 by lasoubai          #+#    #+#             */
/*   Updated: 2026/06/28 06:50:29 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/WebServ.hpp"
#include<stdio.h>
int main()
{
    std::string configPath = "testconf.conf";
    // std::string req = "POST /html2/index.html HTTP/1.1\r\nHost: example.com\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nHello\r\n7\r\nWorld!\r\n2b\r\nStreaming raw data.\r\n0\r\n\r\n";

    // std::string req ="POST /html/ HTTP/1.1\r\nHost:  127.0.0.1:8080\r\n\r\n";
   std::string req = "POST /html2/ HTTP/1.1\r\n"
                  "Host: 127.0.0.1:8080\r\n"
                  "Content-Type: multipart/form-data; boundary=X-BORDER-12345\r\n"
                  "Content-Length: 300\r\n\r\n" // Note: Replace 264 with the actual total byte size of your body!

                  "--X-BORDER-12345\r\n"
                  "Content-Disposition: form-data; name=\"resume\"; filename=\"my_resume.pdf\"\r\n"
                  "Content-Type: application/pdf\r\n\r\n"
                  "[Raw PDF Data]\r\n"

                  "--X-BORDER-12345\r\n"
                  "Content-Disposition: form-data; name=\"cover_letter\"; filename=\"letter.docx\"\r\n"
                  "Content-Type: application/msword\r\n\r\n"
                  "[Raw Word Doc Data]\r\n"

                  "--X-BORDER-12345--\r\n";
    try 
    {
      Config config;
      config.load(configPath);   
        // std::cout <<config.getServers()[0].locations[0].redirect.code <<"\n";
    //   printf("im here \n");
    //   std::cout<<config.getServers()[0].error_pages[404]<<std::endl;
      Request R(NULL,req);
      ProcessRequest PS(R,config.getServers()[0]);
      ServeStaticRq Sc(R,PS,config.getServers()[0]);
      Response Rp (PS,Sc,R);
        //  ProcessCgi cg(NULL,PS, R);
        //  cg.GeneretCgiResponse();
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