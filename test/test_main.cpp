#include "../include/WebServ.hpp"
#include <iostream>
#include <string>

void printConfig(const Config& cfg) {
    std::vector<ServerConfig> servers = cfg.getServers();
    
    for (size_t i = 0; i < servers.size(); ++i) {
        std::cout << "========================================\n";
        std::cout << "SERVER " << i + 1 << "\n";
        std::cout << "========================================\n";
        
        const ServerConfig& srv = servers[i];
        
        std::cout << "Listens:\n";
        for (size_t j = 0; j < srv.listens.size(); ++j) {
            std::cout << "  - " << srv.listens[j].host << ":" << srv.listens[j].port << "\n";
        }
        
        std::cout << "client_max_body_size: " << srv.client_max_body_size << "\n";
        
        std::cout << "Error pages:\n";
        for (std::map<int, std::string>::const_iterator it = srv.error_pages.begin(); it != srv.error_pages.end(); ++it) {
            std::cout << "  - " << it->first << " -> " << it->second << "\n";
        }
        
        std::cout << "Locations (" << srv.locations.size() << "):\n";
        for (size_t j = 0; j < srv.locations.size(); ++j) {
            const Location& loc = srv.locations[j];
            std::cout << "  [" << loc.path << "]\n";
            if (!loc.root.empty())
                std::cout << "    root: " << loc.root << "\n";
            if (!loc.index.empty()) {
                std::cout << "    index: ";
                for (size_t k = 0; k < loc.index.size(); ++k)
                    std::cout << loc.index[k] << " ";
                std::cout << "\n";
            }
            std::cout << "    autoindex: " << (loc.autoindex ? "on" : "off") << "\n";
            if (!loc.allow_methods.empty()) {
                std::cout << "    allow_methods: ";
                for (size_t k = 0; k < loc.allow_methods.size(); ++k)
                    std::cout << loc.allow_methods[k] << " ";
                std::cout << "\n";
            }
            std::cout << "    client_max_body_size: " << loc.client_max_body_size << "\n";
            if (loc.upload_enabled && !loc.upload_store.empty())
                std::cout << "    upload_store: " << loc.upload_store << "\n";
            if (!loc.cgi.empty()) {
                std::cout << "    cgi:\n";
                for (std::map<std::string, std::string>::const_iterator it = loc.cgi.begin(); it != loc.cgi.end(); ++it)
                    std::cout << "      " << it->first << " -> " << it->second << "\n";
            }
            if (loc.redirect.enabled)
                std::cout << "    redirect: " << loc.redirect.code << " " << loc.redirect.url << "\n";
        }
        std::cout << "\n";
    }
}

int main(int argc, char** argv) {
  std::string configPath = "test/test.conf";
    
    if (argc > 1)
        configPath = argv[1];
    
    try {
        Config cfg;
        cfg.load(configPath);
        std::cout << "Config loaded successfully!\n\n";
        printConfig(cfg);
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
