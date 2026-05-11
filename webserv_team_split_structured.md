# Webserv Team Split — Structured by Request Lifecycle

> **Project:** Webserv (42 School)  
> **Team Size:** 2 peers  
> **Split Model:** Back-End (Engine/I/O) + Front-End (Protocol/Logic)  
> **Based on:** `Webserv_Request_Lifecycle.txt`  
> **Generated:** 2026-05-07

---

## The Golden Rule

| Back-End | Front-End |
|----------|-----------|
| Owns **file descriptors, processes, network layer** | Owns **HTTP protocol, content logic, business rules** |
| "I move bytes through sockets and pipes" | "I turn bytes into HTTP requests and responses" |
| Never parses HTTP headers | Never touches `poll()`, `socket()`, `fork()` |

---

## Core Data Structures (From Lifecycle)

### `Client` (Shared Mutable State)
```cpp
class Client {
public:
    // --- Back-end managed ---
    int         fd;                     // e.g., 5 (from accept)
    std::string read_buffer;            // raw HTTP bytes from recv()
    std::string write_buffer;           // complete HTTP response to send()
    int         state;                  // 0=READING, 1=PROCESSING, 2=CGI_RUNNING, 3=WRITING
    bool        keep_alive;             // true by default (HTTP/1.1)
    int         server_idx;             // 0 = first server block in config
    time_t      last_activity;          // for timeout logic

    // --- CGI (Back-end managed) ---
    int         cgi_pid;                // child PID after fork()
    int         cgi_stdin_fd;           // pipe write-end (server -> CGI)
    int         cgi_stdout_fd;          // pipe read-end (CGI -> server)
    std::string cgi_input_buffer;       // POST body to feed CGI
    std::string cgi_output_buffer;      // raw output from CGI stdout pipe

    // --- Front-end managed (opaque to back-end) ---
    void*       request_obj;            // HttpRequest* (Phase 5)
    void*       response_obj;           // HttpResponse* (Phase 10)
};
```

### `HttpRequest` (Front-End only)
```cpp
class HttpRequest {
public:
    std::string                         method;         // "GET", "POST"
    std::string                         uri;            // "/login.php"
    std::string                         queryString;    // "redirect=dashboard"
    std::string                         version;        // "HTTP/1.1"
    std::map<std::string, std::string>  headers;        // Host, Cookie, etc.
    std::string                         body;           // request body
    std::map<std::string, std::string>  cookies;        // parsed Cookie header
    bool                                isChunked;
    size_t                              contentLength;
};
```

### `HttpResponse` (Front-End only)
```cpp
class HttpResponse {
public:
    int                                 statusCode;     // 200, 404, etc.
    std::map<std::string, std::string>  headers;        // Content-Type, Set-Cookie
    std::string                         body;           // HTML or file bytes

    std::string build();    // serializes to raw HTTP response string
};
```

### `RouteResult` (Front-End only)
```cpp
struct RouteResult {
    const Location*     location;           // matched location block
    const ServerConfig* server;             // matched server block
    std::string         filePath;           // "./www/login.php"
    bool                isCgi;              // true if extension matches
    std::string         cgiInterpreter;     // "/usr/bin/php-cgi"
    int                 statusCode;         // 0 if OK, else immediate error
};
```

---

## Phase-by-Phase Breakdown

### Phase 1: Foundation & Project Skeleton

| Task | Owner | Deliverable |
|------|-------|-------------|
| Create folder structure (`src/`, `include/`, `conf/`, `www/`, `cgi-bin/`) | **Both** (agree Day 1) | Matching paths in both peers' code |
| Write `Makefile` with `c++ -Wall -Wextra -Werror -std=c++98` | **Back-End** | Compiles both peers' code |
| Write `main.cpp` entry point | **Back-End** | `./webserv [config_file]` |
| Define `Shared.hpp` contract (structs above) | **Both** (Day 1) | `Config`, `ServerConfig`, `Location`, `Client` structs |

**Integration Checkpoint:** Empty binary compiles and exits cleanly.

---

