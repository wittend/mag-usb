# Local changes to mengrao-websocket in mag-usb

This directory is a vendored copy of [MengRao/websocket](https://github.com/MengRao/websocket)
(MIT, see `LICENSE`). mag-usb carries the modifications below. They are
recorded here so that they can be reviewed and sent upstream.

## 1. Case-insensitive handshake header matching (mag-usb #13)

**File:** `websocket.h`, `WSServer` handshake parser (request side only).

**Problem.** The server compared HTTP header names and token values with
`memcmp`, which is case-sensitive. RFC 9110 §5.1 makes header names
case-insensitive, and RFC 6455 §4.2.1 makes `Upgrade` and `Connection`
values case-insensitive token lists. Clients that send `sec-websocket-key`
(lowercase, as Deno's `WebSocket` and most HTTP/2-era libraries do), or
`Connection: keep-alive, Upgrade`, or `Upgrade: WebSocket`, were rejected with
`400 Bad Request`.

**Change.**
- Added two helpers in `namespace websocket`:
  - `header_is(name, len, expected)`: case-insensitive comparison of a header
    name, with the length checked first.
  - `has_token(list, len, token)`: case-insensitive search of a
    comma-separated token list.
- The server handshake now uses `header_is` for every header name it reads,
  and `has_token` for the `Upgrade` and `Connection` values.
- `#include <strings.h>` for `strncasecmp`.

**Not changed.** The client-side response parser (the `WSClient` code that
reads the server's `101` response, around the `Upgrade`/`Connection` checks)
still compares case-sensitively. The mag-usb server does not use that path;
only the loopback test and the `ws-client` harness do. It has the same pattern
and could take the same fix.

**Behaviour.** Strictly more permissive for the two checked values: any case
or token-list position is accepted. Requests without an `Upgrade` token in
`Connection`, or with a different `Upgrade` value, are still rejected.

**Tests.** `tests/test_websocket.cpp` (loopback) still passes. The raw-socket
handshake cases (canonical, lowercase names, mixed-case values, missing
Upgrade token, wrong Upgrade value) were checked against this header and
against the original; the original fails the lowercase and mixed-case cases.

**Upstream.** This is a candidate for a PR to MengRao/websocket. The
upstream code may have changed since the copy here was taken.
