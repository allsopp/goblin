#include "goblin.h"

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

static struct header {
	uint8_t id_length;
	uint8_t color_map;
	uint8_t image_type;
	uint8_t color_spec[5];
	uint8_t image_spec[10];
} header;

int
main(int argc, char **argv)
{
	unsigned char *data;
	size_t bytes;
	uint32_t width, height, i;
	int rs;

	if (argc < 2) {
		fprintf(stderr, "usage: %s <file>\n", argv[0]);
		return EXIT_FAILURE;
	}
	rs = goblin__img_stat(argv[1], &bytes, &width, &height);
	if (rs == GOBLIN_E_FILE_SYSTEM) {
		fprintf(stderr, "ERROR: '%s' not found\n", argv[1]);
		return EXIT_FAILURE;
	}
	assert(!rs);
	fprintf(stderr, "# [%s] %lu bytes, %ux%u pixels\n",
		__FILE__, (unsigned long)bytes, width, height);
	data = malloc(bytes);
	assert(data);
	rs = goblin__img_load(argv[1], data);
	assert(!rs);
	header.image_type = 0x02; /* uncompressed true-colour */
	header.image_spec[4] = width & 0xFF;
	header.image_spec[5] = (width >> 8) & 0xFF;
	header.image_spec[6] = height & 0xFF;
	header.image_spec[7] = (height >> 8) & 0xFF;
	header.image_spec[8] = 24; /* 24 bits per pixel */
	fwrite(&header.id_length, sizeof header.id_length, 1, stdout);
	fwrite(&header.color_map, sizeof header.color_map, 1, stdout);
	fwrite(&header.image_type, sizeof header.image_type, 1, stdout);
	fwrite(header.color_spec, sizeof header.color_spec, 1, stdout);
	fwrite(header.image_spec, sizeof header.image_spec, 1, stdout);
	data += 4 * width * (height - 1);
	while (height--) {
		for (i = 0; i < width; ++i) {
			putchar(data[2]);
			putchar(data[1]);
			putchar(data[0]);
			data += 4;
		}
		data -= 8 * width;
	}
	return EXIT_SUCCESS;
}