### Phase 2: Configuration File Parser

| Task | Owner | Why |
|------|-------|-----|
| Read file, tokenize (`{}`, `;`, `#`) | **Back-End** | File I/O, boot-time urgency |
| Validate syntax (balanced braces, semicolons) | **Back-End** | Structural validation |
| Validate HTTP semantics (methods, paths, redirects) | **Front-End** | Knows valid HTTP values |
| Store in `Config` object | **Back-End** | Owns the instance |
| Default values / fallbacks | **Front-End** | Knows what responses need |

**Back-End Output:** `Config` object with all parsed data.  
**Front-End Input:** `const Config&` (read-only reference).

**Data Example:**
```cpp
Config config;
config.servers[0].host = "127.0.0.1";
config.servers[0].port = 8080;
config.servers[0].locations[0].path = "/";
config.servers[0].locations[0].root = "./www";
config.servers[0].locations[0].cgi_extension = ".php";
config.servers[0].locations[0].cgi_path = "/usr/bin/php-cgi";
```

**Integration Checkpoint:** `./webserv conf/default.conf` parses and prints config.

---

### Phase 3: Session Store Infrastructure (Bonus)

| Task | Owner | Details |
|------|-------|---------|
| `SessionManager` singleton | **Front-End** | Pure HTTP state |
| `createSession()` -- random 16-char hex ID | **Front-End** | e.g., `"b8e3f1a2c5d9"` |
| `getSession(id)` -- retrieve data | **Front-End** | Returns `std::map*` |
| `destroySession(id)` -- cleanup | **Front-End** | Expiration logic |

**I/O Example:**
```cpp
// Input:  none
// Output: std::string sessionId = "b8e3f1a2c5d9";

// Input:  std::string sessionId = "b8e3f1a2c5d9";
// Output: std::map<std::string, std::string>* data;
//         data["theme"] = "light";
```

**Integration Checkpoint:** Unit test: create -> store -> retrieve -> destroy.

---

### Phase 4: Socket Layer

| Task | Owner | Details |
|------|-------|---------|
| Loop `Config.servers`, extract unique `host:port` | **Back-End** | Needs config at boot |
| `socket()`, `setsockopt(SO_REUSEADDR)` | **Back-End** | Socket creation |
| `bind()` to address + port | **Back-End** | Network layer |
| `listen()` with backlog | **Back-End** | Queue connections |
| `fcntl(fd, F_SETFL, O_NONBLOCK)` on ALL fds | **Back-End** | **Critical!** |
| Store listening fds in vector | **Back-End** | `std::vector<int> listenFds` |

**Data Example:**
```cpp
// Output
std::vector<int> listenFds; // e.g., [3] for port 8080
```

**Integration Checkpoint:** `netstat -tlnp` shows ports listening.

---

### Phase 5: The Single Poll Loop (The Engine)

| Task | Owner | Details |
|------|-------|---------|
| Build `pollfd` array from all sockets | **Back-End** | Manages the array |
| Main `poll()` loop with timeout | **Back-End** | Server heartbeat |
| `accept_new_client()` -- create `Client`, add to poll | **Back-End** | Connection acceptance |
| `read_from_client()` -- `recv()` into `read_buffer` | **Back-End** | Raw byte reading |
| Detect complete request (`\r\n\r\n` + body) | **Back-End** | Simple boundary check |
| **Trigger `FrontEnd::handleRequest()`** | **Back-End** | **Handoff to front-end** |
| `write_to_client()` -- `send()` from `write_buffer` | **Back-End** | Raw byte writing |
| Handle partial writes, keep `POLLOUT` | **Back-End** | Non-blocking management |
| `close_client()` -- remove from poll, cleanup | **Back-End** | Connection teardown |
| Client timeout logic | **Back-End** | Close inactive connections |

**The Handoff:** When `client->read_buffer` contains a complete HTTP request, Back-End calls:
```cpp
FrontEnd::handleRequest(Client& client, const Config& config);
```

