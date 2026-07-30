#!/bin/bash

BASE_URL="http://localhost:8080"
PASS=0
FAIL=0
TOTAL=0

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

check_server() {
    if ! curl -s -o /dev/null --connect-timeout 2 "$BASE_URL/" 2>/dev/null; then
        echo -e "${RED}[ERROR] Server is not running at $BASE_URL${NC}"
        echo "Start it with: ./webserv test_custom/conf/test.conf"
        exit 1
    fi
}

run_test() {
    local num="$1"
    local name="$2"
    local expected="$3"
    shift 3
    local cmd="$@"

    TOTAL=$((TOTAL + 1))
    local status
    status=$(eval "$cmd" 2>/dev/null | head -1 | grep -oP '^\d{3}' | head -1)

    if [ "$status" = "$expected" ]; then
        echo -e "  ${GREEN}[PASS]${NC} #$num $name (HTTP $status)"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}[FAIL]${NC} #$num $name (expected $expected, got $status)"
        FAIL=$((FAIL + 1))
    fi
}

run_test_raw() {
    local num="$1"
    local name="$2"
    local expected="$3"
    shift 3
    local cmd="$@"

    TOTAL=$((TOTAL + 1))
    local output
    output=$(eval "$cmd" 2>/dev/null)
    local status
    status=$(echo "$output" | head -1 | grep -oP '\d{3}')

    if [ "$status" = "$expected" ]; then
        echo -e "  ${GREEN}[PASS]${NC} #$num $name (HTTP $status)"
        PASS=$((PASS + 1))
    else
        echo -e "  ${RED}[FAIL]${NC} #$num $name (expected $expected, got $status)"
        FAIL=$((FAIL + 1))
    fi
}

echo ""
echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}   Webserv Test Suite - Automated       ${NC}"
echo -e "${CYAN}========================================${NC}"
echo ""

check_server
echo -e "${GREEN}Server is running at $BASE_URL${NC}"
mkdir -p test_custom/upload 2>/dev/null
echo ""

echo -e "${YELLOW}--- Static Files ---${NC}"
run_test 1 "Static Text File"       "200" curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/static/test.txt"
run_test 2 "Static JSON File"       "200" curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/static/data.json"
run_test 3 "Static Markdown"        "200" curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/static/readme.md"
run_test 4 "Binary File Download"   "200" curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/static/binary.dat"
run_test 5 "Directory Listing"      "200" curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/files/"
run_test 6 "Nested Directory"       "200" curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/files/subfolder/"
echo ""

echo -e "${YELLOW}--- Python CGI ---${NC}"
run_test 7 "CGI Env Vars"           "200" curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi/echo.py"
run_test 8 "CGI Query String"       "200" curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi/query_test.py?name=webserv"
run_test 9 "CGI POST Form"          "200" curl -s -o /dev/null -w "%{http_code}" -X POST -d "name=test" "$BASE_URL/cgi/form_handler.py"
run_test 10 "CGI Error 500"         "500" curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi/error_cgi.py"
run_test 11 "CGI Not Found (404)"   "404" curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/cgi/nonexistent.py"
echo ""

echo -e "${YELLOW}--- PHP CGI ---${NC}"
if command -v php-cgi &>/dev/null; then
    run_test 12 "PHP Info"           "200" curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/php/info.php"
    run_test 13 "PHP POST Form"      "200" curl -s -o /dev/null -w "%{http_code}" -X POST -d "name=test" "$BASE_URL/php/form_handler.php"
    run_test 14 "PHP Session"        "200" curl -s -o /dev/null -w "%{http_code}" -c /tmp/cookies.txt -b /tmp/cookies.txt "$BASE_URL/php/session_test.php"
else
    echo -e "  ${YELLOW}[SKIP]${NC} PHP CGI tests (php-cgi not installed)"
fi
echo ""

echo -e "${YELLOW}--- Upload & POST ---${NC}"
rm -rf upload/* 2>/dev/null
run_test 15 "Upload Form (GET)"      "200" curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/upload"
run_test 16 "File Upload (POST)"     "201" curl -s -o /dev/null -w "%{http_code}" -X POST -F "file=@html/static/test.txt" "$BASE_URL/upload"
run_test 17 "Upload Dir Listing"     "200" curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/upload/"
run_test 18 "POST Static Page"       "200" curl -s -o /dev/null -w "%{http_code}" -X POST -d "name=test" "$BASE_URL/post"
echo ""

echo -e "${YELLOW}--- DELETE ---${NC}"
cp static_files/file2.txt /tmp/file2_backup.txt 2>/dev/null
run_test 19 "Delete File"            "200" curl -s -o /dev/null -w "%{http_code}" -X DELETE "$BASE_URL/delete/file2.txt"
run_test 20 "Delete Nonexistent"     "404" curl -s -o /dev/null -w "%{http_code}" -X DELETE "$BASE_URL/delete/nonexistent.txt"
cp /tmp/file2_backup.txt static_files/file2.txt 2>/dev/null
echo ""

echo -e "${YELLOW}--- Redirects ---${NC}"
run_test 21 "301 Redirect"           "301" curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/redirect"
echo ""

echo -e "${YELLOW}--- Error Pages ---${NC}"
run_test 22 "404 Not Found"          "404" curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/nonexistent"
run_test 23 "413 Payload Too Large"  "413" curl -s -o /dev/null -w "%{http_code}" -X POST -d "$(python3 -c 'print("A"*200)')" "$BASE_URL/small_only"
echo ""

echo -e "${YELLOW}--- HTTP Compliance & Security ---${NC}"
run_test_raw 24 "Missing Host Header"    "400" printf '"GET / HTTP/1.1\r\n\r\n"' '|' nc -w 2 localhost 8080
run_test 25 "HTTP/1.0 Request"           "200" curl -s -o /dev/null -w "%{http_code}" --http1.0 "$BASE_URL/"
run_test 26 "POST with data"             "200" curl -s -o /dev/null -w "%{http_code}" -X POST -d "test=data" "$BASE_URL/post"
run_test 27 "Keep-Alive"                 "200" curl -s -o /dev/null -w "%{http_code}" "$BASE_URL/" "$BASE_URL/static/test.txt"
run_test_raw 28 "Chunked Transfer"       "400" printf '"POST /post HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n"' '|' nc -w 2 localhost 8080
echo ""

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}   Results: ${GREEN}$PASS passed${NC} / ${RED}$FAIL failed${NC} / $TOTAL total"
echo -e "${CYAN}========================================${NC}"
echo ""

if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
else
    echo -e "${RED}$FAIL test(s) failed.${NC}"
fi

rm -f /tmp/cookies.txt /tmp/file2_backup.txt
exit $FAIL
