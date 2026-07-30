#!/usr/bin/env bash
#===============================================================================
#  Webserv Siege Evaluation Script
#  Usage: ./siege_eval.sh <url> [webserv_pid]
#  Example: ./siege_eval.sh http://127.0.0.1:8080 12345
#===============================================================================

set -euo pipefail

# --- Colors -------------------------------------------------------------------
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'

# --- Args ---------------------------------------------------------------------
URL="${1:-}"
PID="${2:-}"

if [ -z "$URL" ]; then
    echo "Usage: $0 <url> [webserv_pid]"
    echo "Example: $0 http://127.0.0.1:8080 12345"
    exit 1
fi

# --- Auto-detect PID ----------------------------------------------------------
if [ -z "$PID" ]; then
    PID=$(pgrep -f "./webserv" | head -1 || pgrep -f "webserv" | head -1 || true)
    if [ -n "$PID" ]; then
        echo -e "${CYAN}[INFO]${RESET} Auto-detected webserv PID: $PID"
    fi
fi

# --- Parse host/port for connection checks ------------------------------------
HOST=$(echo "$URL" | sed -E 's|https?://||; s|/.*||; s|:.*||')
PORT=$(echo "$URL" | sed -E 's|https?://[^/]+||; s|/.*||' | tr -d '/')
if [ -z "$PORT" ]; then
    PORT=$(echo "$URL" | sed -E 's|https?://[^/]+:||; s|/.*||')
fi
if [ -z "$PORT" ] || [ "$PORT" = "$HOST" ]; then
    PORT="80"
    [[ "$URL" == https* ]] && PORT="443"
fi

TMPDIR=$(mktemp -d)
trap 'rm -rf "$TMPDIR"' EXIT

# --- Helpers ------------------------------------------------------------------
phase() {
    echo ""
    echo -e "${BOLD}${BLUE}════════════════════════════════════════════════════════════════${RESET}"
    echo -e "${BOLD}${BLUE}  PHASE $1: $2${RESET}"
    echo -e "${BOLD}${BLUE}════════════════════════════════════════════════════════════════${RESET}"
}
ok()   { echo -e "${GREEN}[PASS]${RESET} $1"; }
fail() { echo -e "${RED}[FAIL]${RESET} $1"; }
warn() { echo -e "${YELLOW}[WARN]${RESET} $1"; }
info() { echo -e "${CYAN}[INFO]${RESET} $1"; }

get_mem() {
    if [ -n "${PID:-}" ] && kill -0 "$PID" 2>/dev/null; then
        ps -o rss= -p "$PID" 2>/dev/null | tr -d ' ' || echo "0"
    else
        echo "0"
    fi
}

get_conns() {
    local n
    if command -v ss &>/dev/null; then
        n=$(ss -tan 2>/dev/null | grep -cE ":$PORT[[:space:]]" || true)
    else
        n=$(netstat -tan 2>/dev/null | grep -cE ":$PORT[[:space:]]" || true)
    fi
    echo "${n:-0}"
}

get_hanging() {
    local n
    if command -v ss &>/dev/null; then
        n=$(ss -tan 2>/dev/null | grep -E ":$PORT[[:space:]]" | grep -cE "CLOSE-WAIT|FIN-WAIT-1|FIN-WAIT-2|LAST-ACK" || true)
    else
        n=$(netstat -tan 2>/dev/null | grep -E ":$PORT[[:space:]]" | grep -cE "CLOSE_WAIT|FIN_WAIT_1|FIN_WAIT_2|LAST_ACK" || true)
    fi
    echo "${n:-0}"
}

# --- Deps ---------------------------------------------------------------------
for cmd in siege curl ps; do
    if ! command -v "$cmd" &>/dev/null; then
        echo -e "${RED}Error: '$cmd' is required but not installed.${RESET}"
        exit 1
    fi
done

echo -e "${BOLD}Target URL:${RESET} $URL"
echo -e "${BOLD}Host:${RESET}     $HOST"
echo -e "${BOLD}Port:${RESET}     $PORT"
[ -n "$PID" ] && echo -e "${BOLD}PID:${RESET}      $PID"
echo ""

#===============================================================================
# PHASE 1: Server Reachability
#===============================================================================
phase "1" "Server Reachability"
HTTP_CODE=$(curl -s -o /dev/null --max-time 5 -w "%{http_code}" "$URL" || echo "000")
if [ "$HTTP_CODE" = "200" ]; then
    ok "Server reachable (HTTP 200)"
