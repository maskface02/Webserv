# Webserv Complete I/O Specification

> **Shared contract between Back-end and Front-end peers.**  
> Every phase: Input → Processing → Output, with exact data structures.

---

## Shared Header Contract (`include/WebservTypes.hpp`)

```cpp
#ifndef WEBSERV_TYPES_HPP
#define WEBSERV_TYPES_HPP

#include <string>
#include <vector>
#include <map>

// ==================== CONFIG SCHEMA ====================
struct Location {
    std::string                 path;
    std::string                 root;
    std::string                 index;
    bool                        autoindex;
    std::vector<std::string>    allow_methods;
    std::string                 upload_store;
    std::string                 cgi_extension;
    std::string                 cgi_path;
    std::string                 redirect;
    size_t                      client_max_body_size;
};

struct ServerConfig {
    std::string                 host;
    int                         port;
    std::string                 server_name;
    std::map<int, std::string>  error_pages;
    size_t                      client_max_body_size;
    std::vector<Location>       locations;
};

struct Config {
    std::vector<ServerConfig>   servers;
};

// ==================== CLIENT (Back-end owns) ====================
class Client {
public:
    // --- Back-end managed ---
    int         fd;
    std::string read_buffer;
    std::string write_buffer;
    int         state;          // 0=READING, 1=PROCESSING, 2=CGI_RUNNING, 3=WRITING, 4=DONE
    bool        keep_alive;
    int         server_idx;
    time_t      last_activity;

    // --- CGI (Back-end managed) ---
    int         cgi_pid;
    int         cgi_stdin_fd;
    int         cgi_stdout_fd;
    std::string cgi_input_buffer;
    std::string cgi_output_buffer;

    // --- Front-end managed (opaque to back-end) ---
    void*       request_obj;    // Front-end's HttpRequest*
    void*       response_obj;   // Front-end's HttpResponse*
};

// ==================== INTERFACE FUNCTIONS ====================
class FrontEnd {
public:
    static void handleRequest(Client& client, const Config& config);
    static void handleCgiComplete(Client& client, const Config& config, int exitStatus);
};

class BackEnd {
public:
    static void startCgi(Client& client,
                         const std::string& interpreter,
                         const std::string& scriptPath,
                         const std::map<std::string, std::string>& env);
    static void setClientWritable(Client& client);
};

#endif
```

---

## Phase 1: Foundation

| | Back-End | Front-End |
|---|---|---|
| **Input** | `int argc, char** argv` | `int argc, char** argv` |
| **Processing** | Validate args, instantiate `Server` class | Review `Shared.hpp` contract |
| **Output** | `Server server(configPath)` object created | — |
| **Data** | `class Server { Config config; std::vector<int> listenFds; }` | — |

```cpp
// main.cpp (owned by back-end)
int main(int argc, char** argv) {
    std::string configPath = (argc > 1) ? argv[1] : "conf/default.conf";
    Server server(configPath);
    server.run();
}
```

---

## Phase 2: Config Parser (Back-End)

| | |
|---|---|
| **Input** | `std::string configFilePath` |
| **Processing** | Open file, tokenize, validate syntax, fill structs |
| **Output** | `Config` object |

```cpp
// Input
std::string path = "conf/default.conf";

// Output
Config config;
config.servers[0].host = "127.0.0.1";
config.servers[0].port = 8080;
config.servers[0].locations[0].path = "/";
config.servers[0].locations[0].root = "./www";
```

**Validation Output:** `bool success`, `std::string errorMsg` (fatal if false).

---

## Phase 3: Session Store (Front-End)

| | |
|---|---|
| **Input** | None (self-contained singleton) |
| **Processing** | Generate IDs, store key-value maps |
| **Output** | `SessionManager` instance |

```cpp
class SessionManager {
public:
    static SessionManager& getInstance();

    // Input:  none
    // Output: std::string sessionId (16-char hex)
    std::string createSession();

    // Input:  std::string sessionId
    // Output: std::map<std::string, std::string>* (NULL if invalid)
    std::map<std::string, std::string>* getSession(const std::string& id);

    // Input:  std::string sessionId
    // Output: void
    void destroySession(const std::string& id);

private:
    std::map<std::string, std::map<std::string, std::string> > sessions;
};
```

---

## Phase 4: Socket Layer (Back-End)

| | |
|---|---|
| **Input** | `const Config& config` |
| **Processing** | `socket()`, `setsockopt()`, `bind()`, `listen()`, `fcntl(O_NONBLOCK)` |
| **Output** | `std::vector<int> listenFds` |

