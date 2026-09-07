# Contributing to Darknet Prototype

Thanks for your interest in helping out. This is an early-stage ARPG prototype, so contributions of all kinds are welcome — bug reports, balance feedback, documentation improvements, art, audio, and code.

## Quick Start

1. Fork the repository and clone your fork.
2. Create a focused branch: `git checkout -b feature/your-feature` or `fix/your-bug`.
3. Make your changes.
4. Run the validation gates locally (see below).
5. Open a pull request against `main` with a clear description and test results.

## Building and Running

```bash
mkdir build && cd build
cmake .. -A x64          # Windows MSVC
cmake --build . --config Release
```

Run the game:

```bash
./build/Release/darknet.exe
```

Run the headless autotest:

```bash
./build/Release/darknet.exe --headless --autotest --test-seconds=120 --seed=7
```

## Validation Gates

Before opening a PR, please make sure the following pass:

- **Release build**: `cmake --build build --config Release`
- **Debug build**: `cmake --build build --config Debug`
- **Unit tests**: `build/Release/darknet_tests.exe` (26 cases / 159 assertions)
- **Autotest seed 7**: `./validate.sh 120 7`
- **Autotest seed 20260821**: `./validate.sh 120 20260821`

Both autotest runs must end with `VALIDATION: PASSED`.

## What to Contribute

Good first contributions:

- Fix a P2/P3 issue listed in `audit/2026-09-06-full-audit.md`.
- Improve English documentation or code comments.
- Add unit tests for uncovered systems.
- Refactor a small helper out of the `Game` class.
- Balance enemy spawn rates, damage, or economy.

Please avoid large architectural refactors without discussing them first in an issue or discussion.

## Code Style

- C++17. Prefer standard library and raylib idioms.
- Keep functions short and focused.
- Add comments only when the "why" is not obvious from the code.
- Do not commit IDE files, build outputs, or runtime logs.

## Reporting Bugs

Use the [Bug Report issue template](https://github.com/OmegaSoftDLL/darknet-prototype/issues/new?template=bug_report.yml). Include:

- Steps to reproduce.
- Expected vs actual behavior.
- Seed and phase if relevant.
- Build configuration (Release/Debug, compiler).

## Questions and Ideas

Open a [Discussion](https://github.com/OmegaSoftDLL/darknet-prototype/discussions) for questions, design ideas, or show-and-tell.

## License

By contributing, you agree that your contributions will be licensed under the [MIT License](./LICENSE).
