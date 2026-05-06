# Webserv Team Split: Back-End vs Front-End

> **One owner per task. No overlap.**  
> Config parser → Back-End | Sessions/Cookies → Front-End

---

## Ownership Rule

| If it involves... | Owner |
|-------------------|-------|
| **File descriptors, processes, network layer** | **Back-End** |
| **Headers, status codes, content logic** | **Front-End** |

---

## Peer Profiles

### Back-End Peer: "The Engine"
- Sockets, `poll()` loop, connection lifecycle
- Non-blocking I/O, read/write buffers
- Config file parser (infrastructure)
- CGI execution (`fork`, pipes, `execve`, `waitpid`)
- Client state machine (`READING` → `WRITING`)
- Connection keep-alive / teardown

### Front-End Peer: "The Protocol"
- HTTP request/response parsing
- Routing & location matching
- Methods: GET, POST, DELETE
- Static file serving logic
- File uploads (`multipart/form-data`)
- Error page generation
- Cookies & session management (Bonus)
- CGI output formatting into HTTP

---

## Phase 1: Foundation

| Back-End | Front-End |
|----------|-----------|
| Create `Makefile`, `main.cpp`, folder structure | Review `Shared.hpp` contract |
| Handle `./webserv [config_file]` args | — |
| Compile with `-Wall -Wextra -Werror -std=c++98` | — |

**Deliverable:** Empty binary compiles and runs.

---

## Phase 2: Configuration File → **BACK-END**

| Back-End | Front-End |
|----------|-----------|
| **Implement config parser**: read file, handle `{}`, `;`, `#` | **Define** `Config`, `ServerConfig`, `Location` structs in `Shared.hpp` |
| Validate: numeric ports, valid methods, existing paths | Review parser output / test with sample configs |
| Store parsed data in `Config` object | — |

**Why Back-End:** Needs `listen` ports at boot time to create sockets. Front-end cannot route if sockets don't exist.

**Deliverable:** `./webserv conf/default.conf` parses and prints config.

---

## Phase 3: Session Store Infrastructure → **FRONT-END**

| Back-End | Front-End |
|----------|-----------|
| — | **Implement `SessionManager`** class |
| — | `createSession()`, `getSession()`, `destroySession()` |
| — | Generate random session IDs (hex string) |

**Why Front-End:** Sessions are pure HTTP header logic. Back-end just sends bytes.

**Deliverable:** `SessionManager::getInstance().createSession()` returns valid ID.

---

## Phase 4: Socket Layer → **BACK-END**

| Back-End | Front-End |
|----------|-----------|
| Loop through `Config.servers`, create listening socket per unique `host:port` | — |
| `socket()`, `setsockopt(SO_REUSEADDR)`, `bind()`, `listen()` | — |
| `fcntl(fd, F_SETFL, O_NONBLOCK)` on every fd | — |

**Deliverable:** `netstat -tlnp` shows server listening on configured ports.

---

## Phase 5: The Single Poll Loop → **BACK-END**

| Back-End | Front-End |
|----------|-----------|
| Build `poll()` array with all listening fds | — |
| Main loop: `poll()` → dispatch events | — |
| `accept_new_client()`: create `Client`, add to poll set with `POLLIN` | — |
| `read_from_client()`: `recv()` into `client->read_buffer` | — |
| Detect full request (`\r\n\r\n` + full body), then call `FrontEnd::handleRequest()` | — |
| `write_to_client()`: `send()` from `client->write_buffer`, handle partial writes | — |
| `close_client()`: remove from poll set, cleanup | — |

**Deliverable:** Telnet connects, server echoes raw bytes back.

---

## Phase 6: HTTP Request Parsing → **FRONT-END**

| Back-End | Front-End |
|----------|-----------|
| — | **Implement `HttpRequest` class** |
| — | Parse request line: `METHOD URI HTTP/VERSION` |
| — | Parse URI (path + query string) |
| — | Parse headers into `std::map<std::string, std::string>` |
| — | **Bonus**: Parse `Cookie` header into map |
| — | Extract `Host`, `Content-Length`, `Transfer-Encoding`, `Connection` |
| — | Handle chunked encoding if needed |
| — | Validate: supported method? HTTP version? URI ok? |

**Deliverable:** Raw bytes from back-end become structured `HttpRequest` object.