**Data Flow -- New Connection:**
```cpp
// Input:  listenFd = 3 has POLLIN
// Processing: accept(3, &client_addr, &len) -> fd = 5
// Output:
Client* c = new Client();
c->fd = 5;
c->read_buffer = "";
c->write_buffer = "";
c->state = 0;           // READING
c->server_idx = 0;      // first server block
c->keep_alive = true;
// Add fd=5 to poll array with POLLIN
```

**Data Flow -- Reading:**
```cpp
// Input:  fd=5 has POLLIN
// Processing: recv(5, buf, 4096, 0) -> partial data
// Output: client->read_buffer += partial_data
//         Repeat until \r\n\r\n found, then handoff
```

**Integration Checkpoint:** `telnet localhost 8080` connects, server accepts.

---

### Phase 6: HTTP Request Parsing (Front-End Entry Point)

| Task | Owner | Details |
|------|-------|---------|
| `HttpRequest` class | **Front-End** | Owns the class |
| Parse request line: `METHOD URI HTTP/VERSION` | **Front-End** | Split & validate |
| Parse URI: path + query string | **Front-End** | `uri`, `queryString` |
| Parse headers into `std::map<string, string>` | **Front-End** | Key-value pairs |
| Extract `Host`, `Content-Length`, `Transfer-Encoding`, `Connection` | **Front-End** | Critical headers |
| **Bonus:** Parse `Cookie` header | **Front-End** | `cookies["session_id"] = "abc123"` |
| Handle chunked transfer encoding | **Front-End** | Un-chunk body |
| Read body based on `Content-Length` | **Front-End** | Wait for full body |
| Validate: method, HTTP version, URI | **Front-End** | Return 400/405 |
| Store `HttpRequest*` in `client->request_obj` | **Front-End** | Cast to `void*` |

**Input:** `client->read_buffer` (raw bytes from browser).  
**Output:** `HttpRequest` object stored in `client->request_obj`.

**Data Example:**
```cpp
// Input (from client->read_buffer):
// "GET /login.php?redirect=dashboard HTTP/1.1\r\nHost: localhost\r\n\r\n"

// Output:
HttpRequest req;
req.method = "GET";
req.uri = "/login.php";
req.queryString = "redirect=dashboard";
req.version = "HTTP/1.1";
req.headers["Host"] = "localhost";
req.cookies = {};  // empty on first visit

client.request_obj = new HttpRequest(req);
```

**Integration Checkpoint:** Send raw HTTP via telnet, front-end prints parsed request.

---

### Phase 7: Routing

| Task | Owner | Details |
|------|-------|---------|
| Match `Host:port` to `ServerConfig` | **Front-End** | Use `client->server_idx` |
| Match URI to longest `location` prefix | **Front-End** | Iterative matching |
| Check `allow_methods` -> 405 if invalid | **Front-End** | Per-location method check |
| Extract `root`, `index`, `autoindex`, `redirect` | **Front-End** | Location rules |
| Extract **Bonus:** `cgi_extension`, `cgi_path` | **Front-End** | CGI config |
| Build `RouteResult` | **Front-End** | Return matched location + path |

**Input:** `HttpRequest*`, `const Config&`, `client->server_idx`.  
**Output:** `RouteResult` with `isCgi`, `filePath`, `cgiInterpreter`.

**Data Example:**
```cpp
// Input:  req.uri = "/login.php", client.server_idx = 0
// Output:
RouteResult route;
route.location = &config.servers[0].locations[0];  // location /
route.server = &config.servers[0];
route.filePath = "./www/login.php";
route.isCgi = true;
route.cgiInterpreter = "/usr/bin/php-cgi";
route.statusCode = 0;  // OK
```

**Integration Checkpoint:** Different URIs route to correct locations.

---

### Phase 8: Static File Serving

| Task | Owner | Details |
|------|-------|---------|
| Build file path: `root + URI` | **Front-End** | Path resolution |
| `open()`/`read()` file into string | **Front-End** | Regular file I/O |
| Determine MIME type from extension | **Front-End** | `.html`->`text/html`, `.css`->`text/css` |
| Directory handling: `index`, `autoindex`, 403 | **Front-End** | Directory logic |
| 404 if file not found | **Front-End** | Error detection |

