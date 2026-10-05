#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."
make
mkdir -p config logs

block_list=config/blocked_domains.txt
log_file=logs/proxy.log
saved_block_list=$(mktemp)
saved_log=$(mktemp)
cp "$block_list" "$saved_block_list" 2>/dev/null || :
cp "$log_file" "$saved_log" 2>/dev/null || :

cleanup() {
    kill "${proxy_pid:-}" "${web_pid:-}" 2>/dev/null || :
    wait "${proxy_pid:-}" "${web_pid:-}" 2>/dev/null || :
    cp "$saved_block_list" "$block_list"
    cp "$saved_log" "$log_file"
    rm -f "$saved_block_list" "$saved_log"
}
trap cleanup EXIT

: > "$block_list"
: > "$log_file"

python3 -m http.server 18080 --bind 127.0.0.1 >/dev/null 2>&1 &
web_pid=$!
./proxy_server >/dev/null 2>&1 &
proxy_pid=$!
sleep 1

proxy_url=http://127.0.0.1:8080
target=http://127.0.0.1:18080/
curl --noproxy '' -fsS -x "$proxy_url" "$target" >/dev/null
curl --noproxy '' -fsS -x "$proxy_url" "$target" >/dev/null

client_pids=()
for _ in 1 2 3 4 5; do
    curl --noproxy '' -fsS -x "$proxy_url" "$target" >/dev/null &
    client_pids+=("$!")
done
for client_pid in "${client_pids[@]}"; do
    wait "$client_pid"
done

printf 'blocked.test\n' > "$block_list"
status=$(curl --noproxy '' -sS -o /dev/null -w '%{http_code}' -x "$proxy_url" http://blocked.test/)
test "$status" = 403
grep -q 'CACHE_MISS' "$log_file"
sleep 1
grep -q 'CACHE_HIT' "$log_file"
grep -q 'BLOCKED' "$log_file"
