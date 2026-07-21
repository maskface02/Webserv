#include "../include/WebServ.hpp"

RequestDelimiter::RequestDelimiter() {}

RequestDelimiter::~RequestDelimiter() {}

ParsedRequestLine RequestDelimiter::parseRequestLine(const std::string &buffer) {
  ParsedRequestLine line;
  line.method = "-";
  line.uri = "-";
  line.httpVers = "-";

  size_t pos = buffer.find("\r\n");
  if (pos == std::string::npos)
    return line;

  std::string reqLine = buffer.substr(0, pos);

  size_t sp1 = reqLine.find(' ');
  if (sp1 == std::string::npos)
    return line;
  line.method = reqLine.substr(0, sp1);

  size_t sp2 = reqLine.find(' ', sp1 + 1);
  if (sp2 == std::string::npos)
    return line;
  line.uri = reqLine.substr(sp1 + 1, sp2 - sp1 - 1);
  line.httpVers = reqLine.substr(sp2 + 1);

  return line;
}

bool RequestDelimiter::iequal(std::string &a, const std::string &b) {
  if (a.size() != b.size())
    return false;
  for (size_t i = 0; i < a.size(); ++i)
    if (tolower(a[i]) != tolower(b[i]))
      return false;
  return true;
}

size_t RequestDelimiter::findHeaderValue(std::string &buffer, const std::string &name,
                                      size_t headerEnd) {
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
        while (val < lineEnd && isspace(buffer[val]))
          ++val;
        return val;
      }
    }
    pos = lineEnd + 2;
  }
  return std::string::npos;
}

size_t RequestDelimiter::parseChunkedBody(std::string &buffer, size_t bodyStart) {
  size_t bodyEnd = buffer.find("0\r\n\r\n", bodyStart);
  if (bodyEnd == std::string::npos)
    return std::string::npos;
  return bodyEnd - bodyStart + 5;
}

bool RequestDelimiter::isChunked(std::string &buffer, size_t headerEnd) {
  size_t te_pos = findHeaderValue(buffer, "Transfer-Encoding", headerEnd);
  if (te_pos == std::string::npos)
    return false;
  size_t lineEnd = buffer.find("\r\n", te_pos);
  if (lineEnd == std::string::npos)
    return false;
  std::string value(buffer, te_pos, lineEnd - te_pos);
  size_t last = value.find_last_not_of(" \t");
  if (last != std::string::npos)
    value = value.substr(0, last + 1);
  return iequal(value, "chunked");
}

size_t RequestDelimiter::getContentLength(std::string &buffer, size_t headerEnd) {
  size_t cl_pos = findHeaderValue(buffer, "Content-Length", headerEnd);
  if (cl_pos == std::string::npos)
    return 0;
  size_t lineEnd = buffer.find("\r\n", cl_pos);
  if (lineEnd == std::string::npos)
    return 0;
  std::string value(buffer, cl_pos, lineEnd - cl_pos);
  size_t last = value.find_last_not_of(" \t");
  if (last != std::string::npos)
    value = value.substr(0, last + 1);
  else
    value.clear();
  char *endptr;
  long num = std::strtol(value.c_str(), &endptr, 10);
  if (endptr == value.c_str() || *endptr != '\0' || num < 0)
    return 0;
  return static_cast<size_t>(num);
}

size_t RequestDelimiter::getRequestSize(std::string &buffer) {
  size_t header_end = buffer.find("\r\n\r\n");
  if (header_end == std::string::npos)
    return std::string::npos;
  size_t body_start = header_end + 4;

  if (isChunked(buffer, header_end)) {
    size_t bodySize = parseChunkedBody(buffer, body_start);
    if (bodySize == std::string::npos)
      return std::string::npos;
    size_t total = body_start + bodySize;
    return (buffer.size() >= total) ? total : std::string::npos;
  }

  size_t total = body_start + getContentLength(buffer, header_end);
  return (buffer.size() >= total) ? total : std::string::npos;
}