**Data Example:**
```cpp
// Input:  route.filePath = "./www/index.html"
// Output:
std::string body = "<html>...</html>";
std::string mime = "text/html";
int code = 200;
```

**Integration Checkpoint:** Browser loads `index.html` with CSS/images.

---

### Phase 9: Error Pages

| Task | Owner | Details |
|------|-------|---------|
| Map status codes to custom error pages | **Front-End** | `error_page 404 /404.html` |
| Read custom error page from disk | **Front-End** | File serving |
| Generate default HTML if missing | **Front-End** | Inline HTML |
| All status codes: 200, 201, 204, 301, 302, 400, 403, 404, 405, 413, 500, 502, 504 | **Front-End** | Complete coverage |

**Integration Checkpoint:** Non-existent file -> 404 page in browser.

---

### Phase 10: Response Builder with Cookies (Bonus)

| Task | Owner | Details |
|------|-------|---------|
| `HttpResponse` class | **Front-End** | Owns the class |
| Build status line + headers + body | **Front-End** | `res.build()` |
| Check `Cookie` for `session_id` | **Front-End** | Session validation |
| No session -> `SessionManager::createSession()` | **Front-End** | New 16-char hex ID |
| Add `Set-Cookie` header | **Front-End** | `Set-Cookie: session_id=xyz; Path=/` |
| Valid session -> attach data | **Front-End** | Personalization |
| Fill `client->write_buffer` | **Front-End** | Write to shared buffer |
| Set `client->keep_alive` and `state = WRITING` | **Front-End** | Signal ready |
| Call `BackEnd::setClientWritable(client)` | **Front-End** | Ask for `POLLOUT` |

**Input:** Status code, body, MIME type, `HttpRequest*`.  
**Output:** Complete HTTP response string in `client->write_buffer`.

**Data Example:**
```cpp
// Input:  status=200, body="<html>...", mime="text/html", req has no cookie
// Processing:
std::string sessionId = SessionManager::getInstance().createSession(); // "b8e3f1a2c5d9"

// Output (written to client.write_buffer):
// HTTP/1.1 200 OK\r\n
// Content-Type: text/html\r\n
// Content-Length: 123\r\n
// Set-Cookie: session_id=b8e3f1a2c5d9; Path=/\r\n
// Connection: keep-alive\r\n
// \r\n
// <html>...</html>

client.keep_alive = true;
client.state = 3; // WRITING
BackEnd::setClientWritable(client);
```

**Integration Checkpoint:** First request gets `Set-Cookie`, second sends `Cookie`.

---

### Phase 11: CGI with Multiple Types (Bonus)

#### Part A: Front-End Detects & Prepares

| Task | Owner | Details |
|------|-------|---------|
| Detect CGI: URI matches `cgi_extension` | **Front-End** | Extension check |
| Resolve interpreter: `cgi_path` from location | **Front-End** | `.php`->`php-cgi`, `.py`->`python3` |
| Build env vars: `REQUEST_METHOD`, `QUERY_STRING`, etc. | **Front-End** | Environment map |
| **Bonus:** `HTTP_COOKIE` | **Front-End** | Pass cookies to CGI |
| Call `BackEnd::startCgi()` | **Front-End** | Handoff to back-end |

**Data Example:**
```cpp
// Input:  route.isCgi=true, route.cgiInterpreter="/usr/bin/php-cgi"
//         req.method="GET", req.queryString="redirect=dashboard"
// Output: Environment map
std::map<std::string, std::string> env;
env["REQUEST_METHOD"] = "GET";
env["QUERY_STRING"] = "redirect=dashboard";
env["CONTENT_LENGTH"] = "0";
env["CONTENT_TYPE"] = req.headers["Content-Type"];
env["SCRIPT_NAME"] = "/login.php";
env["PATH_INFO"] = "/login.php";
env["PATH_TRANSLATED"] = absolutePath(route.filePath);
env["HTTP_HOST"] = req.headers["Host"];
env["HTTP_USER_AGENT"] = req.headers["User-Agent"];
env["HTTP_COOKIE"] = req.headers["Cookie"];  // empty on first visit

BackEnd::startCgi(client, route.cgiInterpreter, route.filePath, env);
```

