# Installing Melian

This is a guide for installing Melian.

## Building

### Dependencies

* GCC / Clang (C99 or newer)
* `libevent` ≥ 2.1
* `libmysqlclient`
* `libjansson` (for client JSON parsing)
* `xxhash`
* POSIX environment (Linux or macOS)

### Compile

```bash
# Build
make

# View valid make targets
make help

# Rebuild everything
make clean all
```

