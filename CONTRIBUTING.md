# Contributing to mag-usb

Thank you for your interest in contributing! This document explains how to build, test, and propose changes to mag-usb.

## Quick links
- Getting Started (users): docs/Getting-Started.md
- Hardware setup and udev rules: docs/Hardware-Setup.md
- Troubleshooting: docs/Troubleshooting.md
- Development guide (this file + docs/Development.md)

## Build locally
mag-usb is a C11 project built with CMake.

Recommended (CLion):
- Open the project root in CLion. The IDE provides Debug/Release CMake profiles under cmake-build-debug and cmake-build-release.
- Build target: mag-usb

Command line (plain CMake):
- Release example:
  - cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  - cmake --build build --target mag-usb

## Tests (CTest)
Unit tests (currently focused on the i2c-pololu helper) are integrated with CTest.

- Configure with tests on:
  - cmake -S . -B build -DBUILD_TESTING=ON
- Build tests:
  - cmake --build build --target i2c-pololu-tests
- Run:
  - (cd build && ctest --output-on-failure)

You can also run the test binary directly: ./build/i2c-pololu-tests

## Warnings and code hygiene
- The code is compiled with -Wall -Wextra -Wpedantic by default.
- You can opt-in to warnings-as-errors with -DENABLE_WERROR=ON. New code should compile cleanly with this option.
- C standard is C11; avoid compiler-specific extensions unless guarded.

## Coding guidelines (lightweight)
- Keep internal helpers static in .c files. Only expose public APIs via headers in src/.
- Prefer explicit casts for pointer signedness (e.g., to const uint8_t*) to avoid noisy warnings.
- Be defensive with error paths and check return values from I/O.
- Add brief comments for non-obvious logic (e.g., sensor value decoding).
- Follow existing formatting; no enforced clang-format at this time.

## Commit messages
- Summarize changes in a short subject line, then provide context in the body if needed.
- Reference issues where applicable (e.g., Fixes #123).

## Proposing changes
1. Fork and create a feature branch.
2. Make focused, minimal changes that are easy to review.
3. Ensure builds pass in Debug and Release, with -DENABLE_WERROR=ON if possible.
4. Run CTest and ensure all tests pass.
5. Update docs (README or docs/*) when behavior or interfaces change.
6. Open a Pull Request with a clear description and testing notes.

## Roadmap & issues
- See docs/Development.md for roadmap pointers.
- If you’re unsure about direction or design, feel free to open a discussion issue first.