else
    fail "Server unreachable at $URL (HTTP $HTTP_CODE)"
    exit 1
fi

#===============================================================================
# PHASE 2: siege -b Benchmark — Availability > 99.5%
#===============================================================================
phase "2" "Siege -b Benchmark (Availability must be > 99.5%)"

info "Running: siege -b -t30s $URL"
siege -b -t30s "$URL" 2>&1 | tee "$TMPDIR/siege_bench.log" | tail -n 20

AVAIL=$(grep "Availability:" "$TMPDIR/siege_bench.log" | awk '{print $2}' | tr -d '%' || echo "0")
TRANS=$(grep "Transactions:" "$TMPDIR/siege_bench.log" | awk '{print $2}' || echo "0")
FAILED=$(grep "Failed transactions:" "$TMPDIR/siege_bench.log" | awk '{print $3}' || echo "0")

info "Results → Availability: ${AVAIL}% | Transactions: $TRANS | Failed: $FAILED"

if awk "BEGIN {exit !($AVAIL > 99.5)}" 2>/dev/null; then
    ok "Availability ${AVAIL}% > 99.5%"
else
    fail "Availability ${AVAIL}% ≤ 99.5%"
fi

#===============================================================================
# PHASE 3: Memory Leak Detection
#===============================================================================
phase "3" "Memory Leak Detection (Monitor process memory usage)"

if [ -z "$PID" ] || ! kill -0 "$PID" 2>/dev/null; then
    warn "Skipping memory test — provide webserv PID as 2nd argument"
else
    MEM_BASE=$(get_mem)
    info "Baseline RSS: ${MEM_BASE} KB"

    info "Running sustained siege -b -t60s in background..."
    siege -b -t60s "$URL" >/dev/null 2>&1 &
    SIEGE_PID=$!

    > "$TMPDIR/mem.log"
    for i in $(seq 1 6); do
        sleep 10
        MEM_NOW=$(get_mem)
        echo "$MEM_NOW" >> "$TMPDIR/mem.log"
        info "  +${i}0s → RSS: ${MEM_NOW} KB"
    done

    wait "$SIEGE_PID" 2>/dev/null || true
    MEM_END=$(get_mem)
    info "Final RSS: ${MEM_END} KB"

    FIRST=$(head -1 "$TMPDIR/mem.log")
    LAST=$(tail -1 "$TMPDIR/mem.log")
    DIFF=$((LAST - FIRST))

    if [ "$DIFF" -lt 50 ]; then
        ok "Memory stable (Δ ${DIFF} KB — no leak detected)"
    elif [ "$DIFF" -lt 200 ]; then
        warn "Memory increased by ${DIFF} KB (minor, monitor closely)"
    else
        fail "Memory increased by ${DIFF} KB — possible leak"
    fi
fi

#===============================================================================
# PHASE 4: Hanging Connection Check
#===============================================================================
phase "4" "Hanging Connection Check"

BEFORE_TOTAL=$(get_conns)
BEFORE_HANG=$(get_hanging)
info "Before load → Total: $BEFORE_TOTAL | Hanging: $BEFORE_HANG"

info "Running burst siege -b -t15s..."
siege -b -t15s "$URL" >/dev/null 2>&1

sleep 2

AFTER_TOTAL=$(get_conns)
AFTER_HANG=$(get_hanging)
info "After load  → Total: $AFTER_TOTAL | Hanging: $AFTER_HANG"

if [ "$AFTER_HANG" -eq 0 ]; then
    ok "No hanging connections detected"
else
    fail "$AFTER_HANG connection(s) stuck in CLOSE-WAIT / FIN-WAIT / LAST-ACK"
    command -v ss &>/dev/null && ss -tan 2>/dev/null | grep -E ":$PORT[[:space:]]" | grep -E "CLOSE-WAIT|FIN-WAIT-1|FIN-WAIT-2|LAST-ACK" || true
fi

#===============================================================================
# PHASE 5: Indefinite Siege Test (Long-running stability)
#===============================================================================
phase "5" "Indefinite Siege Test (Should run without restart)"

info "Running siege -b -t300s (5 min) to verify indefinite stability..."
info "Criterion: server must survive indefinitely without restart"