#### Part B: Back-End Forks & Executes

| Task | Owner | Details |
|------|-------|---------|
| `pipe()` stdin + stdout | **Back-End** | 4 fds total |
| `fcntl(O_NONBLOCK)` on pipes | **Back-End** | Non-blocking pipes |
| Add pipe fds to poll set | **Back-End** | Monitor CGI output |
| `fork()` + `dup2()` + `execve()` | **Back-End** | Spawn process |
| Feed POST body to CGI stdin | **Back-End** | Wait for `POLLOUT` |
| Read CGI stdout into buffer | **Back-End** | Wait for `POLLIN` |
| Detect exit via `waitpid()` | **Back-End** | Reap child |
| Call `FrontEnd::handleCgiComplete()` | **Back-End** | Handoff to front-end |
| Cleanup pipes, remove from poll | **Back-End** | Resource cleanup |

**Data Example:**
```cpp
// Output (mutated client):
client.cgi_pid = 12345;
client.cgi_stdin_fd = 6;
client.cgi_stdout_fd = 7;
client.state = 2; // CGI_RUNNING
// cgi_stdout_fd added to poll array with POLLIN
```

#### Part C: Back-End Reads CGI Output

| Task | Owner | Details |
|------|-------|---------|
| `POLLIN` on `client.cgi_stdout_fd` | **Back-End** | Pipe has data |
| `read()` into `client.cgi_output_buffer` | **Back-End** | Loop until EAGAIN/EOF |
| Detect EOF (`read() == 0`) | **Back-End** | CGI finished writing |
| `waitpid()` to reap child | **Back-End** | Get exit status |

**Data Example:**
```cpp
// Input:  CGI stdout pipe has POLLIN
// Output: client.cgi_output_buffer += "Content-type: text/html\r\n\r\n<html>..."
```

#### Part D: Back-End Notifies Completion

| Task | Owner | Details |
|------|-------|---------|
| Child exited (`waitpid` returned PID) | **Back-End** | Reap child |
| Clean up pipes, remove from poll set | **Back-End** | Close fds 6 & 7 |
| Call front-end handler | **Back-End** | Handoff |

```cpp
FrontEnd::handleCgiComplete(client, config, exitStatus);
```

#### Part E: Front-End Formats CGI Output

| Task | Owner | Details |
|------|-------|---------|
| Parse CGI output: split headers/body | **Front-End** | Handle `Content-type` |
| No `Content-Length` -> EOF marks end | **Front-End** | Subject rule |
| Wrap into HTTP response | **Front-End** | Run through Phase 10 |

**Data Example:**
```cpp
// Input:  client.cgi_output_buffer
//         "Content-type: text/html\r\n\r\n<html><body><form>...</form></body></html>"

// Processing:
size_t pos = client.cgi_output_buffer.find("\r\n\r\n");
std::string cgiHeaders = client.cgi_output_buffer.substr(0, pos);
std::string cgiBody = client.cgi_output_buffer.substr(pos + 4);

// Output:
HttpResponse res;
res.statusCode = 200;
res.headers["Content-Type"] = "text/html";
res.headers["Content-Length"] = sizeToString(cgiBody.size());
res.body = cgiBody;

client.write_buffer = res.build();
client.state = 3; // WRITING
BackEnd::setClientWritable(client);
```

**Integration Checkpoint:** `.php` and `.py` both execute in browser.

---

### Phase 12: POST & File Uploads

| Task | Owner | Details |
|------|-------|---------|
| Parse `multipart/form-data` | **Front-End** | Boundary separation |
| Parse `application/x-www-form-urlencoded` | **Front-End** | Form data |
| Save files to `upload_store` | **Front-End** | Disk write |
| **Bonus:** Update session data | **Front-End** | Upload count |
| Return `201`/`204` | **Front-End** | Success |
| Enforce `client_max_body_size` -> 413 | **Front-End** | Size limit |

