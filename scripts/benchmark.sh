#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 1 ]; then
    echo "Usage: bash scripts/benchmark.sh http://host/path" >&2
    exit 2
fi
url=$1
if [[ "$url" != http://* ]]; then
    echo "Use an HTTP URL for cache-miss/hit comparison." >&2
    exit 2
fi

cd "$(dirname "$0")/.."
direct_file=$(mktemp)
proxy_file=$(mktemp)
hit_file=$(mktemp)
trap 'rm -f "$direct_file" "$proxy_file" "$hit_file"' EXIT

echo "Direct connection (5 runs):"
for i in 1 2 3 4 5; do
    curl --noproxy '*' -o /dev/null -sS -w '%{time_total} %{speed_download}\n' "$url" | tee -a "$direct_file"
done
awk '{t += $1; s += $2} END {printf "Direct mean: %.6f s, %.0f B/s\n", t/NR, s/NR}' "$direct_file"

echo "Proxy connection, forced cache miss (5 runs):"
for i in 1 2 3 4 5; do
    curl --noproxy '' -x http://127.0.0.1:8080 -H 'Cache-Control: no-cache' -o /dev/null -sS -w '%{time_total} %{speed_download}\n' "$url" | tee -a "$proxy_file"
done
awk '{t += $1; s += $2} END {printf "Proxy mean: %.6f s, %.0f B/s\n", t/NR, s/NR}' "$proxy_file"

echo "Proxy cache hit (warming once, then 5 runs):"
curl --noproxy '' -x http://127.0.0.1:8080 -o /dev/null -sS "$url"
for i in 1 2 3 4 5; do
    curl --noproxy '' -x http://127.0.0.1:8080 -o /dev/null -sS -w '%{time_total} %{speed_download}\n' "$url" | tee -a "$hit_file"
done
awk '{t += $1; s += $2} END {printf "Cache-hit mean: %.6f s, %.0f B/s\n", t/NR, s/NR}' "$hit_file"

echo "Confirm CACHE_MISS and CACHE_HIT events in logs/proxy.log before recording the results."
