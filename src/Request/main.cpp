/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 19:54:58 by lasoubai          #+#    #+#             */
/*   Updated: 2026/05/24 23:47:58 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/httprequest.hpp"



int main(void)
{
std::string req = "POST /stream HTTP/1.1\r\nHost: example.com\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nHello\r\n7\r\nWorld!\r\n2b\r\nStreaming raw data.\r\n0\r\n\r\n";
// "POST /index.html?q=1 HTTP/1.1\r\nHost: localhost\r\nContent-Type: text/plain\r\nContent-Length: 10\r\n\r\nThis is the body";
 
    try
    {
        Request R(req);
    }
    catch(const Request::HttpError& e)
    {
        std::cerr <<e.getErrorCode()<<" "<<e.what()<<std::endl;
        //clear
    }

}