**Integration Checkpoint:** HTML form uploads file to `upload_store`.

---

### Phase 13: DELETE

| Task | Owner | Details |
|------|-------|---------|
| Resolve file path | **Front-End** | `root + URI` |
| `unlink()` file | **Front-End** | Delete |
| Return `204`/`200` | **Front-End** | Success |
| Handle `404`/`403` | **Front-End** | Errors |

**Integration Checkpoint:** `curl -X DELETE` removes file.

---

### Phase 14: Redirection

| Task | Owner | Details |
|------|-------|---------|
| Check `return 301/302 /new-path` | **Front-End** | Redirect detection |
| Build `301/302` + `Location:` header | **Front-End** | Response building |
| Preserve session cookie | **Front-End** | Phase 10 handles |

**Integration Checkpoint:** `/old` -> browser redirects to `/new`.

---

### Phase 15: Connection Management

| Task | Owner | Details |
|------|-------|---------|
| Check `client->keep_alive` | **Back-End** | Reads front-end's flag |
| `keep_alive == true`: reset to `READING`, clear buffers | **Back-End** | Next request |
| `keep_alive == false`: `close(fd)`, remove from poll | **Back-End** | Teardown |
| Client timeout | **Back-End** | Close inactive |
| Set `keep_alive` based on HTTP version + `Connection` | **Front-End** | HTTP/1.1 default |

**Data Flow -- Keep-Alive:**
```cpp
// Input:  write_buffer empty, client.keep_alive = true
// Output:
client.state = 0;           // READING
client.read_buffer.clear();
client.last_activity = now();
// fd stays in poll array with POLLIN only
```

**Data Flow -- Close:**
```cpp
// Input:  write_buffer empty, client.keep_alive = false
// Output:
close(client.fd);
// Remove from poll array, delete Client object
```

**Integration Checkpoint:** Multiple requests on same connection (`curl -v`).

---

### Phase 16: Multiple Ports / Servers

| Task | Owner | Details |
|------|-------|---------|
| Parse multiple `server {}` blocks | **Back-End** | Phase 2 |
| Create listening socket per `host:port` | **Back-End** | Socket creation |
| All listeners in same poll set | **Back-End** | Single `poll()` |
| Tag `Client` with `server_idx` | **Back-End** | `client->server_idx` |
| Use `server_idx` during routing | **Front-End** | Phase 7 |

**Data Example:**
```cpp
// Output:
std::vector<int> listenFds;     // [3, 4]
std::map<int, int> fdToServer;  // {3: 0, 4: 1}

// On accept() from fd=3:
Client* c = new Client();
c->server_idx = fdToServer[3];  // 0
```

**Integration Checkpoint:** Different content on port 8080 vs 9090.

---

### Phase 17: Demo Pages (Bonus)

| Task | Owner | Details |
|------|-------|---------|
| Create `www/cookie_demo.html` | **Front-End** | HTML |
| Create `cgi-bin/counter.py` | **Front-End** | Python CGI |
| Create `cgi-bin/hello.php` | **Front-End** | PHP CGI |
| Create upload test form | **Front-End** | HTML form |
| Ensure files in correct dirs | **Back-End** | Directory setup |

**Integration Checkpoint:** All demo pages work in browser.

---

### Phase 18: Testing & Hardening

| Task | Owner | Details |
|------|-------|---------|
| Browser testing (Chrome/Firefox/Safari) | **Both** | Visual check |
| Telnet raw HTTP testing | **Both** | Protocol check |
| **Bonus:** `Set-Cookie` -> `Cookie` flow | **Front-End** | Session test |
| **Bonus:** `.php` + `.py` execution | **Both** | CGI test |
| Stress test: `siege -c 100 -t 10s` | **Back-End** | Concurrency |
| `valgrind --leak-check=full --track-fds=yes` | **Back-End** | Memory/fd leaks |
| Client disconnect mid-request | **Back-End** | Cleanup |
| CGI hang -> timeout + `kill()` | **Back-End** | Zombie prevention |
| Malformed headers, huge URI | **Front-End** | Input validation |
| Remove debug prints, C++98 compliance | **Both** | Polish |

