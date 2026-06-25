#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <errno.h>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <cstdlib>

struct Client {
    std::string cgi_output_buffer;
    pid_t cgi_pid;
    time_t last_activity;
    bool cgi_done;
    
    Client() : cgi_pid(-1), last_activity(0), cgi_done(false) {}
};

void cleanupCgi(Client* client, int status) {
    client->cgi_done = true;
    std::cout << "[cleanup] child exited with status=" << status 
              << " total_bytes=" << client->cgi_output_buffer.size() << std::endl;
}

void handleCgiOutput(Client* client, int pipe_fd) {
    const size_t MAX_PER_CYCLE = 64 * 1024;
    size_t read_this_cycle = 0;

    while (read_this_cycle < MAX_PER_CYCLE) {
        char buffer[4096];
        ssize_t bytes = read(pipe_fd, buffer, sizeof(buffer));

        if (bytes > 0) {
            client->cgi_output_buffer.append(buffer, bytes);
            client->last_activity = time(NULL);
            read_this_cycle += bytes;
        }
        else if (bytes == 0) {
            int status;
            pid_t result = waitpid(client->cgi_pid, &status, 0);
            if (result > 0)
                cleanupCgi(client, WEXITSTATUS(status));
            else
                cleanupCgi(client, -1);
            return;
        }
        else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            }
            waitpid(client->cgi_pid, NULL, WNOHANG);
            cleanupCgi(client, -1);
            return;
        }
    }

    std::cout << "[yield] read " << read_this_cycle 
              << " bytes this cycle, total buffered: " 
              << client->cgi_output_buffer.size() << std::endl;
}

int spawnSlowCgi(pid_t& out_pid, size_t total_bytes) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }
    
    int flags = fcntl(pipefd[0], F_GETFL, 0);
    fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);
    
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(1);
    }
    
    if (pid == 0) {
        close(pipefd[0]);
        
        // Write a recognizable pattern so you can verify the file
        std::string header = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n";
        write(pipefd[1], header.c_str(), header.size());
        
        size_t body_bytes = total_bytes > header.size() ? total_bytes - header.size() : 0;
        std::string chunk(4096, 'X');
        size_t written = 0;
        while (written < body_bytes) {
            size_t to_write = (body_bytes - written < chunk.size()) 
                              ? (body_bytes - written) : chunk.size();
            ssize_t n = write(pipefd[1], chunk.c_str(), to_write);
            if (n < 0) {
                perror("write");
                break;
            }
            written += n;
            usleep(1000);
        }
        
        close(pipefd[1]);
        _exit(42);
    }
    
    close(pipefd[1]);
    out_pid = pid;
    return pipefd[0];
}

int main() {
    std::cout << "=== Testing CGI 64KB-yield handler ===" << std::endl;
    
    Client client;
    size_t target_size = 500 * 1024;
    pid_t pid;
    int fd = spawnSlowCgi(pid, target_size);
    client.cgi_pid = pid;
    
    int poll_cycles = 0;
    while (!client.cgi_done && poll_cycles < 1000) {
        handleCgiOutput(&client, fd);
        poll_cycles++;
        if (!client.cgi_done)
            usleep(500);
    }
    close(fd);
    
    std::cout << "\n=== Results ===" << std::endl;
    std::cout << "poll() cycles: " << poll_cycles << std::endl;
    std::cout << "expected size: " << target_size << std::endl;
    std::cout << "buffered size: " << client.cgi_output_buffer.size() << std::endl;
    std::cout << "match: " << (client.cgi_output_buffer.size() == target_size ? "YES" : "NO") << std::endl;
    
    // ============================================
    // WRITE BUFFER TO FILE
    // ============================================
    if (client.cgi_done && !client.cgi_output_buffer.empty()) {
        const char* filename = "cgi_output_dump.bin";
        std::ofstream out_file(filename, std::ios::binary);
        if (out_file.is_open()) {
            out_file.write(client.cgi_output_buffer.c_str(), 
                           client.cgi_output_buffer.size());
            out_file.close();
            std::cout << "\n[file] CGI output written to: " << filename << std::endl;
            std::cout << "[file] Size on disk: " << client.cgi_output_buffer.size() << " bytes" << std::endl;
            
            // Verify first few bytes
            std::cout << "[file] First 50 bytes: \"" 
                      << client.cgi_output_buffer.substr(0, 50) << "\"" << std::endl;
        } else {
            std::cerr << "[file] Failed to open " << filename << " for writing" << std::endl;
        }
    }
    
    return 0;
}
