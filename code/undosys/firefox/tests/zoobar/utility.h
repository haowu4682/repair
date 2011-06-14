#pragma once

#include <sys/types.h>

void
die(const char *s);

ssize_t
sendfd(int socket, const void *buffer, size_t length, int fd);

ssize_t
recvfd(int socket, void *buffer, size_t length, int *fd);

void
url_decode(char *src, char *dst);

int
read_line(char *buf, int size, int fd);

void
fdprintf(int fd, char *fmt, ...);

void
http_err(int fd, int code, char *fmt, ...);

void
deserialize_env(const char *env, size_t len);

char *
parse_http_header(int fd);