siege -b -t300s "$URL" 2>&1 | tee "$TMPDIR/siege_long.log" | tail -n 20

LONG_AVAIL=$(grep "Availability:" "$TMPDIR/siege_long.log" | awk '{print $2}' | tr -d '%' || echo "0")
LONG_FAILED=$(grep "Failed transactions:" "$TMPDIR/siege_long.log" | awk '{print $3}' || echo "0")
LONG_TRANS=$(grep "Transactions:" "$TMPDIR/siege_long.log" | awk '{print $2}' || echo "0")

info "Long-run → Availability: ${LONG_AVAIL}% | Failed: $LONG_FAILED | Trans: $LONG_TRANS"

if awk "BEGIN {exit !($LONG_AVAIL > 99.5)}" 2>/dev/null; then
    ok "Long-run availability ${LONG_AVAIL}% > 99.5%"
else
    fail "Long-run availability ${LONG_AVAIL}% ≤ 99.5%"
fi

#===============================================================================
# PHASE 6: Controlled Load (-c, -d, -r)
#===============================================================================
phase "6" "Controlled Load Test (-c -d -r to limit connections/sec)"

echo "$URL" > "$TMPDIR/urls.txt"

info "Running: siege -c 50 -d 1 -r 1000 -f urls.txt"
info "This limits the connection rate with 50 clients, 1s max delay, 1000 reps each"

siege -c 50 -d 1 -r 1000 -f "$TMPDIR/urls.txt" 2>&1 | tee "$TMPDIR/siege_ctrl.log" | tail -n 20

CTRL_AVAIL=$(grep "Availability:" "$TMPDIR/siege_ctrl.log" | awk '{print $2}' | tr -d '%' || echo "0")
CTRL_FAILED=$(grep "Failed transactions:" "$TMPDIR/siege_ctrl.log" | awk '{print $3}' || echo "0")

info "Controlled → Availability: ${CTRL_AVAIL}% | Failed: $CTRL_FAILED"

if awk "BEGIN {exit !($CTRL_AVAIL > 99.5)}" 2>/dev/null; then
    ok "Controlled load availability ${CTRL_AVAIL}% > 99.5%"
else
    fail "Controlled load availability ${CTRL_AVAIL}% ≤ 99.5%"
fi

#===============================================================================
# PHASE 7: Final Memory Verification
#===============================================================================
phase "7" "Final Memory Verification"

if [ -n "$PID" ] && kill -0 "$PID" 2>/dev/null; then
    MEM_FINAL=$(get_mem)
    info "Final RSS: ${MEM_FINAL} KB (baseline was ${MEM_BASE:-0} KB)"
    if [ "${MEM_BASE:-0}" -gt 0 ] && [ "$MEM_FINAL" -gt "$((MEM_BASE * 3))" ]; then
        fail "Memory more than tripled since baseline — likely leak"
    else
        ok "Memory within acceptable bounds"
    fi
else
    warn "PID not available for final memory check"
fi

#===============================================================================
# SUMMARY
#===============================================================================
phase "SUMMARY" "Evaluation Complete"

echo ""
echo -e "${BOLD}Results:${RESET}"
printf "  %-35s %s\n" "Phase 2 (siege -b avail):"     "${AVAIL}%"
printf "  %-35s %s\n" "Phase 3 (memory Δ):"           "${DIFF:-N/A} KB"
printf "  %-35s %s\n" "Phase 4 (hanging conns):"      "$AFTER_HANG"
printf "  %-35s %s\n" "Phase 5 (long-run avail):"     "${LONG_AVAIL}%"
printf "  %-35s %s\n" "Phase 6 (controlled avail):"   "${CTRL_AVAIL}%"
echo ""

# Final verdict
PASS=0
awk "BEGIN {exit !($AVAIL > 99.5)}" 2>/dev/null && ((PASS++))
awk "BEGIN {exit !($LONG_AVAIL > 99.5)}" 2>/dev/null && ((PASS++))
awk "BEGIN {exit !($CTRL_AVAIL > 99.5)}" 2>/dev/null && ((PASS++))

if [ "$PASS" -eq 3 ] && [ "$AFTER_HANG" -eq 0 ]; then
    echo -e "${BOLD}${GREEN}✓ ALL CRITERIA PASSED${RESET}"
else
    echo -e "${BOLD}${RED}✗ SOME CRITERIA FAILED${RESET}"
fi
