# HTTPServer

A multi-process **HTTP/1.1 web server** written in C with a clean layered
architecture, CGI/1.1 and PHP-CGI support, unit tests based on
[GoogleTest](https://github.com/google/googletest), a CMake build system
and [Doxygen](https://www.doxygen.nl/) documentation.

![C](https://img.shields.io/badge/C-11-A8B9CC?logo=c)
![C++ (tests)](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus)
![CMake](https://img.shields.io/badge/CMake-3.14%2B-064F8C?logo=cmake)
![GoogleTest](https://img.shields.io/badge/GoogleTest-1.14-4285F4)
![Linux](https://img.shields.io/badge/Linux-BSD_Sockets-FCC624?logo=linux)

## Features

- **HTTP/1.1** request parser (GET, HEAD, POST) with URL decoding
- **Path-traversal protection** (`../` rejected)
- **Static file serving** with extensible MIME table
- **CGI/1.1** execution for `/cgi-bin/*` scripts
- **PHP** execution via the system `php-cgi` binary
- **Multi-process concurrency** via `fork()`, with `SIGCHLD`-based reaping
- **Graceful shutdown** on `SIGINT` / `SIGTERM`
- **Access log** with timestamps, PID, client IP, status and path
- **Google Test** unit suite covering every pure module
- **Doxygen** API documentation for every public function

## Project layout

```
HTTPServer/
├── CMakeLists.txt
├── Doxyfile.in
├── include/http_server/     Public API headers (Doxygen-commented)
│   ├── server.h
│   ├── socket_utils.h
│   ├── request.h
│   ├── response.h
│   ├── status_codes.h
│   ├── mime.h
│   ├── path_utils.h
│   ├── logger.h
│   ├── file_handler.h
│   ├── cgi_handler.h
│   └── php_handler.h
├── src/                     Implementation (.c files)
├── tests/                   Google Test suites
├── www/                     Sample document root
│   ├── index.html, style.css
│   ├── info.php
│   └── cgi-bin/hello.cgi
├── scripts/build.sh         One-shot build+test script
└── docs/                    Generated documentation (after make docs)
```

## Building

### Requirements

| Component | Version |
|-----------|---------|
| CMake | >= 3.14 |
| GCC or Clang | C11 + C++17 |
| POSIX / Linux | fork, BSD sockets |
| Doxygen (optional) | >= 1.9 |
| php-cgi (optional) | runtime, for `.php` support |

### One-shot build + tests

```bash
./scripts/build.sh
```

### Manual build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

### Building Doxygen documentation

```bash
cmake -S . -B build -DBUILD_DOCUMENTATION=ON
cmake --build build --target docs
# Open build/docs/html/index.html
```

## Running

```bash
./build/httpserver <port> <document_root> [log_file]
```

Example:

```bash
./build/httpserver 8080 ./www
curl http://localhost:8080/
curl http://localhost:8080/cgi-bin/hello.cgi
```

## Architecture

```
  main -> ServerConfig -> server_run
                              │
                              ├── listening socket (SO_REUSEADDR)
                              │
                              ▼
                          accept loop ── SIGCHLD reaper
                              │
                              └── fork() per connection
                                    │
                                    ▼
                              handle_client_connection
                                    │
                                    ├── http_request_parse
                                    ├── is_request_path_safe
                                    ├── resolve_document_path
                                    │
                                    ├── cgi_handler_is_cgi_request ── cgi_handler_execute
                                    ├── php_handler_is_php_request ── php_handler_execute
                                    └── file_handler_serve
                                              │
                                              └── mime_get_content_type + response_write_*
```

## Clean-code principles applied

- **Single Responsibility** per translation unit: parsing, responding, logging,
  MIME detection, CGI execution, PHP execution all live in separate modules.
- **No single-letter identifiers** — names describe intent.
- **No global mutable state** except a single append-only log path that is set
  once at startup.
- **Bounds-checked I/O**: `snprintf`, `memcpy`, `strncmp` &mdash; `strcat` /
  `sprintf` removed.
- **Path traversal rejected** before any filesystem access.
- **Zombies reaped** via a `SIGCHLD` handler (original code leaked one zombie
  per request).
- **Signal-safe shutdown** through a `sig_atomic_t` flag.
- **Doxygen-documented** public API.
- **Header-only definitions removed** — headers are declarations only.

## License

MIT-compatible, provided as-is for educational purposes.
