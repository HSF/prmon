# AGENTS.md

Guidance for AI coding agents working on **prmon** (PRocess MONitor). Read
this before changing code. Human-oriented detail lives in
[README.md](README.md) and [doc/](doc/); this file summarises what matters for
making correct, reviewable changes.

## What prmon is

- A small, standalone C++ program that monitors the resource consumption of a
  process **and all its children** (CPU, memory incl. PSS from `smaps`, I/O,
  network, process/thread counts, NVIDIA GPUs).
- Used to monitor very large numbers of HEP grid jobs (WLCG/HSF), so it must
  stay **lightweight, robust and dependency-light**. It measures resource
  consumption; it is *not* a profiler. Features that are heavy or not of
  general interest across millions of jobs are out of scope.
- **Linux only**: it depends on `/proc` (and `/sys`). Problems on other
  platforms are out of scope. prmon will not build/run usefully on macOS -
  if you are on a non-Linux host, say so and ask the user to test in a Linux
  container (e.g. podman/docker Ubuntu or AlmaLinux) rather than claiming a
  change is verified.

## Contribution rules (important)

- **Every PR must be linked to an issue the maintainers have accepted.**
  Unsolicited PRs are closed without discussion. Do not open PRs on the
  upstream repo (`HSF/prmon`) unless the user explicitly asks.
- AI assistance is allowed; the human submitter takes responsibility and
  should disclose AI use for non-routine work (design, architecture, complex
  problem solving).
- New features need an **integration test**. Comment code so others can
  follow it.
- Copyright is held by CERN; source files carry a header like
  `// Copyright (C) 2018-2026 CERN` / `// License Apache2 - see LICENCE file`.
  Update the end year when you substantially edit a file.

## Repository layout

| Path | Contents |
|---|---|
| `package/src/` | All C++ sources: `prmon.cpp` (main loop, CLI), one `*mon.{h,cpp}` pair per monitor, `parameter.{h,cpp}`, `registry.h`, `MessageBase` (logging), `prmonutils`, `utils` |
| `package/include/popl.hpp` | Bundled CLI option parser |
| `package/tests/` | Python integration test wrappers (`test_*.py`), C++ "burner" load generators, gtest unit tests (`test_values.cpp`, `test_fields.cpp`, `test_parameter_classes.cpp`) |
| `package/scripts/` | `prmon_plot.py`, `prmon_compress_output.py`, `precook_test.py` and `precooked_tests/` fake `/proc` trees |
| `submodules/` | `nlohmann_json`, `spdlog` (used if no system copy is found) |
| `cmake/` | Dependency, packaging, `clang-format`, `black`/`flake8` targets |
| `.github/` | CI workflows and the container build script `scripts/build-test.sh` |
| `doc/` | `ADDING_MONITORS.md`, `PRECOOKED_SOURCES.md`, `CONTRIBUTING.md`, `RELEASE_PROCEDURE.md` |

## Building and testing

```sh
git clone --recurse-submodules https://github.com/HSF/prmon.git
cmake -S . -B build -DBUILD_GTESTS=ON   # gtests are OFF by default
cmake --build build
cd build && ctest --output-on-failure
```

- C++ standard: **C++11** on `main` (`CMAKE_CXX_STANDARD` in the top-level
  `CMakeLists.txt`). Do not use newer language features unless the standard is
  deliberately raised.
- CMake >= 3.10. Python >= 3.6 for the tests; also keep test scripts working on
  current Python (e.g., 3.13+ removed `cgi`; avoid deprecated stdlib modules).
- Python test wrappers are copied into the build tree by `script_install()` in
  `package/tests/CMakeLists.txt` and run from `build/package/tests`, invoking
  `../prmon`. **Any new helper module or script a test imports or runs must be
  added with `script_install()`**, or it will be missing at test time.
- Shared test helpers live in `package/tests/prmon_test_utils.py`; decorate
  test methods with `@dump_on_failure` so prmon's output files are printed
  when a test fails (CI cannot retrieve them otherwise).
- Every test writes `prmon.txt` / `prmon.json` / `prmon.log` in the same
  directory, so these files belong to whichever test ran last.
- CI (`.github/workflows/ci.yml`) builds and tests in AlmaLinux 9/10 and
  Ubuntu 24.04/26.04 containers via `.github/scripts/build-test.sh`, with gcc
  (and clang on AlmaLinux 10). Failing tests print their output and
  `LastTest.log`.

## Formatting and linting

