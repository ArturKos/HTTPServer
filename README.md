# SerwerHTTP

Multi-process HTTP/1.1 web server written in C. Uses `fork()` for concurrent client handling and serves static files with proper MIME type detection.

## Features

- BSD socket API with bind/listen/accept lifecycle
- Fork-based concurrency for parallel request handling
- MIME type support: HTML, CSS, PNG, GIF, JPG
- Request logging to file
- Automatic `index.html` fallback for directory requests

## Build & Run

```bash
gcc httpd.c -o myhttpd
./myhttpd 81 ./www
```

Built and tested on Ubuntu Linux.
