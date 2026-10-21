# mag-usb: HamSCI fork retired, and notes on the MQTT path

**From:** HamSCI / sigmond station side (AC0G, mijahauan)
**Date:** 2026-08-07
**Re:** `wittend/mag-usb` at `6e660577` (v0.0.9)

---

## 1. We've retired our fork — thank you

Since you took the May patches, `HamSCI/mag-usb` no longer carries anything of
its own. We verified against `wittend/master` that everything `mag-recorder`
depends on is upstream:

- `-f <config>` — the flag our recorder relies on to point mag-usb at a
  generated config instead of the discovery path
- `-A <hex addr>` address override and `-P` register readback
- the CC/NOS registers actually being programmed on-chip

All three CLI flags are in your getopt string
(`"h?B:c:CD:g:PMSQTVO:ui:o:Ww:a:f:A:"`), so we confirmed by behaviour rather
than by history — in several cases you reimplemented rather than merged, which
is fine by us and honestly produced cleaner results in `magdata.c`.

That left our `sigmond-integration` branch changing exactly one thing: the
default of `ENABLE_WEBSOCKET`. Since our build script passes that flag
explicitly anyway, the branch had stopped earning its keep and was just drift —
it had quietly fallen 39 commits behind while still being what our shipped
binary was built from. So:

- `HamSCI/mag-usb:master` is now a straight mirror of `wittend/master`
- `sigmond-integration` is deleted (preserved as tag
  `sigmond-integration-retired-20260807`)
- our build tracks your `master` directly from here on

We also switched our build to `ENABLE_WEBSOCKET=OFF`, because your MQTT work
supersedes it for our use — broker-mediated means only the broker needs
exposing and clients can live anywhere, which is exactly the property we want.
Dropping it also takes the C++11 toolchain and the vendored
`mengrao-websocket` header out of our build, so we're back to a pure-C artifact
plus OpenSSL.

**Build verified** on Debian 13 / gcc 14.2 / cmake 3.31: v0.0.9 builds clean,
73512 bytes, links only `libssl`/`libcrypto`. The 1 Hz cadence rework
(`clock_nanosleep(TIMER_ABSTIME)`) and the `missed_sample` diagnostic both land
correctly for us — we read stdout for samples and let stderr inherit to the
journal, so the new diagnostics are logged without being mistaken for data.

One heads-up on the register work: because CC/NOS are now genuinely written to
the chip, gain and resolution can change relative to the old binary. We're
running a before/after continuity comparison on the same magnetometer before we
let the new binary near archived PSWS data. Not a complaint — it's the correct
behaviour, it just needs a calibration check on our end.

---

## 2. MQTT — security and robustness notes