```cpp
// Input
const Config& config;

// Output
std::vector<int> listenFds; // e.g., [3, 4] for ports 8080 and 9090
```

---

## Phase 5: Poll Loop (Back-End)

| | |
|---|---|
| **Input** | `std::vector<int> listenFds` |
| **Processing** | `poll()` loop, `accept()`, `recv()`, `send()` |
| **Output** | Mutated `Client` objects |

```cpp
// Internal data structure (back-end only)
std::vector<Client*> clients;
std::vector<struct pollfd> pollFds;

// Event triggers:
// 1. New connection on listenFd=3
//    Output: Client* c = new Client(); c->fd = 5; c->state = 0;

// 2. POLLIN on client fd=5
//    Output: c->read_buffer += "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
//            When complete, call FrontEnd::handleRequest(*c, config);

// 3. POLLOUT on client fd=5
//    Output: send(c->fd, c->write_buffer.c_str(), c->write_buffer.size(), 0);
//            c->write_buffer.erase(0, bytesSent);
```

**Trigger to Front-End:** When `\r\n\r\n` found + full body received:
```cpp
FrontEnd::handleRequest(*client, config);
```

---

## Phase 6: HTTP Request Parsing (Front-End)

| | |
|---|---|
| **Input** | `const std::string& rawRequest` (from `client.read_buffer`) |
| **Processing** | Split request line, headers, body |
| **Output** | `HttpRequest` object |

```cpp
class HttpRequest {
public:
    std::string                         method;
    std::string                         uri;
    std::string                         queryString;
    std::string                         version;
    std::map<std::string, std::string>  headers;
    std::string                         body;
    std::map<std::string, std::string>  cookies;     // Bonus
    bool                                isChunked;
    size_t                              contentLength;
};

// Input
std::string raw = "GET /index.html?id=5 HTTP/1.1\r\nHost: localhost\r\nCookie: session=abc\r\n\r\n";

// Output
HttpRequest req;
req.method = "GET";
req.uri = "/index.html";
req.queryString = "id=5";
req.headers["Host"] = "localhost";
req.cookies["session"] = "abc";
```

**Storage:** Front-end allocates `HttpRequest*`, stores via `client.request_obj`:
```cpp
client.request_obj = new HttpRequest(req);
```

---

## Phase 7: Routing (Front-End)

| | |
|---|---|
| **Input** | `const HttpRequest& req`, `const Config& config`, `int serverIdx` |
| **Processing** | Match server block, match location prefix |
| **Output** | `RouteResult` struct |

```cpp
struct RouteResult {
    const Location*     location;
    const ServerConfig* server;
    std::string         filePath;
    bool                isCgi;
    std::string         cgiInterpreter;
    int                 statusCode;     // 0 if OK, else immediate error
};

// Input
const HttpRequest& req;
const Config& config;
int serverIdx = client.server_idx;

// Output
RouteResult route;
route.location = &config.servers[0].locations[2];
route.filePath = "./cgi-bin/test.php";
route.isCgi = true;
route.cgiInterpreter = "/usr/bin/php-cgi";
```

---

## Phase 8: Static File Serving (Front-End)

| | |
|---|---|
| **Input** | `const RouteResult& route` |
| **Processing** | `open()`, `read()`, MIME type detection |
| **Output** | `std::string fileContent`, `std::string mimeType`, `int statusCode` |

```cpp
// Input
std::string path = "./www/index.html";

// Output
std::string body = "<html>...</html>";
std::string mime = "text/html";
int code = 200;

// Failure Output:
body = "";
mime = "";
code = 404; // or 403
```

---

## Phase 9: Error Pages (Front-End)

| | |
|---|---|
| **Input** | `int statusCode`, `const ServerConfig& server` |
| **Processing** | Look up custom error page path, read file, or generate HTML |
| **Output** | `std::string errorBody` |

```cpp
// Input
int code = 404;
const ServerConfig& server = config.servers[0];

// Processing
std::map<int, std::string>::const_iterator it = server.error_pages.find(404);

// Output
std::string body = "<html><body><center><h1>404 Not Found</h1></center></body></html>";
```

---

## Phase 10: Response Builder with Cookies (Front-End)

| | |
|---|---|
| **Input** | `int statusCode`, `std::string body`, `std::string mimeType`, `const HttpRequest* req` |
| **Processing** | Build status line, add headers, inject Set-Cookie if needed |
| **Output** | `std::string rawResponse` written to `client.write_buffer` |

