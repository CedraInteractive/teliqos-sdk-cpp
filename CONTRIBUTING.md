# Contributing to teliqos-sdk-cpp

Thank you for your interest in contributing! Here's how to get started.

## Getting Started

1. Fork the repository
2. Clone your fork and create a branch:
   ```bash
   git clone https://github.com/YOUR_USERNAME/teliqos-sdk-cpp.git
   cd teliqos-sdk-cpp
   git checkout -b my-feature
   ```
3. Build and run tests:
   ```bash
   cmake -B build -DTELIQOS_BUILD_TESTS=ON
   cmake --build build
   cd build && ctest -C Debug --output-on-failure
   ```

## Development Guidelines

- **C++17** — no newer standard features
- **No external dependencies** without discussion — we keep the SDK lightweight
- **Thread safety** — any shared state must be protected
- **Platform support** — changes should work on Windows, macOS, Linux, and Android
- **Tests** — new features need tests, bug fixes need a regression test

## Pull Request Process

1. Make sure all tests pass on your platform
2. Keep PRs focused — one feature or fix per PR
3. Write a clear description of what changed and why
4. Reference any related issues

## Code Style

- Follow the existing style in the codebase
- `Teliqos::` namespace for public API, `Teliqos::Internal::` for internals
- snake_case for files, PascalCase for types, camelCase for functions/variables

## Reporting Bugs

Use the [bug report template](https://github.com/CedraInteractive/teliqos-sdk-cpp/issues/new?template=bug_report.yml) with:
- SDK version and platform
- Minimal reproduction code
- Expected vs actual behavior

## License

By contributing, you agree that your contributions will be licensed under the MIT License.
