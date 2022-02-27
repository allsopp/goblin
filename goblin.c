#include "goblin.h"

#include <zlib.h>

#include <limits.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

/*
 * Based on Portable Network Graphics (PNG) Specification
 *  https://www.w3.org/TR/2003/REC-PNG-20031110/
 */

#define DEFLATE_BUFFER 65536

const uint8_t magic[] = { 0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A };

struct ihdr {
	uint32_t width;
	uint32_t height;
	uint8_t depth;
	uint8_t color;
	uint8_t compression;
	uint8_t filter;
	uint8_t interlace;
};

struct plte {
	uint32_t colors[256];
};

struct idat {
	size_t bytes;
	uint8_t *data;
	size_t zbytes;
	uint8_t *zdata;
	int count;
};

enum chunk {
	SKIP,
	IHDR,
	IDAT,
	PLTE,
	IEND
};

static int
read_value(void *buf, size_t bytes, FILE *fp)
{
	assert(buf);
	assert(fp);

	do { /* assume little endian arch (big endian on disk) */
		fread((uint8_t*)buf + --bytes, 1, 1, fp);
	} while (bytes);

	return bytes != 0;
}

static int
read_array(void *buf, size_t bytes, FILE *fp)
{
	size_t total;
	assert(buf);
	assert(fp);

	total = fread(buf, 1, bytes, fp);
	return total != bytes;
}

static int
parse_magic(FILE *fp)
{
	char buf[sizeof magic];
	assert(fp);

	if (read_array(buf, sizeof buf, fp))
		return GOBLIN_E_INVALID_FORMAT;
	if (memcmp(magic, buf, sizeof buf))
		return GOBLIN_E_INVALID_FORMAT;

	return GOBLIN_OK;
}

static int
parse_chunk(enum chunk *type, uint32_t *bytes, FILE *fp)
{
	char s[4];
	uint32_t b;
	assert(type);
	assert(fp);

	if (read_value(&b, sizeof b, fp))
		return GOBLIN_E_INVALID_FORMAT;
	if (read_array(s, sizeof s, fp))
		return GOBLIN_E_INVALID_FORMAT;

	if (!memcmp(s, "IHDR", sizeof s))
		*type = IHDR;
	else if (!memcmp(s, "IDAT", sizeof s))
		*type = IDAT;
	else if (!memcmp(s, "PLTE", sizeof s))
		*type = PLTE;
	else if (!memcmp(s, "IEND", sizeof s))
		*type = IEND;
	else
		*type = SKIP; /* chunk will be skipped */

	if (bytes)
		*bytes = b;

	return GOBLIN_OK;
}

static int
parse_plte(struct plte *plte, size_t bytes, FILE *fp)
{
	size_t i;
	assert(plte);
	assert(fp);

	if (bytes % 3)
		return GOBLIN_E_INVALID_FORMAT;

	for (i = 0; i < bytes / 3; ++i) {
		uint32_t color = 0;
		if (read_array(&color, 3, fp))
			return GOBLIN_E_FILE_SYSTEM;

		/* by convention index 0 has alpha 0x00,
		   the remainder have alpha 0xFF */
		if (i != 0) /* FIXME read from tRNS chunk */
			color |= 0xFF000000;

		plte->colors[i] = color;
	}

	fseek(fp, 4, SEEK_CUR); /* skip CRC */

	return GOBLIN_OK;
}

static int
parse_idat(struct idat *idat, size_t bytes, FILE *fp)
{
	assert(idat);
	assert(fp);

	if (!idat->zdata)
		idat->zdata = malloc(bytes);
	else
		idat->zdata = realloc(idat->zdata, idat->zbytes + bytes);

	if (!idat->zdata)
		return GOBLIN_E_MEMORY_ALLOCATION;
	if (fread(idat->zdata + idat->zbytes, 1, bytes, fp) != bytes)
		return GOBLIN_E_FILE_SYSTEM;

	fseek(fp, 4, SEEK_CUR); /* skip CRC */

	idat->count += 1;
	idat->zbytes += bytes;

	return GOBLIN_OK;
}

