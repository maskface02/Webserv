*This project has been created as part of the 42 curriculum by lasoubai, zatais.*

# Webserv

A lightweight HTTP/1.1 web server built from scratch in C++98. No external libraries, just sockets, poll(), and a lot of RFC reading.

## Description

Webserv is our take on building a web server like nginx or Apache from the ground up. It handles the basics you'd expect: serves static files, processes CGI scripts, manages file uploads, and deals with multiple clients at once using non-blocking I/O.

The whole thing runs on a single-threaded event loop with poll(), so it can handle thousands of concurrent connections without breaking a sweat. Configuration is done through a custom nginx-like config file that we parse ourselves.

### What it does

- Handles GET, POST, and DELETE requests
- Serves static files with proper MIME types
- Executes CGI scripts (Python, PHP) with stdin/stdout piping
- File uploads via multipart/form-data
- Directory listing (autoindex)
- URL redirections (301)
- Custom error pages (404, 403, 500,...)
- Session management with cookies
- Chunked transfer encoding for streaming large bodies
- Keep-alive connections
- Client and CGI timeouts to prevent hangs
- Colored logging so you can actually see what's happening

## Instructions

### Prerequisites

You need:
- A C++ compiler (g++ or clang++)
- Make
- Python 3 (for CGI scripts)
- PHP-CGI if you want to test PHP scripts (optional)

### Compilation

```bash
make
```

That's it. You'll get a `webserv` binary in the project root.

Other targets:
- `make clean` - Remove object files
- `make fclean` - Remove everything including the binary
- `make re` - Recompile from scratch

### Running the server

**Default config:**
```bash
./webserv
```

This uses `conf/default.conf` and starts listening on `127.0.0.1:8080`.

**Custom config:**
```bash
./webserv path/to/your/config.conf
```

### Configuration

The config format is inspired by nginx. Here's a basic example:

```nginx
server {
    listen 127.0.0.1:8080;
    
    client_max_body_size 1048576;
    error_page 404 ./errors/404.html;
    
    location / {
        root ./html;
        index index.html;
        autoindex on;
        allow_methods GET POST DELETE;
        upload_store ./upload;
        cgi .py /usr/bin/python3;
    }
    
    location /api {
        return 301 http://example.com;
    }
}
```

**Available directives:**

Server level:
- `listen <ip>:<port>` - Where to bind (can have multiple)
- `client_max_body_size <bytes>` - Max request body size
- `error_page <code> <path>` - Custom error pages

Location level:
- `root <directory>` - Document root
- `index <file1> <file2>` - Default files to serve
- `autoindex on|off` - Enable directory listing
- `allow_methods <methods>` - Which HTTP methods are allowed
- `upload_store <directory>` - Where to save uploads
- `cgi <extension> <interpreter>` - CGI handler for file types
- `return <code> <url>` - Redirect

### Testing

We've got a test suite in `test_suite/`:

```bash
cd test_suite
./run_tests.sh
```

Or test manually with curl:

```bash
# Basic GET
curl http://localhost:8080/

# File upload
curl -X POST -F "file=@test.txt" http://localhost:8080/upload

# DELETE
curl -X DELETE http://localhost:8080/file.txt
```

### Stopping

Just hit `Ctrl+C`. The server handles SIGINT and shuts down cleanly.

## Resources

### Documentation we actually read

**HTTP Protocol:**
- [RFC 2616](https://tools.ietf.org/html/rfc2616) - The original HTTP/1.1 spec. Dense but necessary.
- [RFC 7230-7235](https://httpwg.org/specs/) - Updated HTTP specs
- [MDN HTTP Docs](https://developer.mozilla.org/en-US/docs/Web/HTTP) - Way more readable than the RFCs, great for quick reference

**Socket Programming:**
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) - If you're new to sockets, start here. It's gold.
- [Linux man pages](https://man7.org/linux/man-pages/) - socket(7), poll(2), fork(2) - you'll be living in these

**CGI:**
- [RFC 3875](https://tools.ietf.org/html/rfc3875) - The CGI 
**Non-blocking I/O:**
- [poll() vs select() vs epoll()](https://eklitzke.org/blocking-io-nonblocking-io-and-epoll) - Why we chose poll()
**Config Parsing:**
- [Nginx Config Docs](https://nginx.org/en/docs/beginners_guide.html) - We borrowed their format because it works

### How we used AI

Look, we didn't pretend AI doesn't exist. We used it, but as a reference tool, not a code generator. Here's the honest breakdown:

**What AI helped with:**

1. **Understanding the HTTP spec** - RFCs are dry. We used AI to explain tricky parts like chunked encoding, multipart parsing, and proper header handling.

2. **Architecture brainstorming** - We bounced ideas off AI about how to structure the server. The separation between Request parsing, Response generation, and CGI handling came from those discussions.

3. **Debugging edge cases** - When we hit weird bugs (race conditions, memory leaks, clients that don't close connections), AI helped us think through what might be going wrong.

4. **Code review** - We asked AI to check our code for C++98 compliance and potential issues. It caught some stuff we missed.

5. **Config parser design** - We talked through different approaches to parsing the config file. AI suggested the tokenization strategy we ended up using.
```

## Technical choices we made

**Why C++98?**
Because 42 said so. Honestly, it forced us to understand memory management properly. No smart pointers, no auto, no foreach loops - you learn what's actually happening under the hood.

**Why poll() instead of select() or epoll()?**
- select() has a hard limit on file descriptors (FD_SETSIZE, usually 1024)
- epoll() is Linux-specific and we wanted something more portable
- poll() sits in the sweet spot: handles lots of connections, works everywhere, not too complex

**Why nginx-like config?**
Because it's familiar to anyone who's worked with web servers, and the hierarchical structure (server → location) maps perfectly to what we needed to configure.