```cpp
class HttpResponse {
public:
    int                                 statusCode;
    std::map<std::string, std::string>  headers;
    std::string                         body;

    std::string build() {
        std::string res = "HTTP/1.1 " + statusToString(statusCode) + "\r\n";
        for (std::map<std::string, std::string>::const_iterator it = headers.begin();
             it != headers.end(); ++it) {
            res += it->first + ": " + it->second + "\r\n";
        }
        res += "\r\n" + body;
        return res;
    }
};

// Example output written to client.write_buffer:
// HTTP/1.1 200 OK\r\n
// Content-Type: text/html\r\n
// Content-Length: 123\r\n
// Set-Cookie: session_id=a3f7e2; Path=/\r\n
// Connection: keep-alive\r\n
// \r\n
// <html>...</html>
```

**Side Effect:** Front-end sets `client.keep_alive = true/false` and calls:
```cpp
BackEnd::setClientWritable(client); // Tells back-end to add POLLOUT
```

---

## Phase 11: CGI (Split Between Peers)

### Part A: Front-End Detects & Prepares

| | |
|---|---|
| **Input** | `const RouteResult& route`, `const HttpRequest& req` |
| **Processing** | Build env vars, determine interpreter |
| **Output** | Call to back-end |

```cpp
// Input
const RouteResult& route; // isCgi=true, cgiInterpreter="/usr/bin/php-cgi"
const HttpRequest& req;

// Processing: Build environment map
std::map<std::string, std::string> env;
env["REQUEST_METHOD"] = req.method;
env["QUERY_STRING"] = req.queryString;
env["CONTENT_LENGTH"] = sizeToString(req.body.size());
env["CONTENT_TYPE"] = req.headers["Content-Type"];
env["SCRIPT_NAME"] = req.uri;
env["PATH_INFO"] = req.uri;
env["HTTP_COOKIE"] = req.headers["Cookie"];

// Output: Ask back-end to fork
BackEnd::startCgi(client, route.cgiInterpreter, route.filePath, env);
```

### Part B: Back-End Executes

| | |
|---|---|
| **Input** | `Client&`, `interpreter`, `scriptPath`, `envMap` |
| **Processing** | `pipe()`, `fork()`, `dup2()`, `execve()`, add pipes to poll |
| **Output** | `client.cgi_pid`, `client.cgi_stdin_fd`, `client.cgi_stdout_fd` |

```cpp
// Output (mutated client)
client.cgi_pid = 12345;
client.cgi_stdin_fd = 6;
client.cgi_stdout_fd = 7;
client.state = 2; // CGI_RUNNING
```

### Part C: Back-End Reads CGI Output

| | |
|---|---|
| **Input** | `POLLIN` on `client.cgi_stdout_fd` |
| **Processing** | `read()` into `client.cgi_output_buffer` |
| **Output** | Full CGI output stored in `client.cgi_output_buffer` |

### Part D: Back-End Notifies Completion

| | |
|---|---|
| **Input** | Child exited (`waitpid` returned PID) |
| **Processing** | Clean up pipes, remove from poll set |
| **Output** | Call to front-end |

```cpp
FrontEnd::handleCgiComplete(client, config, exitStatus);
```

### Part E: Front-End Formats CGI Output

| | |
|---|---|
| **Input** | `const std::string& cgiOutput` (from `client.cgi_output_buffer`) |
| **Processing** | Split CGI headers from body, wrap in HTTP response |
| **Output** | `client.write_buffer` filled with final HTTP response |

```cpp
// Input
std::string cgiOutput = "Content-type: text/html\r\n\r\n<html>Hello</html>";

// Processing
std::string cgiHeaders, cgiBody;
size_t pos = cgiOutput.find("\r\n\r\n");
if (pos != std::string::npos) {
    cgiHeaders = cgiOutput.substr(0, pos);
    cgiBody = cgiOutput.substr(pos + 4);
}

// Output
HttpResponse res;
res.statusCode = 200;
res.headers["Content-Type"] = "text/html";
res.headers["Content-Length"] = sizeToString(cgiBody.size());
res.body = cgiBody;

client.write_buffer = res.build();
client.state = 3; // WRITING
BackEnd::setClientWritable(client);
```

---

## Phase 12: POST & File Uploads (Front-End)

| | |
|---|---|
| **Input** | `const HttpRequest& req`, `const Location& loc` |
| **Processing** | Parse `multipart/form-data` or `application/x-www-form-urlencoded` |
| **Output** | `std::vector<SavedFile>` or form data map |

