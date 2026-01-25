# WebSocket Output Options Evaluation

This document captures the evaluation and comparison of approaches to add WebSocket output to mag-usb while keeping build and dependency constraints in mind.

## Key constraint check
- Both WebSocket++ and MengRao/websocket are C++ libraries; they require a C++ compiler and C++11+ (and typically Asio/Boost). That conflicts with “keep project C11” if you mean “no C++ toolchain required.”
- “Header-only” does not mean “dependency-free.” WebSocket++ usually depends on Boost.Asio or standalone Asio, so you would still be vendoring additional headers and accepting C++ ABI/toolchain implications.

## Option 1: Directly couple a WS library
- Pros: single binary; lowest runtime plumbing; straightforward to broadcast data to multiple clients.
- Cons: drags in C++ toolchain + Asio/Boost; changes build surface area; complicates CMake and portability; blurs C11 guarantee; harder to keep the core tiny.
- Fit with constraints: poor.

## Option 2: Directly couple behind a build flag
- Pros: best of both worlds for users; core can still be “C-only” builds; optional feature for advanced setups.
- Cons: still adds C++/Asio complexity for the optional path; dual build/test matrix; more maintenance.
- Fit with constraints: acceptable only if you are willing to support a C++ toolchain in the optional build.

## Option 3: Separate small WS bridge (websocketd-style)
- Pros: preserves C11 core; no C++ dependency in mag-usb; simple piping (stdout/pipe); easiest to keep versioning isolated; can be swapped/replaced without touching core.
- Cons: extra process; needs IPC (pipe/stdio); you must handle reconnection/backpressure in the bridge; packaging two binaries.
- Fit with constraints: best match if “C11 core” is a hard requirement.

## Library choice implications
- WebSocket++: header-only but C++11+ and Asio/Boost; good maturity, but heavyweight for your goals.
- MengRao/websocket: also C++-based; still violates C11-only goal.
- Net: neither meets “C11-only” unless you accept a C++ optional component.

## Recommended direction
- If C11 purity is non-negotiable: choose Option 3 and build a minimal WS bridge (could be in C++ if you are fine with that in the separate tool, or in C if you want everything C-only). This keeps mag-usb simple and stable while still enabling WS.
- If you can tolerate optional C++: choose Option 2 with a build flag and keep WS in a separate module. Users who do not want C++ just build without it.

## Alternative worth considering
- A small C-based embedded server library (e.g., Mongoose) can be vendored as C source rather than a C++ dependency. That would allow Option 1 or 2 without a C++ toolchain, but introduces a third-party C library and licensing considerations.

---

## Follow-up request and responses

User request:
"I am willing to sacrifice the 'C11 purity' constraint,  but I don't want ASIO/Boost involved. I would like to pursue the option 2 using the meng/rao library. I want that library vendored into the project.  This can all bive behind a build flag.  I will need test cases and a test harness to explore the results."

Assistant response:
"Added optional WebSocket output behind `-DENABLE_WEBSOCKET=ON`, vendored MengRao/websocket under `third_party/`, and wired runtime config + CLI flags (`-W`, `-w`, `-a`) so mag-usb can broadcast each JSON line to connected WS clients. I also added a basic test (`websocket-tests`) and a small `ws-client` harness for manual exploration; docs and the requirements spec now reflect the new capability.

Details and touch points
- Build flag and targets: `CMakeLists.txt` adds `ENABLE_WEBSOCKET`, `ws-client`, and `websocket-tests`
- Runtime integration: `src/main.c` polls the WS server in the output loop and broadcasts the JSON line
- Configuration + CLI: `src/config.c`, `src/config.toml`, `src/cmdmgr.c`, `src/main.h`
- Wrapper and tooling: `src/ws_bridge.h`, `src/ws_bridge.cpp`, `tools/ws_client.cpp`, `tests/test_websocket.cpp`
- Docs/reqs: `docs/Configuration.md`, `docs/Getting-Started.md`, `docs/Requirements.md`, `README.md`
- `third_party/mengrao-websocket/`

Tests not run.

If you want to verify:
```
cmake -S . -B build -DENABLE_WEBSOCKET=ON
cmake --build build --target mag-usb ws-client
cmake --build build --target websocket-tests
(cd build && ctest --output-on-failure)
```

To try the harness:
```
./build/mag-usb -W -w 8765 -a 0.0.0.0
./build/ws-client 127.0.0.1 8765 /
```

If you want adjustments (port defaults, max connections, or a dedicated WS thread), tell me which behavior you prefer."
