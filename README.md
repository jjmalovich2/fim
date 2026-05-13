# fim — File Integrity Monitor

`fim` is a lightweight, Linux-specific file integrity monitor written in C++17. It recursively watches a target directory using the kernel's `inotify` subsystem, computes cryptographic BLAKE3 hashes of every regular file, and emits JSON Lines alerts to stdout and a rotating log file whenever files are created, modified, or deleted.

On first run, `fim` builds a baseline database by hashing all files under the watched directory. On subsequent runs it loads that baseline from disk and compares new hashes. When a change is detected, the baseline is automatically updated and persisted.

---

## Features

- **Real-time monitoring** via Linux `inotify` with recursive directory watches.
- **Cryptographic hashing** using BLAKE3 for fast, strong integrity checks.
- **Zero-copy file reads** through `mmap` where possible (empty files handled explicitly).
- **Structured logging** as JSON Lines, suitable for ingestion by log aggregation tools.
- **Automatic baseline management** — created on first run, updated on every change, persisted to disk.
- **Severity levels** — `INFO`, `LOW`, `MEDIUM`, `CRITICAL` depending on event type.
- **Self-contained build** — no external package manager or system libraries required.

---

## Technology Stack

| Component     | Technology / Library                                    |
|---------------|---------------------------------------------------------|
| Language      | C++17                                                   |
| Build system  | GNU Make                                                |
| File watching | Linux `inotify` API (`sys/inotify.h`)                   |
| Hashing       | BLAKE3 (vendored C implementation)                      |
| JSON          | nlohmann/json 3.11.3 (single-header, `include/json.hpp`)|
| File I/O      | `mmap` / `munmap` for efficient reads                   |

---

## Prerequisites

- Linux kernel with `inotify` support.
- `g++` with C++17 support.
- `gcc` for compiling the vendored BLAKE3 C sources.
- GNU Make.

No external libraries need to be installed; all dependencies are vendored in the repository.

---

## Building

Clone the repository and run `make` from the project root:

```bash
make
```

This compiles all C++ sources under `src/`, the selected BLAKE3 C sources under `vendor/blake3/c/`, and produces the `fim` binary.

To remove build artifacts:

```bash
make clean
```

### Compiler flags

- C++: `-std=c++17 -Wall -Wextra -O2 -Iinclude -Ivendor/blake3/c`
- C:   `-O3 -Wall -Ivendor/blake3/c`
- BLAKE3 SIMD is explicitly disabled for portability: `-DBLAKE3_NO_SSE2 -DBLAKE3_NO_SSE41 -DBLAKE3_NO_AVX2 -DBLAKE3_NO_AVX512`

---

## Usage

Run the monitor from the project root:

```bash
./fim
```

The program will block indefinitely, watching the hard-coded target directory (`src/`). On first execution it builds `fim_baseline.json`. On later runs it loads the existing baseline.

Open a second terminal and create, edit, or delete files under `src/` to generate events. Alerts are printed to stdout and appended to `logs.jsonl`.

### Stopping the monitor

Send `SIGINT` (Ctrl+C) to terminate the process cleanly.

---

## Architecture

The codebase is organized around four abstract interfaces with concrete implementations:

1. **IHasher / Blake3Hasher**
   - Computes a lower-case hex BLAKE3 hash for a given file path.
   - Uses `open()`, `fstat()`, `mmap()`, and `munmap()` for efficient reads.
   - Falls back to explicit empty-file handling because `mmap` fails on zero-byte files.
   - Returns an empty string if the file cannot be opened or mapped; the caller silently drops the event.

2. **IMonitor / InotifyMonitor**
   - Wraps the Linux `inotify` API.
   - Recursively adds watches for the target directory and all subdirectories.
   - Blocks in a `read()` loop and invokes a user-provided `std::function` callback for each relevant event.
   - Watched mask: `IN_MODIFY | IN_CREATE | IN_DELETE | IN_ATTRIB`.
   - Directory-only events are filtered out inside the callback.