```cpp
struct UploadedFile {
    std::string filename;
    std::string contentType;
    std::string data;
};

// Input
const HttpRequest& req;
const Location& loc; // upload_store = "./www/uploads"

// Processing
std::vector<UploadedFile> files = parseMultipart(req.body, boundary);

// Output: Write to disk
for (size_t i = 0; i < files.size(); ++i) {
    std::string path = loc.upload_store + "/" + files[i].filename;
    writeFile(path, files[i].data);
}

// Response
HttpResponse res;
res.statusCode = 201;
res.body = "<html><h1>Upload Successful</h1></html>";
client.write_buffer = res.build();
```

---

## Phase 13: DELETE (Front-End)

| | |
|---|---|
| **Input** | `const RouteResult& route` (resolved filePath) |
| **Processing** | `unlink(filePath)` |
| **Output** | `HttpResponse` |

```cpp
// Input
std::string path = route.filePath; // "./www/uploads/old.txt"

// Processing
int result = unlink(path.c_str());

// Output
HttpResponse res;
if (result == 0) {
    res.statusCode = 204;
} else {
    res.statusCode = (errno == ENOENT) ? 404 : 403;
}
client.write_buffer = res.build();
```

---

## Phase 14: Redirection (Front-End)

| | |
|---|---|
| **Input** | `const Location& loc` (loc.redirect = "/new-path") |
| **Processing** | Build 301/302 response |
| **Output** | `HttpResponse` with `Location` header |

```cpp
// Input
loc.redirect = "http://localhost:8080/new";
loc.path = "/old";

// Output
HttpResponse res;
res.statusCode = 301;
res.headers["Location"] = loc.redirect;
res.body = "<html><body>Moved permanently</body></html>";
client.write_buffer = res.build();
```

---

## Phase 15: Connection Management (Back-End)

| | |
|---|---|
| **Input** | `Client& client` (after write_buffer empty) |
| **Processing** | Check `client.keep_alive` |
| **Output** | Client state transition or close |

```cpp
// Input
Client& c;
// c.write_buffer is now empty
// c.keep_alive set by front-end

// Processing & Output
if (c.keep_alive) {
    c.state = 0;        // READING
    c.read_buffer.clear();
    c.last_activity = now();
} else {
    close(c.fd);
    // Remove from poll set, delete Client object
}
```

---

## Phase 16: Multiple Ports (Back-End)

| | |
|---|---|
| **Input** | `const Config& config` (multiple server blocks) |
| **Processing** | Create socket per unique `host:port` |
| **Output** | `std::map<int, int> fdToServerIdx` |

```cpp
// Input
Config config; // 2 servers: 127.0.0.1:8080 and 127.0.0.1:9090

// Output
std::vector<int> listenFds;     // [3, 4]
std::map<int, int> fdToServer;  // {3: 0, 4: 1}

// On accept():
int serverIdx = fdToServer[listenFd];
Client* c = new Client();
c->server_idx = serverIdx;
```

---

## Phase 17: Demo Pages (Front-End)

| | |
|---|---|
| **Input** | HTML/CSS/JS files in `www/` |
| **Processing** | Front-end writes these files |
| **Output** | Static assets served by Phase 8 |

No code I/O — content creation. Files must exist on disk for back-end to `open()`.

---

## Phase 18: Testing (Both)

| Peer | Input | Output |
|------|-------|--------|
| **Back-end** | `siege`, `valgrind`, `telnet` raw bytes | `std::cerr` logs, `0` exit code, no leaks |
| **Front-end** | Browser dev tools, curl, telnet | Valid HTTP responses, correct headers |

---

## Master Interface Summary

```cpp
// BACK-END → FRONT-END (function calls)
void FrontEnd::handleRequest(Client& client, const Config& config);
void FrontEnd::handleCgiComplete(Client& client, const Config& config, int exitStatus);

// FRONT-END → BACK-END (function calls)
void BackEnd::startCgi(Client& client,
                       const std::string& interpreter,
                       const std::string& scriptPath,
                       const std::map<std::string, std::string>& env);
void BackEnd::setClientWritable(Client& client);

// SHARED MUTATION (Front-end writes, Back-end reads)
client.write_buffer       // Front-end fills, Back-end sends
client.keep_alive         // Front-end sets, Back-end reads
client.state              // Front-end sets (3=WRITING), Back-end reads
client.request_obj        // Front-end owns (HttpRequest*)
client.response_obj       // Front-end owns (HttpResponse*)

// SHARED MUTATION (Back-end writes, Front-end reads)
client.read_buffer        // Back-end fills, Front-end consumes
client.cgi_output_buffer  // Back-end fills, Front-end consumes
client.server_idx         // Back-end sets on accept, Front-end reads
```
