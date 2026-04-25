# systemd integration

Two units that let `systemd` manage HTTPServer with privilege-less socket
activation:

* `httpserver.socket` — owns the listening TCP socket on port 8080.
* `httpserver.service` — starts the binary and inherits the socket via
  `LISTEN_PID` / `LISTEN_FDS` (the protocol used by `sd_listen_fds(3)`).

## Install

```bash
sudo install -Dm755 build/httpserver           /usr/local/bin/httpserver
sudo install -Dm644 systemd/httpserver.service /etc/systemd/system/httpserver.service
sudo install -Dm644 systemd/httpserver.socket  /etc/systemd/system/httpserver.socket
sudo install -Dm644 httpserver.example.ini     /etc/httpserver/httpserver.ini
sudo mkdir -p /var/log/httpserver

sudo systemctl daemon-reload
sudo systemctl enable --now httpserver.socket
```

The `httpserver.service` unit is started on demand when the first connection
arrives on the socket, or via `systemctl start httpserver`.

## Verifying socket activation

When the service receives the inherited socket, the startup banner prints
`systemd-inherited` instead of `self-bound`:

```
Server listening on port 8080 (plain, systemd-inherited socket), ...
```