---

## Interface Contract (The API Boundary)

### Back-End -> Front-End (function calls)

```cpp
// When complete HTTP request is in read_buffer
void FrontEnd::handleRequest(Client& client, const Config& config);

// When CGI child exits and output is ready
void FrontEnd::handleCgiComplete(Client& client, const Config& config, int exitStatus);
```

### Front-End -> Back-End (function calls)

```cpp
// Launch CGI script
void BackEnd::startCgi(Client& client,
                       const std::string& interpreter,
                       const std::string& scriptPath,
                       const std::map<std::string, std::string>& env);

// Signal response is ready for sending
void BackEnd::setClientWritable(Client& client);
```

### Shared Mutable State

| Field | Writer | Reader | Purpose |
|-------|--------|--------|---------|
| `client->read_buffer` | Back-End | Front-End | Raw HTTP request bytes |
| `client->write_buffer` | Front-End | Back-End | Complete HTTP response |
| `client->keep_alive` | Front-End | Back-End | Connection persistence flag |
| `client->state` | Front-End | Back-End | `READING`/`WRITING`/`CGI_RUNNING` |
| `client->request_obj` | Front-End | Front-End | `HttpRequest*` (opaque to back-end) |
| `client->response_obj` | Front-End | Front-End | `HttpResponse*` (opaque to back-end) |
| `client->cgi_output_buffer` | Back-End | Front-End | Raw CGI output |
| `client->server_idx` | Back-End | Front-End | Which server block |

---

## Daily Integration Checkpoints

| Day | What Must Work | Validation |
|-----|----------------|------------|
| **Day 2** | Config parses, prints to stdout | Both review output |
| **Day 3** | Server accepts telnet, echoes raw bytes | `telnet localhost 8080` |
| **Day 4** | Front-end returns hardcoded `200 OK` | Browser shows "Hello" |
| **Day 6** | Static files serve (`index.html`, CSS, images) | Browser renders page |
| **Day 8** | CGI runs `.php`, POST upload works | Upload form + script |
| **Day 10** | **Bonus:** Cookies persist, multiple CGIs work | Refresh page, session holds |
| **Day 12** | `valgrind` clean, `siege` stable | Zero leaks, no crashes |

---

## Summary Table

| Phase | Back-End Tasks | Front-End Tasks |
|-------|---------------|-----------------|
| 1 | `Makefile`, `main.cpp`, build system | Review `Shared.hpp` |
| 2 | File lexer, syntax validation, `Config` object | Semantic validation, defaults |
| 3 | -- | `SessionManager` |
| 4 | All socket creation, `bind`, `listen`, `O_NONBLOCK` | -- |
| 5 | `poll()` loop, `accept`, `recv`, `send`, handoff | -- |
| 6 | -- | `HttpRequest` parser |
| 7 | -- | Router, `RouteResult` |
| 8 | -- | Static file serving |
| 9 | -- | Error pages |
| 10 | -- | `HttpResponse`, cookies, `Set-Cookie` |
| 11 | `pipe`, `fork`, `execve`, read CGI output | Detect CGI, build env, format output |
| 12 | -- | POST, uploads, multipart |
| 13 | -- | DELETE |
| 14 | -- | Redirection |
| 15 | `close`/`keep-alive`, timeout | Set `keep_alive` flag |
| 16 | Multiple listeners, `server_idx` | Use `server_idx` in routing |
| 17 | Directory setup | Demo pages, CGI scripts |
| 18 | Stress test, `valgrind`, leaks | Browser test, input validation |

---

## One-Sentence Rule

> **If it involves `fd`, `poll()`, `socket()`, `fork()`, `pipe()`, or `recv()`/`send()` -> Back-End.**  
> **If it involves headers, status codes, routing, MIME types, cookies, or HTML -> Front-End.**