- C++: `clang-format --style=Google` (`cmake --build build --target
  clang-format`). A CI workflow also applies it on pushes to non-main
  branches.
- Python: `black` and `flake8` with the repo's `.flake8` (88 columns,
  E203 ignored); targets `black` and `flake8`.
- Run these before handing work back. If the tools are not installed, `uvx
  black` / `uvx flake8 --config .flake8` work without installing anything.

## Monitor architecture

Read [doc/ADDING_MONITORS.md](doc/ADDING_MONITORS.md) before adding or
changing a monitor. Key points and pitfalls:

- Monitors implement `Imonitor` (`package/src/Imonitor.h`) and self-register
  with `REGISTER_MONITOR(Imonitor, classname, "Description")`. They must also
  be added to `add_executable(prmon ...)` in `package/CMakeLists.txt`.
- Monitors are named for **what** they measure, not **how** they measure it
  (e.g. `powermon`, not `curlmon`).
- Use RAII: the monitor must be fully usable (or report `is_valid() == false`)
  after construction. **`is_valid()` is only checked once, at registration**
  in `prmon.cpp`; setting `valid = false` later has no effect on whether the
  monitor is polled or reported.
- Each cycle `update_stats(pids, read_path)` is called. The main loop only
  catches `std::ifstream::failure`; **any other exception escaping
  `update_stats` terminates prmon** (breaking the guarantee that prmon exits
  with the child's exit code). Catch and log errors inside the monitor.
- Values are `prmon::mon_value` (`unsigned long long`); averages are
  `double`. Use `prmon::parameter_list` for definitions and
  `prmon::monitored_list` / `monitored_value` for state.
  - Mark only genuinely cumulative counters as **monotonic**; a monotonic
    value can never decrease, so marking an instantaneous reading (e.g.
    watts, thread count) monotonic freezes its peak.
  - For counters that don't start at zero, pass an offset / use
    `set_offset()` (see `netmon`) rather than ad-hoc "initial" snapshots,
    and take the baseline in the constructor.
  - Before converting a `double` to `mon_value`, check it is finite,
    `>= 0` and `< 2^64` - out-of-range float-to-integer conversion is
    undefined behaviour.
- **`read_path`** prefixes every file the monitor reads (`read_path +
  "/proc/..."`) so tests can feed it precooked data. Honour it for every data
  source, and add a case to `package/tests/test_values.cpp` for new precooked
  data (see [doc/PRECOOKED_SOURCES.md](doc/PRECOOKED_SOURCES.md)).
- `wallmon` cannot be disabled. Monitors can be disabled with `--disable` or
  `PRMON_DISABLE_MONITOR`.
- Monitors are held in an `unordered_map`, so **text output column order is
  not stable** across monitor additions; consumers must use the header row.
  New monitors are on by default and add columns/JSON keys for every user -
  document them in the README.

## Robustness and security expectations

prmon runs inside other people's jobs on shared nodes, so treat all input it
reads as untrusted, including `/proc` contents, files named in environment
variables and responses from local services (an unprivileged port on
`localhost` can be bound by any user).

- Bound every read (response bodies, files - note `/proc`/`/sys` files report
  size 0, and FIFOs block on open) and every wait (use overall timeouts; the
  main loop is synchronous).
- When parsing text, cast to `unsigned char` before `<cctype>` functions,
  avoid locale-dependent number parsing where it matters, and match exact
  tokens rather than substrings.
- When using any external library or resource be extremely careful to avoid
  and blocking calls, calls that would result in resource exhaustion (e.g., 
  arbitrarily long reads), or leak any resources (sockets, memory, etc.)
  - e.g., if using libcurl: restrict protocols (`CURLOPT_PROTOCOLS_STR`), set
    connect and total timeouts, cap response size, disable proxies for local
    endpoints, check every `curl_easy_setopt` and the transfer result, treat
    HTTP errors as failures, and always `curl_easy_cleanup` exactly once (an
    RAII wrapper avoids leaks and double frees).
- Don't log credentials (e.g. user info inside URLs).
- Keep new runtime dependencies to a minimum; prefer loading optional ones at
  runtime (as NVML is, via `dlopen`) so prmon still builds and runs without
  them.

## Working practices for agents

- Keep changes minimal and in the style of the surrounding code; don't
  reformat unrelated files.
- Don't commit or push unless asked; when asked to add work to a branch, add
  only the files you changed.
- State clearly what you verified (built, unit tested, ran in a Linux
  container) and what you could not.
