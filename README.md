# libsam3

[![Build Status](https://travis-ci.org/i2p/libsam3.svg?branch=master)](https://travis-ci.org/i2p/libsam3)

A C library for the [SAM v3 API](https://geti2p.net/en/docs/api/samv3).

## Development Status

Unmaintained, but PRs welcome!

## Usage

Copy the two files from one of the following locations into your codebase:

- `src/libsam3` - Synchronous implementation.
- `src/libsam3a` - Asynchronous implementation.

See `examples/` for how to use various parts of the API.

## Cross-Compiling for Windows from debian:

Set your cross-compiler up:

``` sh
export CC=x86_64-w64-mingw32-gcc
export CFLAGS='-Wall -O2 '
export LDFLAGS='-lmingw32 -lws2_32 -lwsock32 -mwindows'
```

run `make build`
