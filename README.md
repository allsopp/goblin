# goblin

[![builds.sr.ht status](https://builds.sr.ht/~allsopp/goblin.svg)](https://builds.sr.ht/~allsopp/goblin?)

Simple decoder library for indexed PNG images

```
int goblin__img_stat(const char *path, size_t *bytes, uint32_t *width, uint32_t *height);
int goblin__img_load(const char *path, uint8_t *rgba);
const char *goblin__strerror(int);
```

## Description

A simple PNG decoder library written in C99 that implements just enough of the
PNG specification to decode indexed PNG files and write the bitmap data into a
`uint8_t *` buffer of sufficient size.
