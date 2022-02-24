#ifndef _GOBLIN_H
#define _GOBLIN_H

#include <stdint.h>
#include <stdlib.h>

#define GOBLIN_OK                         0x00

#define GOBLIN_E_INVALID_SIGNATURE        0x10
#define GOBLIN_E_INVALID_FORMAT           0x11
#define GOBLIN_E_CHECKSUM_FAIL            0x12

#define GOBLIN_E_UNSUPPORTED_DEPTH        0x20
#define GOBLIN_E_UNSUPPORTED_COLOR        0x21
#define GOBLIN_E_UNSUPPORTED_COMPRESSION  0x22
#define GOBLIN_E_UNSUPPORTED_FILTER       0x23
#define GOBLIN_E_UNSUPPORTED_INTERLACE    0x24

#define GOBLIN_E_FILE_SYSTEM              0x30
#define GOBLIN_E_MEMORY_ALLOCATION        0x31

#define GOBLIN_E_ZLIB_INFLATE             0x40

#ifdef __cplusplus
extern "C"
{
#endif

int goblin__img_stat(const char *path, size_t *bytes, uint32_t *width, uint32_t *height);
int goblin__img_load(const char *path, uint8_t *rgba);
const char *goblin__strerror(int);

#ifdef __cplusplus
}
#endif

#endif
