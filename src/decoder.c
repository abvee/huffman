#include <ctype.h>
#include "common.h"
#include "p-queue.c"

static inline uint read_characters(
	struct character *characters,
	byte *buf,
	uint char_count
);

void decode(byte *buf, uint buf_len) {
	uint char_count = buf[0] + 1; // 0 -> 255
	f_printf("Character count: %u\n", char_count);
	uint buf_i = 1;

	struct character *characters = malloc(sizeof *characters * char_count);
	uint max_bit_len = read_characters(characters, buf + buf_i, char_count);
	buf_i += 2 * char_count;

	free(characters);
}

static inline uint read_characters(
	struct character *characters,
	byte *buf,
	uint char_count
) {
	/*
	Read the characters and enter them to the priority queue
	Return max bit length found
	*/
	uint max_bit_len = 0;
	uint buf_i = 0;
	// count characters and put them in a priority queue
	for (
		uint characters_i = 0;
		buf_i < 2 * char_count;
		buf_i += 2, characters_i++
	) {

		// debug print
		if (isalnum(buf[buf_i]))
			f_printf("%c(%d):%d\n", buf[buf_i], buf[buf_i], buf[buf_i + 1]);
		else
			f_printf("(%d):%d\n", buf[buf_i], buf[buf_i + 1]);

		characters[characters_i] = (struct character) {
			.c = buf[buf_i],
			.count = buf[buf_i + 1],
			{NULL, NULL}
		};

		if (max_bit_len < characters[characters_i].count)
			max_bit_len = characters[characters_i].count;
		pq_enqueue(characters + characters_i);
	}
	pq_print();
	return max_bit_len;
}