static int
decompress(struct idat *idat)
{
	z_stream z;
	assert(idat);
	assert(idat->zdata);

	if (!idat->count)
		return GOBLIN_E_INVALID_FORMAT;

	z.next_in = idat->zdata;
	z.avail_in = 0;
	z.zalloc = Z_NULL; /* use default allocation functions */
	z.zfree = Z_NULL;  /* use default allocation functions */
	z.opaque = NULL;   /* unused */

	if (inflateInit(&z) != Z_OK)
		return GOBLIN_E_ZLIB_INFLATE;

	if (!(idat->data = malloc(DEFLATE_BUFFER)))
		return GOBLIN_E_MEMORY_ALLOCATION;

	while (1) {
		int rs;
		size_t remaining = idat->zbytes - z.total_in;

		if (!z.avail_in)
			z.avail_in = (unsigned)MIN(remaining, UINT_MAX);
		z.next_out = &idat->data[z.total_out];
		z.avail_out = DEFLATE_BUFFER;

		rs = inflate(&z, Z_SYNC_FLUSH);
		if (rs < 0)
			return GOBLIN_E_ZLIB_INFLATE;
		if (rs == Z_STREAM_END)
			break;
		if (!(idat->data = realloc(idat->data, z.total_out + DEFLATE_BUFFER)))
			return GOBLIN_E_MEMORY_ALLOCATION;
	}

	idat->bytes = z.total_out; /* actual size of decompressed data */

	if (inflateEnd(&z) != Z_OK)
		return GOBLIN_E_ZLIB_INFLATE;

	return GOBLIN_OK;
}

static int
parse_ihdr(uint32_t *width, uint32_t *height, FILE *fp)
{
	struct ihdr ihdr;
	assert(width);
	assert(height);
	assert(fp);

	if (read_value(&ihdr.width, sizeof ihdr.width, fp))
		return GOBLIN_E_FILE_SYSTEM;
	if (read_value(&ihdr.height, sizeof ihdr.height, fp))
		return GOBLIN_E_FILE_SYSTEM;
	if (read_value(&ihdr.depth, sizeof ihdr.depth, fp))
		return GOBLIN_E_FILE_SYSTEM;
	if (read_value(&ihdr.color, sizeof ihdr.color, fp))
		return GOBLIN_E_FILE_SYSTEM;
	if (read_value(&ihdr.compression, sizeof ihdr.compression, fp))
		return GOBLIN_E_FILE_SYSTEM;
	if (read_value(&ihdr.filter, sizeof ihdr.filter, fp))
		return GOBLIN_E_FILE_SYSTEM;
	if (read_value(&ihdr.interlace, sizeof ihdr.interlace, fp))
		return GOBLIN_E_FILE_SYSTEM;

	if (ihdr.depth != 8)
		return GOBLIN_E_UNSUPPORTED_DEPTH;
	if (ihdr.color != 3) /* indexed color */
		return GOBLIN_E_UNSUPPORTED_COLOR;
	if (ihdr.compression != 0)
		return GOBLIN_E_UNSUPPORTED_COMPRESSION;
	if (ihdr.filter != 0)
		return GOBLIN_E_UNSUPPORTED_FILTER;
	if (ihdr.interlace != 0)
		return GOBLIN_E_UNSUPPORTED_INTERLACE;

	fseek(fp, 4, SEEK_CUR); /* skip CRC */

	*width = ihdr.width;
	*height = ihdr.height;

	return GOBLIN_OK;
}

static void
reconstruct(uint8_t *pixels,
	const uint8_t *data,
	struct plte *plte,
	uint32_t width,
	uint32_t height)
{
	uint32_t i, j;
	size_t offset = 0;
	assert(width);
	assert(height);
	assert(pixels);
	assert(data);
	assert(plte);

	for (i = 0; i < height; ++i) {
		for (j = 0; j < width; ++j) {
			uint8_t index;
			uint32_t color;

			index = data[i*(width+1) + j + 1]; /* scanline filter _ignored_ */
			color = plte->colors[index];

			memcpy(&pixels[offset], &color, sizeof color);
			offset += sizeof color;
		}
	}
}