3. **IBaseline / Baseline**
   - Persists the mapping `filepath -> hash` as a JSON object in `fim_baseline.json`.
   - Loaded at startup and saved after every detected change.

4. **IAlert / Alert**
   - Emits JSON Lines records to stdout and `logs.jsonl`.
   - Each record contains `timestamp`, `severity`, `file`, `event_type`, and `details`.
   - Flushes output immediately (`std::flush`) to minimize log loss on crash.

`main.cpp` wires the components together: instantiates each object, loads or builds the baseline, installs the event callback lambda, and starts the blocking monitor loop.

---

## Directory Layout

```
.
├── Makefile              # Top-level build
├── src/                  # C++ implementations
│   ├── main.cpp          # Entry point, wires all components
│   ├── Blake3Hasher.cpp  # mmap + BLAKE3 hashing
│   ├── InotifyMonitor.cpp# inotify setup and event loop
│   ├── Baseline.cpp      # JSON baseline persistence
│   └── Alert.cpp         # JSON Lines logging
├── include/              # Headers (interfaces + concrete classes)
│   ├── I*.hpp            # Abstract interfaces
│   ├── Blake3Hasher.hpp
│   ├── InotifyMonitor.hpp
│   ├── Baseline.hpp
│   ├── Alert.hpp
│   └── json.hpp          # nlohmann/json (vendor)
├── vendor/blake3/c/      # BLAKE3 C sources
│   ├── blake3.c
│   ├── blake3.h
│   ├── blake3_dispatch.c
│   └── blake3_portable.c
├── tests/                # Empty — reserved for future tests
├── build/                # Object files and dependency tracking
├── fim_baseline.json     # Runtime baseline database (generated)
└── logs.jsonl            # Runtime alert log (generated)
```

---

## Runtime Behavior

- **Target directory:** `src/` (hard-coded in `main.cpp`).
- **Baseline file:** `fim_baseline.json`.
- **Log file:** `logs.jsonl`.
- **First run:** Baseline is built from all regular files under `src/` and saved.
- **Subsequent runs:** Existing baseline is loaded.

### Event handling

| Event            | Severity  | Action                              |
|------------------|-----------|-------------------------------------|
| `CREATED`        | LOW       | Hash file, add to baseline, save.   |
| `MODIFIED`       | CRITICAL  | Re-hash file; if changed, alert and update baseline. |
| `DELETED`        | CRITICAL  | Emit alert, remove from baseline, save. |
| `ATTRIB_CHANGED` | UNKNOWN   | Falls through to hash comparison logic. |

Directory-only events are ignored.

---

## Security Considerations

- BLAKE3 provides fast, cryptographically strong hashing suitable for integrity verification.
- `mmap` is used with `PROT_READ | MAP_PRIVATE`, ensuring the mapping is read-only and private to the process.
- Alerts are flushed to disk immediately to reduce the risk of log loss in the event of a crash.
- The monitor detects unauthorized file modifications and deletions in real time.
- **Note:** `fim` does not implement privilege separation or sandboxing. Run it with appropriate filesystem permissions and consider using a dedicated user or container if necessary.

---

## Testing

The `tests/` directory is currently empty and no automated test suite exists. Manual verification can be performed as follows:

1. Run `./fim` in one terminal.
2. In another terminal, create, edit, or delete files under `src/`.
3. Inspect `logs.jsonl` and `fim_baseline.json` to confirm expected alerts and baseline updates.

Future work may introduce a unit-test harness and CI configuration.

---

## License

[Specify your license here]

---

## Contributing

Contributions are welcome. Please ensure that changes conform to the existing code style:

- Headers use `#pragma once`.
- Interface classes are prefixed with `I` (e.g., `IHasher`).
- Member variables are prefixed with `m_` (e.g., `m_fd`).
- Prefer `std::string_view` for non-owning string parameters.
- Use `std::optional` for values that may be absent.

---

## Acknowledgments

- [BLAKE3](https://github.com/BLAKE3-team/BLAKE3) — for the high-performance cryptographic hash implementation.
- [nlohmann/json](https://github.com/nlohmann/json) — for the single-header JSON library.
