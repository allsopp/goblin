# goblin

goblin is a simple PNG decoder written in C99 that implements just enough of
the PNG specification to decode indexed PNG files and write the bitmap data
into a `uint8_t *` buffer of sufficient size.

## Dependencies

* [zlib](https://zlib.net)

## Interface

```
int goblin__img_stat(const char *path, size_t *bytes, uint32_t *width, uint32_t *height);
int goblin__img_load(const char *path, uint8_t *rgba);
const char *goblin__strerror(int);
```

## Usage

First call `goblin__img_stat` to determine the width and height (in pixels) and the
number of bytes required for the bitmap data.

Secondly call `goblin__img_load`, passing a pointer to a pre-allocated buffer
that will be populated with the bitmap data.

Error codes can be converted to `const char *` using `goblin__strerror`.
