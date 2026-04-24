#!/bin/sh
printf 'Content-Type: text/html; charset=utf-8\r\n'
printf '\r\n'
printf '<!DOCTYPE html><html><body>'
printf '<h1>Hello from CGI</h1>'
printf '<p>REMOTE_ADDR: %s</p>' "${REMOTE_ADDR}"
printf '<p>REQUEST_METHOD: %s</p>' "${REQUEST_METHOD}"
printf '<p>QUERY_STRING: %s</p>' "${QUERY_STRING}"
printf '</body></html>'
