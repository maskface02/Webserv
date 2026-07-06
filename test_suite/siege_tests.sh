#!/bin/bash

# ============================================
# Siege Load Testing Suite for Webserv
# ============================================
# Tests: static files, CGI, POST, mixed load,
#        slow CGI, rapid connect/disconnect
# Pass criteria: 100% availability, 0 failed transactions

SERVER_HOST="127.0.0.1"
SERVER_PORT="8080"
BASE_URL="http://${SERVER_HOST}:${SERVER_PORT}"

GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[0;33m"
RESET="\033[0m"

TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Check if server is running
if ! pgrep webserv > /dev/null 2>&1; then
    echo -e "${RED}ERROR: webserv process not found!${RESET}"
    echo "Please start the server first"
    exit 1
fi

# Run a siege test and evaluate pass/fail based on availability
# Args: $1=test_name, $2=timeout_seconds, $3...=siege arguments
run_siege_test() {
    local test_name="$1"
    local timeout="$2"
    shift 2

    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    echo ""
    echo "--- [$TOTAL_TESTS] $test_name ---"

    local tmpfile
    tmpfile=$(mktemp)

    # Strip ANSI escape codes for clean parsing
    timeout "$timeout" siege "$@" > "$tmpfile" 2>&1
    local exit_code=$?

    # Convert to plain text
    sed -i 's/\x1b\[[0-9;]*m//g' "$tmpfile"

    if [ $exit_code -eq 124 ]; then
        echo -e "Result: ${RED}FAIL${RESET} (timed out after ${timeout}s)"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        rm -f "$tmpfile"
        return
    fi

    local total avail_pct failed trans_rate avg_resp longest
    total=$(grep "Transactions:" "$tmpfile" | awk '{print $2}')
    avail_pct=$(grep "Availability:" "$tmpfile" | awk '{print $2}')
    failed=$(grep "Failed transactions:" "$tmpfile" | awk '{print $3}')
    trans_rate=$(grep "Transaction rate:" "$tmpfile" | awk '{print $3}')
    avg_resp=$(grep "Response time:" "$tmpfile" | awk '{print $3}')
    longest=$(grep "Longest transaction:" "$tmpfile" | awk '{print $3}')

    echo "  Transactions:  $total"
    echo "  Failed:        $failed"
    echo "  Availability:  $avail_pct"
    echo "  Trans/sec:     $trans_rate"
    echo "  Avg response:  ${avg_resp}ms"
    echo "  Longest:       ${longest}ms"

    if [ "$avail_pct" = "100.00" ] && [ "$failed" = "0" ]; then
        echo -e "Result: ${GREEN}PASS${RESET} ($total requests, ${avail_pct}% availability)"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "Result: ${RED}FAIL${RESET} (failed: $failed, availability: $avail_pct)"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi

    rm -f "$tmpfile"
}

echo "=========================================="
echo "  Webserv Siege Load Test Suite"
echo "=========================================="
echo "Server: $BASE_URL"
echo ""

# Create URL files for multi-URL tests
cat > /tmp/siege_static.txt << 'EOF'
http://127.0.0.1:8080/test_suite/test.html
http://127.0.0.1:8080/test_suite/test.txt
http://127.0.0.1:8080/test_suite/test.json
http://127.0.0.1:8080/test_suite/test.css
http://127.0.0.1:8080/test_suite/test.js
EOF

cat > /tmp/siege_mixed.txt << 'EOF'
http://127.0.0.1:8080/test_suite/test.html
http://127.0.0.1:8080/test_suite/simple_cgi.py
http://127.0.0.1:8080/test_suite/test_cgi.py
http://127.0.0.1:8080/
http://127.0.0.1:8080/test_suite/test.json
EOF

# ---- TEST 1: Static file basic ----
# 10 concurrent users, 50 reps each = 500 requests to a single HTML file
# Tests basic static file serving under moderate load
run_siege_test \
    "Static file — 10 concurrent, 50 reps (500 requests)" \
    30 \
    -c10 -r50 -b http://$SERVER_HOST:$SERVER_PORT/test_suite/test.html

# ---- TEST 2: Multiple static files ----
# 20 concurrent users, 20 reps = 400 requests across 5 different static files
# Tests file serving with different MIME types and sizes
run_siege_test \
    "Multiple static files — 20 concurrent, 20 reps (400 requests)" \
    30 \
    -c20 -r20 -b -f /tmp/siege_static.txt

# ---- TEST 3: CGI GET request ----
# 10 concurrent users, 10 reps = 100 requests to CGI script
# Tests fork/exec overhead and CGI pipe handling
run_siege_test \
    "CGI GET — 10 concurrent, 10 reps (100 requests)" \
    60 \
    -c10 -r10 -b http://$SERVER_HOST:$SERVER_PORT/test_suite/test_cgi.py

# ---- TEST 4: Simple CGI (minimal output) ----
# 10 concurrent users, 20 reps = 200 requests to a minimal CGI script
# Tests quick CGI completion and pipe cleanup
run_siege_test \
    "Simple CGI — 10 concurrent, 20 reps (200 requests)" \
    60 \
    -c10 -r20 -b http://$SERVER_HOST:$SERVER_PORT/test_suite/simple_cgi.py

# ---- TEST 5: CGI POST request ----
# 5 concurrent users, 10 reps = 50 POST requests with body to CGI
# Tests CGI stdin pipe handling — body must be written and pipe closed
run_siege_test \
    "CGI POST — 5 concurrent, 10 reps (50 requests)" \
    60 \
    -c5 -r10 -b \
    "http://$SERVER_HOST:$SERVER_PORT/test_suite/test_cgi.py POST name=test&value=123"

# ---- TEST 6: Slow CGI under load ----
# 10 concurrent users, 5 reps = 50 requests to a 3-second CGI script
# Tests that slow CGIs don't block the event loop (non-blocking waitpid)
# If blocking: takes ~150s. If non-blocking: takes ~15s.
run_siege_test \
    "Slow CGI (3s each) — 10 concurrent, 5 reps (50 requests)" \
    120 \
    -c10 -r5 -b http://$SERVER_HOST:$SERVER_PORT/test_suite/slow_cgi.py

# ---- TEST 7: Rapid connect/disconnect ----
# 50 concurrent users, 50 reps = 2500 requests to a single file
# Tests FD leak — server should not accumulate open file descriptors
run_siege_test \
    "Rapid connect/disconnect — 50 concurrent, 50 reps (2500 requests)" \
    30 \
    -c50 -r50 -b http://$SERVER_HOST:$SERVER_PORT/test_suite/test.html

# ---- TEST 8: Mixed static + CGI ----
# 50 concurrent users, 5 reps = 250 requests across static and CGI URLs
# Tests mixed workload — server handles both static and CGI simultaneously
run_siege_test \
    "Mixed static + CGI — 50 concurrent, 5 reps (250 requests)" \
    60 \
    -c50 -r5 -b -f /tmp/siege_mixed.txt

# ---- TEST 9: Stress test — high concurrency ----
# 100 concurrent users, 10 reps = 1000 requests across multiple static files
# Tests server behavior under high connection count (near MAX_CLIENTS limit)
run_siege_test \
    "Stress — 100 concurrent, 10 reps (1000 requests)" \
    60 \
    -c100 -r10 -b -f /tmp/siege_static.txt

# ---- TEST 10: Directory listing under load ----
# 20 concurrent users, 20 reps = 400 requests to autoindex directory
# Tests directory listing generation under concurrent load
run_siege_test \
    "Directory listing — 20 concurrent, 20 reps (400 requests)" \
    30 \
    -c20 -r20 -b http://$SERVER_HOST:$SERVER_PORT/test_suite/

# ---- FD leak check ----
FD_COUNT=$(ls /proc/$(pgrep webserv)/fd 2>/dev/null | wc -l)

# ---- Pipe leak check ----
# Count pipe-type FDs (CGI stdin/stdout pipes show as pipe:[inode] in /proc)
PIPE_COUNT=$(ls -la /proc/$(pgrep webserv)/fd/ 2>/dev/null | grep -c "pipe:")

# ---- Summary ----
echo ""
echo "=========================================="
echo "  Siege Test Results Summary"
echo "=========================================="
echo ""
echo -e "  Total tests:  $TOTAL_TESTS"
echo -e "  Passed:       ${GREEN}$PASSED_TESTS${RESET}"
echo -e "  Failed:       ${RED}$FAILED_TESTS${RESET}"
echo ""
echo "  Server FDs:   $FD_COUNT (expected ~5)"
echo "  Pipe FDs:     $PIPE_COUNT (expected 0, normal under CGI load)"
echo ""

if [ "$FD_COUNT" -gt 10 ]; then
    echo -e "  ${YELLOW}WARNING: Possible FD leak detected ($FD_COUNT FDs)${RESET}"
fi

if [ "$PIPE_COUNT" -gt 20 ]; then
    echo -e "  ${YELLOW}WARNING: Possible pipe leak detected ($PIPE_COUNT pipe FDs)${RESET}"
fi

if [ "$FAILED_TESTS" -eq 0 ]; then
    echo -e "  ${GREEN}All siege tests passed!${RESET}"
else
    echo -e "  ${RED}$FAILED_TESTS test(s) failed${RESET}"
fi

echo "=========================================="

# Cleanup
rm -f /tmp/siege_static.txt /tmp/siege_mixed.txt

exit "$FAILED_TESTS"
