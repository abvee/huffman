#include <ctype.h>
#include <assert.h>
#include "common.h"
#include "p-queue.c"
#include "canon-codes.c"

/*
This is the madness I have come up with for this decoder:

First, we read the min number of bits (say 2)

To find if those 2 bits actually encode something, we check if it's in a
particular range, stored in bit_ranges
This works because all the codes are prefix codes and thanks to the canonical
algorithm, HAS to be generated sequentially

So if the 2 bits we read are infact encoding something, they WILL lie in the
range >= start && <= start + len

We don't even need to account for overflow, because (n) elements having the
same bit can't overflow if they're filled sequentially

So, we only actually need to check encoding[].bits[0], but I've added a pointer
to the entire array, cuz why not, it's type checking and it doesn't take up any
more space.
*/
struct {
	uint64_t (*start)[HM_LEN / TYPE_LEN_BITS];
	uint r_index; // how many more excluding .start are present in this bit range
} bit_ranges[HM_LEN];

static inline uint read_characters(
	struct character *characters,
	byte *buf,
	uint char_count
);

void decode(byte *buf, uint buf_len) {
	// Init bit_ranges
	for (uint i = 0; i < sizeof bit_ranges / sizeof *bit_ranges; i++) {
		bit_ranges[i].start = NULL;
		bit_ranges[i].r_index = 0;
	}

	// Start reading input
	uint char_count = buf[0] + 1; // 0 -> 255
	f_printf("Character count: %u\n", char_count);
	uint buf_i = 1;

	struct character *characters = malloc(sizeof *characters * char_count);
	uint max_bit_len = read_characters(characters, buf + buf_i, char_count);
	buf_i += 2 * char_count;


	/*
	Generate canon codes
	*/
	struct character *prev = pq_dequeue();
	// 0 initial encoding
	encodings[prev->c].n_bits = prev->count;
	memset(encodings[prev->c].bits, 0, sizeof (*encodings).bits);

	// bit ranges
	uint bit_range_index = prev->count - 1; // current index of bit_ranges
	bit_ranges[bit_range_index].start = &encodings[prev->c].bits;

	for (struct character *current; current = pq_dequeue(); prev = current) {
		gen_canon_codes(current, prev);

		if (bit_range_index != current->count - 1) {
			f_printf("Bit len changed from %u to %u\n", bit_range_index + 1, current->count);

			bit_range_index = current->count - 1;
			bit_ranges[bit_range_index].start = &encodings[current->c].bits;

			// new bit started, the len should be 0
			assert(bit_ranges[bit_range_index].r_index == 0);
		} else bit_ranges[bit_range_index].r_index++;

		// debug print
		if (isalnum(current->c))
			f_printf("%c(%d): %d ->\t", current->c, current->c, encodings[current->c].n_bits);
		else
			f_printf("(%d): %d ->\t", current->c, encodings[current->c].n_bits);
		for (int i = 0; i < sizeof (*encodings).bits / sizeof *(*encodings).bits; i++)
			f_printf("0x%016lx ", encodings[current->c].bits[i]);
		f_printf("\n");
	}

	// start reading
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