int
goblin__img_stat(const char *path,
	size_t *bytes,
	uint32_t *width,
	uint32_t *height)
{
	int rs = GOBLIN_OK;
	enum chunk type;
	FILE *fp;
	assert(path);
	assert(bytes);
	assert(width);
	assert(height);

	if (!(fp = fopen(path, "rb")))
		return GOBLIN_E_FILE_SYSTEM;
	else if (parse_magic(fp))
		rs = GOBLIN_E_INVALID_SIGNATURE;
	else if (parse_chunk(&type, NULL, fp))
		rs = GOBLIN_E_INVALID_FORMAT;
	else if (type != IHDR)
		rs = GOBLIN_E_INVALID_FORMAT;

	if (rs || (rs = parse_ihdr(width, height, fp)))
		fclose(fp);
	else if (fclose(fp))
		rs = GOBLIN_E_FILE_SYSTEM;
	else
		*bytes = (*width) * (*height) * 4;

	return rs;
}

int
goblin__img_load(const char *path, uint8_t *rgba)
{
	int rs = GOBLIN_OK;
	uint32_t width = 0, height = 0;
	struct plte plte;
	struct idat idat;
	enum chunk type = SKIP;
	FILE *fp;
	assert(path);
	assert(rgba);

	idat.count = 0;
	idat.bytes = 0;
	idat.zbytes = 0;
	idat.data = NULL; /* always freeable */
	idat.zdata = NULL; /* always freeable */

	if (!(fp = fopen(path, "rb")))
		return GOBLIN_E_FILE_SYSTEM;
	else if (parse_magic(fp))
		rs = GOBLIN_E_INVALID_SIGNATURE;
	else if (parse_chunk(&type, NULL, fp))
		rs = GOBLIN_E_INVALID_FORMAT;
	else if (type != IHDR)
		rs = GOBLIN_E_INVALID_FORMAT;
	else
		rs = parse_ihdr(&width, &height, fp);

	while (!rs && type != IEND) {
		uint32_t bytes;

		if ((rs = parse_chunk(&type, &bytes, fp)))
			break;

		switch (type) {
		case IDAT:
			rs = parse_idat(&idat, bytes, fp);
			break;
		case PLTE:
			rs = parse_plte(&plte, bytes, fp);
			break;
		case IEND:
			rs = decompress(&idat);
			break;
		default:
			fseek(fp, bytes, SEEK_CUR);
			fseek(fp, 4, SEEK_CUR); /* skip CRC */
		}
	}

	if (rs)
		fclose(fp);
	else if (fclose(fp))
		rs = GOBLIN_E_FILE_SYSTEM;
	else
		reconstruct(rgba, idat.data, &plte, width, height);

	free(idat.zdata);
	free(idat.data);

	return rs;
}

const char *
goblin__strerror(int c)
{
	switch (c) {
	default:
	case GOBLIN_OK:
		return "";
	case GOBLIN_E_INVALID_SIGNATURE:
		return "GOBLIN_E_INVALID_SIGNATURE";
	case GOBLIN_E_INVALID_FORMAT:
		return "GOBLIN_E_INVALID_FORMAT";
	case GOBLIN_E_CHECKSUM_FAIL:
		return "GOBLIN_E_CHECKSUM_FAIL";
	case GOBLIN_E_UNSUPPORTED_DEPTH:
		return "GOBLIN_E_UNSUPPORTED_DEPTH";
	case GOBLIN_E_UNSUPPORTED_COLOR:
		return "GOBLIN_E_UNSUPPORTED_COLOR";
	case GOBLIN_E_UNSUPPORTED_COMPRESSION:
		return "GOBLIN_E_UNSUPPORTED_COMPRESSION";
	case GOBLIN_E_UNSUPPORTED_FILTER:
		return "GOBLIN_E_UNSUPPORTED_FILTER";
	case GOBLIN_E_UNSUPPORTED_INTERLACE:
		return "GOBLIN_E_UNSUPPORTED_INTERLACE";
	case GOBLIN_E_FILE_SYSTEM:
		return "GOBLIN_E_FILE_SYSTEM";
	case GOBLIN_E_MEMORY_ALLOCATION:
		return "GOBLIN_E_MEMORY_ALLOCATION";
	case GOBLIN_E_ZLIB_INFLATE:
		return "GOBLIN_E_ZLIB_INFLATE";
	}
}
