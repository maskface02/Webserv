#include <string>
#include <iostream>
#include <cstdlib>
#include <cctype>

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"

static bool iequal(const std::string& a, const std::string& b) {
  if (a.size() != b.size())
    return false;
  for (size_t i = 0; i < a.size(); ++i)
    if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
      return false;
  return true;
}

static size_t findHeaderValue(const std::string& buffer, const std::string& name, size_t headerEnd) {
  size_t pos = 0;
  while (pos < headerEnd) {
    size_t lineEnd = buffer.find("\r\n", pos);
    if (lineEnd == std::string::npos)
      break;
    size_t colon = buffer.find(':', pos);
    if (colon != std::string::npos && colon < lineEnd) {
      std::string hdrName(buffer, pos, colon - pos);
      if (iequal(hdrName, name)) {
        size_t val = colon + 1;
        while (val < lineEnd && (buffer[val] == ' ' || buffer[val] == '\t'))
          ++val;
        return val;
      }
    }
    pos = lineEnd + 2;
  }
  return std::string::npos;
}

static size_t parseChunkedBody(const std::string& buffer, size_t bodyStart) {
  size_t bodyEnd = buffer.find("0\r\n\r\n", bodyStart);
  if (bodyEnd == std::string::npos)
    return std::string::npos;
  return bodyEnd - bodyStart + 5;
}

static size_t getRequestSize(const std::string& buffer) {
  size_t header_end = buffer.find("\r\n\r\n");
  if (header_end == std::string::npos)
    return std::string::npos;

  size_t body_start = header_end + 4;

  size_t te_pos = findHeaderValue(buffer, "Transfer-Encoding", header_end);
  if (te_pos != std::string::npos) {
    size_t lineEnd = buffer.find("\r\n", te_pos);
    if (lineEnd != std::string::npos) {
      std::string value(buffer, te_pos, lineEnd - te_pos);
      size_t last = value.find_last_not_of(" \t");
      if (last != std::string::npos)
        value = value.substr(0, last + 1);
      if (iequal(value, "chunked")) {
          size_t bodySize = parseChunkedBody(buffer, body_start);
          if (bodySize == std::string::npos)
            return std::string::npos;
          size_t total = body_start + bodySize;
          return (buffer.size() >= total) ? total : std::string::npos;
        }
    }
  }

  size_t content_length = 0;
  size_t cl_pos = findHeaderValue(buffer, "Content-Length", header_end);
  if (cl_pos != std::string::npos) {
    size_t lineEnd = buffer.find("\r\n", cl_pos);
    if (lineEnd == std::string::npos)
      return std::string::npos;
    std::string value(buffer, cl_pos, lineEnd - cl_pos);
    size_t lastNonSpace = value.find_last_not_of(" \t");
    if (lastNonSpace != std::string::npos)
      value = value.substr(0, lastNonSpace + 1);
    else
      value.clear();

    char* endptr;
    long num = std::strtol(value.c_str(), &endptr, 10);
    if (endptr == value.c_str() || *endptr != '\0' || num < 0)
      num = 0;
    content_length = static_cast<size_t>(num);
  }

  size_t total = body_start + content_length;
  return (buffer.size() >= total) ? total : std::string::npos;
}

// Build a string from pieces to avoid hardcoding sizes
struct Req {
  std::string raw;
  std::string label;
};

#define REQ(...) Req{__VA_ARGS__}

static std::string rl(const std::string& method, const std::string& path) {
  return method + " " + path + " HTTP/1.1\r\n";
}

static std::string hdr(const std::string& name, const std::string& value) {
  return name + ": " + value + "\r\n";
}

