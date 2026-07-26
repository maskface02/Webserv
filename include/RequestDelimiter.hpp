#ifndef REQUESTDELIMITER_HPP
#define REQUESTDELIMITER_HPP

#include <string>

struct ParsedRequestLine {
  std::string method;
  std::string uri;
  std::string httpVers;
};

class RequestDelimiter {
private:
  RequestDelimiter(const RequestDelimiter &);
  RequestDelimiter &operator=(const RequestDelimiter &);

public:
  RequestDelimiter();
  ~RequestDelimiter();

  ParsedRequestLine parseRequestLine(const std::string &buffer);
  bool iequal(std::string &a, const std::string &b);
  size_t findHeaderValue(std::string &buffer, const std::string &name,
                         size_t headerEnd);
  size_t parseChunkedBody(std::string &buffer, size_t bodyStart);
  bool isChunked(std::string &buffer, size_t headerEnd);
  size_t getContentLength(std::string &buffer, size_t headerEnd);
  size_t getRequestSize(std::string &buffer);
};

#endif