We read `src/mqtt_client.c` closely because we're planning to enable this. The
protocol work is sound in the parts that matter most (the PUBLISH path builds a
proper variable-byte-integer remaining length and bounds-checks against the
buffer — that's the part people usually get wrong). The notes below are about
the connection's trust and lifetime, not the framing.

Happy to send patches for any of these if that's easier than describing them.

### 2.1 TLS is encrypted but not authenticated — highest priority

`mqtt_client_connect()`, around line 104:

```c
client->ssl_ctx = SSL_CTX_new(TLS_client_method());
client->ssl     = SSL_new(client->ssl_ctx);
if (SSL_connect(client->ssl) <= 0) return -1;
```

There's no `SSL_CTX_set_verify()`, no trust store
(`SSL_CTX_set_default_verify_paths()` / `load_verify_locations()`), no hostname
check, and no SNI. OpenSSL clients default to `SSL_VERIFY_NONE`, so the
certificate the broker presents is accepted whatever it is.

The practical effect is that `use_tls = true` gives confidentiality against a
passive observer but no protection against an active one: anything that can
answer on the broker's address can present any certificate, terminate the TLS
session, read the telemetry, and — because the client subscribes to
`<topic>/command` — issue device commands back. Credentials sent by
`mqtt_client_authenticate()` go over that same session.

This matters more than usual for this design specifically. The stated appeal of
MQTT here is that only the broker needs network exposure; that makes the broker
the trust boundary, and right now the client doesn't verify it's talking to the
real one.

Suggested minimum:

```c
SSL_CTX_set_verify(client->ssl_ctx, SSL_VERIFY_PEER, NULL);
SSL_CTX_set_default_verify_paths(client->ssl_ctx);          /* or a configured CA file */
SSL_set_tlsext_host_name(client->ssl, host);                /* SNI */
SSL_set1_host(client->ssl, host);                           /* hostname verification */
```

plus checking `SSL_get_verify_result()` after `SSL_connect()`. A config key for
a CA bundle path would cover self-signed/private-CA deployments, which we'd
expect to be common for station brokers.

### 2.2 Inbound parser hardening

The receive loop (around line 246) is the one place broker-controlled bytes are
parsed, so it's worth tightening:

- **Unbounded remaining-length decode.** The `do { ... } while (byte & 128)`
  loop has no iteration cap. MQTT 3.1.1 limits this field to 4 bytes
  (268 435 455); as written, a peer can keep setting the continuation bit and
  drive `rem_len` (and `multiplier`) arbitrarily.
- **`malloc(rem_len)` return isn't checked.** With a large `rem_len` the
  allocation fails and `msg` is passed to `recv_all()` as NULL.
- **`msg[0]`/`msg[1]` are read before `rem_len >= 2` is established**, and the
  guard `topic_len <= rem_len - 2` underflows when `rem_len < 2` (size_t), so
  it doesn't catch that case. The `topic_len < 255` test keeps the stack
  `memcpy` in bounds, so this reads a couple of bytes past a small heap
  allocation rather than smashing anything — but it's worth closing.

Capping the length decode at 4 bytes, checking the allocation, and requiring
`rem_len >= 2` before touching `msg[0..1]` would cover all three.

### 2.3 No reconnect

`mqtt_client.c` has no reconnect path. Any transient interruption — broker
restart, network blip, TLS error, a NAT idle timeout — ends the feed
permanently until `mag-usb` itself is restarted.

Recording is unaffected (`main.c` correctly treats MQTT init failure as
non-fatal and continues), so this degrades the live feed rather than the
instrument, which is the right failure ordering. But for a station that runs
unattended for months, "the dashboard silently stopped some weeks ago" is the
likely outcome. A backoff-and-retry around connect, with the publish path
tolerating a disconnected client, would make it durable.

Worth noting for completeness: the **keepalive is fine in normal operation**.
CONNECT advertises 60 s and `PINGREQ` is never sent, but any control packet
resets the broker's timer and the 1 Hz publish rate does that comfortably. It
only becomes a factor if sampling stalls for a minute — at which point the
missing reconnect is the bigger problem anyway. Mentioning it only so the
absent `PINGREQ` doesn't look like an oversight worth fixing on its own.

### 2.4 CONNECT remaining-length limit

You've already flagged this in the code:

```c
*len_ptr = (ptr - len_ptr - 1); // Only works for lengths < 128
```

Just a note that it becomes reachable once credentials and a per-station
client_id are in use: client_id + username + password + ~14 bytes of fixed
fields is comfortably past 127 with, say, a 64-character password. The failure
is silent — a malformed packet rather than an error — so it may be worth either
reusing the variable-byte encoder from the publish path or returning an error
when the total exceeds 127. The CONNECT buffer (`uint8_t buf[512]`) also isn't
bounds-checked against those same three strings.

### 2.5 Defaults that bite in a fleet

Small things, but they surface immediately with more than one station on a
shared broker:

- `client_id` defaults to the constant `"mag-usb-client"` and `topic` to
  `"mag-usb/data"`. Two stations with the same client_id will fight — brokers
  disconnect the older session when a duplicate connects, so they'll knock each
  other offline in a loop. We'll derive both from station identity on our side,
  but a per-host default (hostname-derived) would save everyone else the
  discovery.
- `broker_port` defaults to `8081` (your comment notes it's a placeholder);
  1883/8883 would match expectation.
- `use_tls` defaults `TRUE` while `username`/`password` default to NULL, so the
  out-of-box state is anonymous TLS to localhost:8081.

### 2.6 Build: MQTT is unconditional

`src/mqtt_client.c` is compiled into the `mag-usb` target directly and
`CMakeLists.txt` does `find_package(OpenSSL REQUIRED)`, so OpenSSL is now a
hard build dependency even for deployments that never enable MQTT. An
`option(ENABLE_MQTT ... ON)` mirroring `ENABLE_WEBSOCKET` would let
constrained/pure-C builds opt out — useful for the Pi-class targets. Not urgent
for us; we have OpenSSL either way.

---

## 3. Where we are

We're tracking your `master` now, MQTT compiled in but off
(`mqtt_enable = FALSE`), and we render mag-usb's config from our own operator
settings so nothing changes for our stations until we deliberately enable it.

Our plan is to expose MQTT through `mag-recorder` with per-station `client_id`
and `topic`, treating the MQTT stream as best-effort live visualisation and our
own spool as the authoritative archive — the MQTT messages carry mag-usb's
second-resolution timestamp rather than the one our supervisor re-stamps, which
is fine for a dashboard and not what we'd want in science data.

Before we turn it on for real we'd want 2.1 (TLS verification) addressed, since
enabling an unverified TLS session that accepts remote commands isn't something
we can responsibly deploy across the fleet. Glad to contribute that patch and
the 2.2 hardening if you'd like — just say which you'd prefer to take yourself.

Thanks again for folding in the earlier work, and for the cadence and register
fixes — those cleaned up real problems on our end.
