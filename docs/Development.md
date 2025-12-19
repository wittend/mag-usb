# Development Guide

This page is for developers working on mag-usb.
 
## Repository layout
- src/: C sources and headers
- tests/: Unit tests (CTest)
- install/: Deployment assets (e.g., udev rules)
- docs/: User and developer documentation
- assets/: Reference logs, notes, and images (keep large binaries here when necessary)

## Build matrix
- CMake 3.22+
- C11
- GCC or Clang
- Recommended warning flags are enabled by default: -Wall -Wextra -Wpedantic
- Optional: -DENABLE_WERROR=ON to treat warnings as errors (should be clean)

## Common targets
- mag-usb (main CLI)
- i2c-pololu-tests (unit tests for the Pololu adapter logic)

## Local builds
- Debug profile: faster iteration, symbols
- Release profile: optimized binary

CLion profiles are preconfigured (cmake-build-debug/release). From CLI:
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target mag-usb
```

## Running tests
```
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --target i2c-pololu-tests
(cd build && ctest --output-on-failure)
```

## Coding patterns
- Keep internal helper functions static and out of public headers.
- Avoid global state where possible; prefer passing pList* explicitly.
- Be strict on error handling: check return codes of system calls and I/O.
- Use explicit (void)param casts to silence intentional unused-parameter warnings.
- Cast to const uint8_t* when interacting with byte APIs to avoid signedness warnings.

## Style and tools
- No enforced formatter; follow existing code style.
- You may use clang-format/clang-tidy locally; do not submit sweeping reformat PRs.
- Consider enabling -DENABLE_WERROR=ON locally to catch regressions early.

## Release process (suggested)
- Update CHANGES.txt or a future CHANGELOG.md.
- Tag versions using SemVer (suggest starting at v0.1.0 when ready).
- Build Debug and Release; run CTest in both configurations.

## Roadmap ideas (for discussion)
- Expand unit tests to cover error cases and edge conditions in i2c-pololu.c.
- Add a mock adapter for offline testing.
- Introduce a simple CI workflow to build and run tests on push/PR.
- Split device-specific docs and usage examples into separate pages.
