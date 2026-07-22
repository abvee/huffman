#include <ctype.h>
#include "common.h"
#include "p-queue.c"

void decode(byte *buf, uint buf_len) {
	uint char_count = buf[0] + 1; // 0 -> 255
	f_printf("Character count: %u\n", char_count);

	struct character *characters = malloc(sizeof *characters * char_count);
	uint characters_i = 0;

	// count characters and put them in a priority queue
	uint buf_i = 1;
	for (; buf_i < 2 * char_count + 1; buf_i += 2) {
		// debug print
		if (isalnum(buf[buf_i]))
			f_printf("%c(%d):%d\n", buf[buf_i], buf[buf_i], buf[buf_i + 1]);
		else
			f_printf("(%d):%d\n", buf[buf_i], buf[buf_i + 1]);

		characters[characters_i++] = (struct character) {
			.c = buf[buf_i],
			.count = buf[buf_i + 1],
			{NULL, NULL}
		};
		pq_enqueue(characters + characters_i - 1);
	}
	pq_print();

	free(characters);
}
