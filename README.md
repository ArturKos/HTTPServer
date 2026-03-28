# HTTPServer

A multi-process **HTTP/1.1 web server** written in C that uses `fork()` for concurrent client handling and serves static files with proper MIME type detection and request logging.

![C](https://img.shields.io/badge/C-99-A8B9CC?logo=c)
![Linux](https://img.shields.io/badge/Linux-BSD_Sockets-FCC624?logo=linux)
![HTTP](https://img.shields.io/badge/HTTP-1.1-005AF0)

## Features

- **BSD socket API** with full `socket` / `bind` / `listen` / `accept` lifecycle
- **Fork-based concurrency** -- each incoming connection is handled by a dedicated child process
- **MIME type detection** for HTML, CSS, PNG, GIF, and JPG based on file extension
- **Automatic index fallback** -- requests to `/` are served as `index.html`
- **Request logging** to `myhttpd.log` with timestamps, client IP, process ID, requested URL, and response status
- **404 error handling** with a human-readable error response when files are not found
- **Content-Length header** computed from actual file size for correct HTTP responses

## Dependencies

| Component | Purpose |
|-----------|---------|
| GCC | C compiler |
| POSIX / Linux | `fork()`, BSD sockets, `netinet/in.h` |
| libc | Standard C library (stdio, stdlib, string, time) |

## Building

```bash
gcc httpd.c -o myhttpd
```

## Usage

```bash
./myhttpd <port> <document_root>
```

| Argument | Description |
|----------|-------------|
| `port` | TCP port number the server will listen on |
| `document_root` | Path to the directory containing static files to serve |

### Example

```bash
./myhttpd 8080 ./www
```

The server will listen on port 8080 and serve files from the `./www` directory. Navigating to `http://localhost:8080/` will serve `./www/index.html`.

## Architecture

```
Client Request
      |
      v
  [accept()]  -- main process listens for connections
      |
      v
   [fork()]   -- child process spawned per connection
      |
      v
  [recv()]    -- read HTTP request, extract URL path
      |
      v
  [fopen()]   -- open requested file from document root
      |
      +---> File found:     send "200 OK" + Content-Type + Content-Length + body
      +---> File not found: send "404" + error message
      |
      v
  [push_log]  -- append request details to myhttpd.log
      |
      v
  [close()]   -- terminate child process
```

## Request Log Format

Each request is logged to `myhttpd.log` with the following format:

```
[DATA: DD.MM.YYYY] [GODZINA: HH:MM:SS] IP_HOSTA:[x.x.x.x] PID_PROCESU: [pid] URL_ZADANY: [path] STATUS_ODPOWIEDZI: [OK|NIE ZNALEZIONO PLIKU]
```

## Supported MIME Types

| Extension | Content-Type |
|-----------|-------------|
| `.html` | `text/html; charset=utf-8` |
| `.css` | `text/css; charset=utf-8` |
| `.png` | `image/png` |
| `.jpg` | `image/jpg` |
| `.gif` | `image/gif` |

## Project Structure

```
HTTPServer/
├── httpd.c         # Main server: socket init, fork loop, request handling, MIME routing
├── httpd.h         # Helper functions: socket init, URL/extension parsing, logging, POP3 auth
└── README.md
```

## License

This project is provided as-is for educational purposes.
