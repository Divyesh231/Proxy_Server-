# Computer Networks Proxy Server

A concurrent HTTP forward proxy for HTTP GET and HEAD requests, with HTTPS tunnelling through CONNECT. It includes a small in-memory response cache, hostname blocking, and traffic logs.

Open a terminal at the repository root:

    make
    ./proxy_server

The proxy listens on port 8080. Leave it running. Open a second terminal to send requests:

    curl --noproxy '' -x http://127.0.0.1:8080 http://example.com/
    curl --noproxy '' -x http://127.0.0.1:8080 https://example.com/

Press Ctrl+C in the first terminal to stop it. The log is written to logs/proxy.log.

## Local smoke test

From the repository root, run:

    bash tests/local_smoke_test.sh

It starts a temporary local web server and checks HTTP forwarding, cache miss/hit, five simultaneous requests, and a blocked-host 403 response. It restores the block-list and log files afterward.

## Demonstrations

Send the same ordinary HTTP GET twice. The log should show a miss and forwarding on the first request, then a hit on the second if the destination permits caching:

    curl --noproxy '' -x http://127.0.0.1:8080 http://example.com/
    curl --noproxy '' -x http://127.0.0.1:8080 http://example.com/
    tail -n 20 logs/proxy.log

To demonstrate access control, add a hostname to config/blocked_domains.txt, one exact hostname per line, then request it through the proxy. It should return 403 Forbidden. Remove the hostname afterward.

## Scope and cache policy

The proxy handles GET, HEAD, and CONNECT. CONNECT passes encrypted HTTPS bytes through a tunnel; the proxy does not inspect HTTPS content. Other HTTP methods receive 501 Not Implemented. Request headers are forwarded except hop-by-hop connection headers. The proxy closes each ordinary HTTP connection after one response.

Only GET responses with status 200 are considered for caching. Responses with Set-Cookie, any Cache-Control or Expires directive, or Vary are not cached. Requests with Cookie, Authorization, Cache-Control, or Pragma no-cache bypass cache lookup/storage. Other entries are held in memory (up to 32 entries) for at most 60 seconds. This conservative policy is a simple project policy, not a full implementation of the HTTP caching standard.

## Performance measurements

Measure a stable, cacheable HTTP URL directly and through the proxy. Repeat each command at least five times and report the mean latency and download speed; keep the URL and test conditions the same. Run the proxied request once more to capture a cache hit separately.

    curl -o /dev/null -sS -w 'direct time=%{time_total}s speed=%{speed_download}B/s\n' http://example.com/
    curl --noproxy '' -x http://127.0.0.1:8080 -o /dev/null -sS -w 'proxy time=%{time_total}s speed=%{speed_download}B/s\n' http://example.com/

Record the date, URL, number of runs, mean latency, and mean download speed in docs/performance.md. Network measurements vary; report the values observed in your Codespace.

The helper runs five direct requests, five forced proxy misses, and five cache-hit measurements, then prints the mean for each:

    bash scripts/benchmark.sh http://example.com/