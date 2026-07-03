#!/bin/bash

# Test suite for webserv
# Tests static files, CGI, uploads, and error handling

SERVER_HOST="127.0.0.1"
SERVER_PORT="8080"
BASE_URL="http://${SERVER_HOST}:${SERVER_PORT}"

echo "=========================================="
echo "Starting Webserv Test Suite"
echo "=========================================="
echo ""

# Test 1: GET static HTML file
echo "[Test 1] GET static HTML file"
curl -s -o /dev/null -w "Status: %{http_code}\n" ${BASE_URL}/test_suite/test.html
echo ""

# Test 2: GET static text file
echo "[Test 2] GET static text file"
curl -s -o /dev/null -w "Status: %{http_code}\n" ${BASE_URL}/test_suite/test.txt
echo ""

# Test 3: GET non-existent file (should return 404)
echo "[Test 3] GET non-existent file (expect 404)"
curl -s -o /dev/null -w "Status: %{http_code}\n" ${BASE_URL}/nonexistent.html
echo ""

# Test 4: GET directory without index (should return 403 or autoindex)
echo "[Test 4] GET directory"
curl -s -o /dev/null -w "Status: %{http_code}\n" ${BASE_URL}/test_suite/
echo ""

# Test 5: POST file upload
echo "[Test 5] POST file upload"
curl -s -o /dev/null -w "Status: %{http_code}\n" -X POST -F "file=@/home/m45kf4c3/Desktop/Projects/Webserv/test_suite/test.txt" ${BASE_URL}/upload/
echo ""

# Test 6: CGI GET request
echo "[Test 6] CGI GET request"
curl -s -o /dev/null -w "Status: %{http_code}\n" ${BASE_URL}/test_suite/test_cgi.py
echo ""

# Test 7: CGI POST request
echo "[Test 7] CGI POST request"
curl -s -o /dev/null -w "Status: %{http_code}\n" -X POST -d "name=test&value=123" ${BASE_URL}/test_suite/test_cgi.py
echo ""

# Test 8: DELETE file
echo "[Test 8] DELETE file"
curl -s -o /dev/null -w "Status: %{http_code}\n" -X DELETE ${BASE_URL}/test_suite/test.txt
echo ""

# Test 9: Invalid method
echo "[Test 9] Invalid HTTP method"
curl -s -o /dev/null -w "Status: %{http_code}\n" -X INVALID ${BASE_URL}/
echo ""

# Test 10: Large file upload (stress test)
echo "[Test 10] Large file upload (10MB)"
curl -s -o /dev/null -w "Status: %{http_code}\n" -X POST -F "file=@/home/m45kf4c3/Desktop/Projects/Webserv/test_suite/large_file.bin" ${BASE_URL}/upload/
echo ""

# Test 11: Request with query string
echo "[Test 11] GET with query string"
curl -s -o /dev/null -w "Status: %{http_code}\n" "${BASE_URL}/test_suite/test_cgi.py?foo=bar&baz=qux"
echo ""

# Test 12: Keep-alive test (multiple requests on same connection)
echo "[Test 12] Keep-alive test"
curl -s -o /dev/null -w "Status: %{http_code}\n" --keepalive ${BASE_URL}/test_suite/test.html ${BASE_URL}/test_suite/test.txt
echo ""

echo "=========================================="
echo "Test Suite Complete"
echo "=========================================="