int main() {
  int failed = 0;
  int total = 0;

  // Helper: run a single test case
  struct { std::string label; std::string data; size_t want; } tests[] = {
    // 1-3: No body (GET, HEAD, DELETE)
    { "GET no body",
      rl("GET","/") + hdr("Host","a") + "\r\n",
      0 },
    { "HEAD no body",
      rl("HEAD","/") + hdr("Host","a") + "\r\n",
      0 },
    { "DELETE no body",
      rl("DELETE","/file") + hdr("Host","a") + "\r\n",
      0 },

    // 4-6: Content-Length
    { "POST with CL 5",
      rl("POST","/") + hdr("Host","a") + hdr("Content-Length","5") + "\r\nhello",
      5 },
    { "POST with CL 0",
      rl("POST","/") + hdr("Host","a") + hdr("Content-Length","0") + "\r\n",
      0 },
    { "Large CL 100",
      rl("PUT","/") + hdr("Host","a") + hdr("Content-Length","100") + "\r\n" + std::string(100, 'a'),
      100 },

    // 7-9: Case-insensitive headers
    { "lowercase content-length",
      rl("POST","/") + hdr("Host","a") + hdr("content-length","3") + "\r\nabc",
      3 },
    { "lowercase transfer-encoding",
      rl("POST","/") + hdr("Host","a") + hdr("transfer-encoding","chunked") + "\r\n5\r\nhello\r\n0\r\n\r\n",
      0 },
    { "mixed case Content-Length",
      rl("POST","/") + hdr("Host","a") + hdr("CONTENT-LENGTH","4") + "\r\ndata",
      4 },

    // 10-12: Chunked encoding
    { "simple chunked",
      rl("POST","/") + hdr("Host","a") + hdr("Transfer-Encoding","chunked") + "\r\n5\r\nhello\r\n0\r\n\r\n",
      0 },
    { "multiple chunks",
      rl("POST","/") + hdr("Host","a") + hdr("Transfer-Encoding","chunked") + "\r\n5\r\nhello\r\n3\r\nabc\r\n0\r\n\r\n",
      0 },
    { "chunked with trailing space in header",
      rl("POST","/") + hdr("Host","a") + hdr("Transfer-Encoding","chunked ") + "\r\n5\r\nhello\r\n0\r\n\r\n",
      0 },

    // 13-14: Trailing whitespace in CL value
    { "CL with trailing space",
      rl("POST","/") + hdr("Host","a") + "Content-Length: 5 \r\n\r\nhello",
      5 },
    { "CL with trailing tab",
      rl("POST","/") + hdr("Host","a") + "Content-Length: 5\t\r\n\r\nhello",
      5 },

    // 15-16: Incomplete / npos cases
    { "incomplete headers (no \\r\\n\\r\\n)",
      rl("GET","/") + hdr("Host","a"),
      std::string::npos },
    { "incomplete chunked body (no terminator)",
      rl("POST","/") + hdr("Host","a") + hdr("Transfer-Encoding","chunked") + "\r\n5\r\nhel",
      std::string::npos },

    // 17-18: Pipelining (first request's size should be < full buffer size)
    { "pipelined GETs",
      rl("GET","/a") + hdr("Host","a") + "\r\n" + rl("GET","/b") + hdr("Host","a") + "\r\n",
      (rl("GET","/a") + hdr("Host","a") + "\r\n").size() },
    { "pipelined POST then GET",
      rl("POST","/") + hdr("Host","a") + hdr("Content-Length","2") + "\r\nab" +
      rl("GET","/") + hdr("Host","a") + "\r\n",
      (rl("POST","/") + hdr("Host","a") + hdr("Content-Length","2") + "\r\nab").size() },

    // 19-21: Edge cases
    { "CL whitespace only -> treated as 0",
      rl("POST","/") + hdr("Host","a") + "Content-Length:   \r\n\r\n",
      0 },
    { "CL negative -> treated as 0",
      rl("POST","/") + hdr("Host","a") + hdr("Content-Length","-1") + "\r\n",
      0 },
    { "CL non-numeric -> treated as 0",
      rl("POST","/") + hdr("Host","a") + hdr("Content-Length","abc") + "\r\n",
      0 },
  };

  total = sizeof(tests) / sizeof(tests[0]);

  std::cout << "Testing getRequestSize (" << total << " cases)\n";
  std::cout << "----------------------------------------\n";

  for (int i = 0; i < total; ++i) {
    const std::string& data = tests[i].data;
    size_t want = tests[i].want;

    // For complete-request tests, expected = full string size
    // For npos tests, want stays npos
    // For pipelining, want is already the first-request size
    if (want != std::string::npos && tests[i].label.find("pipelined") == std::string::npos)
      want = data.size();

    size_t got = getRequestSize(data);

    std::cout << "[" << (i + 1) << "] " << tests[i].label << "\n";

    // Display raw request (escape \r\n for readability)
    std::cout << "    raw: \"";
    for (size_t j = 0; j < data.size(); ++j) {
      if (data[j] == '\r')
        std::cout << "\\r";
      else if (data[j] == '\n')
        std::cout << "\\n";
      else
        std::cout << data[j];
    }
    std::cout << "\"\n";

    if (got != std::string::npos) {
      std::cout << "    req: \"";
      for (size_t j = 0; j < got && j < data.size(); ++j) {
        if (data[j] == '\r')
          std::cout << "\\r";
        else if (data[j] == '\n')
          std::cout << "\\n";
        else
          std::cout << data[j];
      }
      if (got > data.size())
        std::cout << "... (truncated)";
      std::cout << "\"\n";
    }

    std::cout << "    buf_size: " << data.size()
              << "  result: ";
    if (got == std::string::npos)
      std::cout << "npos";
    else
      std::cout << got;

    if (got == want) {
      std::cout << GREEN "  PASS" RESET "\n";
    } else {
      std::cout << RED "  FAIL" RESET "  (expected: ";
      if (want == std::string::npos)
        std::cout << "npos";
      else
        std::cout << want;
      std::cout << ")\n";
      ++failed;
    }
    std::cout << "\n";
  }

  std::cout << "----------------------------------------\n";
  std::cout << (failed ? RED "FAILED" RESET : GREEN "PASSED" RESET)
            << "  " << (total - failed) << "/" << total << "\n";

  return failed;
}
