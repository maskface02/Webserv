#!/bin/bash

SERVER_HOST="127.0.0.1"
SERVER_PORT="8080"
BASE_URL="http://${SERVER_HOST}:${SERVER_PORT}"

TOTAL=0
PASSED=0
FAILED=0
TIMEOUT_SECS=5

GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[0;33m"
RESET="\033[0m"

run_test() {
    local test_name="$1"
    local expected_status="$2"
    local curl_args="$3"
    local timeout="${4:-$TIMEOUT_SECS}"

    TOTAL=$((TOTAL + 1))
    echo -n "[$TOTAL] $test_name... "

    actual_status=$(curl -s -o /dev/null -w "%{http_code}" --max-time "$timeout" $curl_args 2>/dev/null)

    if [ "$actual_status" = "$expected_status" ]; then
        echo -e "${GREEN}PASS${RESET} ($actual_status)"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}FAIL${RESET} (expected $expected_status, got $actual_status)"
        FAILED=$((FAILED + 1))
    fi
}

echo "=========================================="
echo "  Webserv Comprehensive Test Suite"
echo "=========================================="
echo ""

echo "=========================================="
echo "  Category 1: Static Files"
echo "=========================================="
echo ""

run_test "GET HTML file" "200" "${BASE_URL}/test_suite/test.html"
run_test "GET TXT file" "200" "${BASE_URL}/test_suite/test.txt"
run_test "GET JSON file" "200" "${BASE_URL}/test_suite/test.json"
run_test "GET CSS file" "200" "${BASE_URL}/test_suite/test.css"
run_test "GET JS file" "200" "${BASE_URL}/test_suite/test.js"
run_test "GET non-existent file (expect 404)" "404" "${BASE_URL}/nonexistent.html"
run_test "GET directory with autoindex" "200" "${BASE_URL}/test_suite/"
run_test "HEAD request rejected (501)" "501" "-I ${BASE_URL}/test_suite/test.html"

echo ""
echo "=========================================="
echo "  Category 2: CGI Scripts"
echo "=========================================="
echo ""

run_test "CGI GET basic" "200" "${BASE_URL}/test_suite/test_cgi.py" 10
run_test "CGI GET simple" "200" "${BASE_URL}/test_suite/simple_cgi.py" 10
run_test "CGI GET with query string" "200" "${BASE_URL}/test_suite/test_cgi.py?foo=bar&baz=qux" 10
run_test "CGI GET echo env" "200" "${BASE_URL}/test_suite/echo_env.py" 10
run_test "CGI GET env test" "200" "${BASE_URL}/test_suite/env_test_cgi.py" 10
run_test "CGI POST with data" "200" "-X POST -d 'name=test&value=123' ${BASE_URL}/test_suite/test_cgi.py" 10
run_test "CGI POST simple" "200" "-X POST -d 'hello=world' ${BASE_URL}/test_suite/simple_cgi.py" 10
run_test "CGI GET slow (10s timeout)" "200" "${BASE_URL}/test_suite/slow_cgi.py" 15
run_test "CGI GET large output" "200" "${BASE_URL}/test_suite/large_output_cgi.py" 10
run_test "CGI GET error exit (expect 502)" "502" "${BASE_URL}/test_suite/error_cgi.py" 10

echo ""
echo "=========================================="
echo "  Category 3: Error Handling"
echo "=========================================="
echo ""

run_test "Invalid HTTP method" "405" "-X INVALID ${BASE_URL}/"
run_test "GET on upload location" "200" "${BASE_URL}/upload/"
run_test "Large file upload (expect 413)" "413" "-X POST -H Expect: -F file=@/home/m45kf4c3/Desktop/Projects/Webserv/test_suite/large_file.bin ${BASE_URL}/upload/" 10

echo ""
echo "=========================================="
echo "  Category 4: DELETE"
echo "=========================================="
echo ""

run_test "DELETE non-existent file (expect 404)" "404" "-X DELETE ${BASE_URL}/test_suite/test_to_delete.txt"

echo ""
echo "=========================================="
echo "  Category 5: Query Strings & Parameters"
echo "=========================================="
echo ""

run_test "CGI GET with empty query" "200" "${BASE_URL}/test_suite/test_cgi.py?" 10
run_test "CGI GET with special chars in query" "200" "${BASE_URL}/test_suite/test_cgi.py?key=hello+world&other=123%20test" 10

echo ""
echo "=========================================="
echo "  Test Summary"
echo "=========================================="
echo ""
echo -e "Total:  $TOTAL"
echo -e "Passed: ${GREEN}$PASSED${RESET}"
echo -e "Failed: ${RED}$FAILED${RESET}"
echo ""

if [ "$FAILED" -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${RESET}"
else
    echo -e "${RED}$FAILED test(s) failed${RESET}"
fi

echo "=========================================="

exit "$FAILED"
