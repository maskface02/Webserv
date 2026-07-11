/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lasoubai <lasoubai@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/12 21:38:25 by zatais            #+#    #+#             */
/*   Updated: 2026/07/11 16:28:01 by lasoubai         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/WebServ.hpp"

int main(int argc, char** argv)
{
    signal(SIGPIPE, SIG_IGN);
      signal(SIGINT, Server::signalHandler);
    std::string configPath = "conf/default.conf";
    if (argc > 1)
        configPath = argv[1];

    try {
       srand(time(NULL));
      Config config;
      config.load(configPath);

      Server server(config);
      server.run();
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