---

## Phase 7: Routing → **FRONT-END**

| Back-End | Front-End |
|----------|-----------|
| — | **Implement Router** |
| — | Match `Host:port` to `ServerConfig` (using `client->server_idx`) |
| — | Match URI to longest `location` prefix |
| — | Check `allow_methods` → `405` if not allowed |
| — | Extract `root`, `index`, `autoindex`, `redirect`, `upload_store`, `cgi_extension`, `cgi_path` |

**Deliverable:** `GET /index.html` resolves to correct file path.

---

## Phase 8: Static File Serving → **FRONT-END**

| Back-End | Front-End |
|----------|-----------|
| — | Build file path: `root + URI` |
| — | `open()`/`read()` file into string (regular disk files don't need poll) |
| — | Determine MIME type from extension |
| — | If directory: `index` file → serve; `autoindex on` → generate HTML listing; else `403` |
| — | If not found → `404` |

**Deliverable:** Browser loads `index.html` with images and CSS.

---

## Phase 9: Error Pages → **FRONT-END**

| Back-End | Front-End |
|----------|-----------|
| — | Map status codes to custom error pages from config |
| — | Generate default HTML if custom page missing |
| — | Implement all status codes: `200`, `201`, `204`, `301`, `302`, `400`, `403`, `404`, `405`, `413`, `500`, `502`, `504` |

**Deliverable:** `curl http://localhost:8080/nonexistent` returns `404` with HTML body.

---

## Phase 10: Response Builder with Cookies → **FRONT-END**

| Back-End | Front-End |
|----------|-----------|
| — | **Implement `HttpResponse` class** |
| — | Build status line + headers + body string |
| — | **Bonus**: Check `Cookie` header for `session_id` |
| — | **Bonus**: If no session → `createSession()` → add `Set-Cookie` header |
| — | **Bonus**: If valid session → attach session data to response |
| — | Fill `client->write_buffer` with final response string |
| — | Set `client->keep_alive` and `client->state = WRITING` |
| — | Call `BackEnd::setClientWritable(client)` |

**Deliverable:** Every response is a valid HTTP/1.1 message.

---

## Phase 11: CGI with Multiple Types → **SPLIT**

### Part A: Front-End Detects & Prepares

| Back-End | Front-End |
|----------|-----------|
| — | **Detect CGI**: URI matches location with `cgi_extension` |
| — | **Resolve interpreter**: use `cgi_path` from matched location (`.php` → `php-cgi`, `.py` → `python3`) |
| — | Prepare env vars map: `REQUEST_METHOD`, `QUERY_STRING`, `CONTENT_LENGTH`, `CONTENT_TYPE`, `SCRIPT_NAME`, `PATH_INFO`, `PATH_TRANSLATED`, `HTTP_COOKIE` |
| — | Call `BackEnd::startCgi(client, interpreter, script_path, env)` |

### Part B: Back-End Executes

| Back-End | Front-End |
|----------|-----------|
| `startCgi()`: `pipe()` stdin + stdout, set `O_NONBLOCK`, add to poll set | — |
| `fork()`: child `dup2()`, `chdir()`, `execve(interpreter, ...)` | — |
| Feed `client->cgi_input_buffer` to CGI stdin pipe (wait for `POLLOUT`) | — |
| Read CGI stdout into `client->cgi_output_buffer` (wait for `POLLIN`) | — |
| When CGI exits (`waitpid` + `WNOHANG`), call `FrontEnd::handleCgiComplete()` | — |

### Part C: Front-End Formats CGI Output

| Back-End | Front-End |
|----------|-----------|
| — | **Implement `handleCgiComplete()`**: parse CGI stdout (split headers/body) |
| — | If no `Content-Length` from CGI, EOF marks end (subject rule) |
| — | Wrap CGI output into proper HTTP response (run through Phase 10 for cookies/headers) |

### Part D: Back-End Cleanup

| Back-End | Front-End |
|----------|-----------|
| Reap zombie processes, remove pipes from poll set | — |

**Deliverable:** `.php` and `.py` scripts both execute and return valid HTTP.

---

## Phase 12: POST & File Uploads → **FRONT-END**

| Back-End | Front-End |
|----------|-----------|
| — | Parse `multipart/form-data` (boundary separation) |
| — | Parse `application/x-www-form-urlencoded` |
| — | Save uploaded files to `upload_store` |
| — | **Bonus**: Update session data (e.g., upload count) |
| — | Return `201`/`204` |
| — | Enforce `client_max_body_size` → `413` |

**Deliverable:** HTML form uploads file, file appears on disk.

---

## Phase 13: DELETE → **FRONT-END**

| Back-End | Front-End |
|----------|-----------|
| — | Resolve file path from URI + root |
| — | `unlink()` file |
| — | Return `204`/`200`, handle `404`/`403` |

**Deliverable:** `curl -X DELETE http://localhost:8080/uploads/file.txt` removes file.

---

## Phase 14: Redirection → **FRONT-END**

| Back-End | Front-End |
|----------|-----------|
| — | If location has `return 301 /new-path`: build `301/302` + `Location:` header |
| — | Run through Phase 10 so session cookie is preserved |

**Deliverable:** `curl -v http://localhost:8080/old` shows `301` with `Location:` header.

---

## Phase 15: Connection Management → **BACK-END**

| Back-End | Front-End |
|----------|-----------|
| After `write_buffer` empty: check `client->keep_alive` | Set `client->keep_alive` based on HTTP version and `Connection` header |
| If `keep_alive == true`: reset state to `READING`, clear `read_buffer` | — |
| If `keep_alive == false`: close fd, remove from poll set | — |
| Implement client timeout: close inactive connections | — |

**Deliverable:** Browser keeps connection open for multiple requests.

---

## Phase 16: Multiple Ports / Servers → **BACK-END**

| Back-End | Front-End |
|----------|-----------|
| Parse multiple `server {}` blocks from config | — |
| Create one listening socket per unique `host:port` | — |
| All listeners in same poll set | — |
| Tag each accepted `Client` with `server_idx` pointing to correct `ServerConfig` | Use `client->server_idx` during routing (Phase 7) |

**Deliverable:** Port 8080 and 9090 serve different content simultaneously.

---

## Phase 17: Demo Pages for Bonus → **FRONT-END**

| Back-End | Front-End |
|----------|-----------|
| Place files in `www/` and `cgi-bin/` directories | Create `cookie_demo.html` |
| Ensure CGI scripts are executable | Create `cgi-bin/counter.py` and `cgi-bin/hello.php` |
| — | Create upload test HTML form |

**Deliverable:** Browser demonstrates cookies and multiple CGI types.

---

## Phase 18: Testing & Hardening

| Back-End | Front-End |
|----------|-----------|
| Stress test: `siege -c 100` | Browser test: Chrome/Firefox |
| `valgrind --leak-check=full --track-fds=yes` | Telnet raw HTTP test |
| Test: client disconnects mid-request, empty request, slow client | Test: malformed headers, huge URI, bad multipart |
| Test: CGI hangs → implement timeout + `kill()` | **Bonus test**: Verify `Set-Cookie` on first request, `Cookie` on second |
| Ensure zero fd leaks | **Bonus test**: `.php` and `.py` both execute correctly |
| Remove debug prints, ensure C++98 | Remove debug prints, ensure C++98 |

**Deliverable:** `valgrind` clean, `siege` stable, no crashes.

---

## Integration Checkpoints (Merge Here)

| Day | What Must Work | Validation |
|-----|---------------|------------|
| **Day 2** | Config parses, prints to stdout | Both peers review output |
| **Day 3** | Back-end accepts telnet, echoes raw bytes | `telnet localhost 8080` |
| **Day 4** | Back-end hands request to Front-end; Front-end returns hardcoded `200 OK` | Browser shows "Hello" |
| **Day 6** | `./webserv default.conf` serves `index.html` with images/CSS | Browser dev tools |
| **Day 8** | CGI runs `.php`; POST upload works | Browser form + `ls uploads/` |
| **Day 10** | Bonus: cookies persist; multiple CGI types work | Browser cookies tab |
| **Day 12** | `valgrind` clean, `siege` stable, no crashes | Both peers run tests |

---

## The Golden Rule

> **Back-end never parses HTTP. Front-end never touches `poll()`, `socket()`, or `fork()`.**  
> The `Shared.hpp` contract is your API boundary — never cross it without talking to your peer first.
