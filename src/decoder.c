#include <ctype.h>
#include <assert.h>
#include "common.h"
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
	uint64_t (*start)[HM_LEN / U64_BIT_LEN];
	uint r_index; // how many more excluding .start are present in this bit range
	uint marker;
} bit_ranges[HM_LEN];
struct {
	uint stk[HM_LEN];
	uint top;
} bit_lens;

static inline void read_characters(
	struct character *characters,
	byte *buf,
	uint char_count
);
void deserialize(byte *buf, uint buf_len);

void decode(byte *buf, uint buf_len) {
	// init bit_ranges and decode_hash_map
	for (uint i = 0; i < sizeof bit_ranges / sizeof *bit_ranges; i++) {
		bit_ranges[i].start = NULL;
		bit_ranges[i].r_index = 0;
		bit_ranges[i].marker = 0;
	}
	memset(bit_lens.stk, 0, sizeof bit_lens.stk);
	bit_lens.top = 0;

	// Start reading input
	uint char_count = buf[0] + 1; // 0 -> 255
	f_printf("Character count: %u\n", char_count);
	uint buf_i = 1;

	struct character *characters = malloc(sizeof *characters * char_count);
	read_characters(characters, buf + buf_i, char_count);
	// characters is now a sorted array
	buf_i += 2 * char_count;

	/*
	Generate canon codes
	*/
	memset(encodings[characters[0].c].bits, 0, sizeof (*encodings).bits);
	encodings[characters[0].c].n_bits = characters[0].count;

	// bit ranges
	uint bit_range_index = characters[0].count - 1; // current index of bit_ranges
	bit_ranges[bit_range_index].start = &encodings[characters[0].c].bits;
	bit_ranges[bit_range_index].marker = 0;

	// debug print
	if (isalnum(characters[0].c))
		f_printf("%c(%d): %d ->\t", characters[0].c, characters[0].c, encodings[characters[0].c].n_bits);
	else
		f_printf("(%d): %d ->\t", characters[0].c, encodings[characters[0].c].n_bits);
	for (int i = 0; i < sizeof (*encodings).bits / sizeof *(*encodings).bits; i++)
		f_printf("0x%016lx ", encodings[characters[0].c].bits[i]);
	f_printf("\n");

	for (uint i = 1; i < char_count; i++) {
		gen_canon_codes(characters + i, characters + i - 1);
		// TODO: Redundancy, encodings[i].n_bits is the same as character.count
		// Remove them by modifying canon-codes.c

		if (bit_range_index != characters[i].count - 1) {

			bit_lens.stk[bit_lens.top++] = bit_range_index + 1;
			// store which bit lengths exist in ascending order
			// This stack CANNOT overflow, there can only ever be 256 bits

			f_printf("Bit len changed from %u to %u\n", bit_range_index + 1, characters[i].count);
			f_printf(
				"Number of %u bit characters: %u\n",
				bit_range_index + 1,
				bit_ranges[bit_range_index].r_index + 1
			);
			bit_ranges[bit_range_index].marker = i;
			f_printf("Bit marker for %u placed at %u\n", bit_range_index + 1, i);

			// bit_ranges stuff
			bit_range_index = characters[i].count - 1;
			bit_ranges[bit_range_index].start = &encodings[characters[i].c].bits;

			// new bit started, the len should be 0
			assert(bit_ranges[bit_range_index].r_index == 0);

		} else bit_ranges[bit_range_index].r_index++;

		// debug print
		if (isalnum(characters[i].c))
			f_printf("%c(%d): %d ->\t", characters[i].c, characters[i].c, characters[i].count);
		else
			f_printf("(%d): %d ->\t", characters[i].c, characters[i].count);
		for (int j = 0; j < sizeof (*encodings).bits / sizeof *(*encodings).bits; j++)
			f_printf("0x%016lx ", encodings[characters[i].c].bits[j]);
		f_printf("\n");
	}

	deserialize(buf + buf_i, buf_len);
	free(characters);
}

static inline void read_characters(
	struct character *characters,
	byte *buf,
	uint char_count
) {
	/*
	Read the characters and enter them to buffer in a sorted order (insertion
	sort). Return max bit length found
	*/
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

		const uint count = buf[buf_i + 1];

		uint i = characters_i;
		for (; i > 0 && count < characters[i - 1].count; i--)
			characters[i] = characters[i - 1];
		f_printf("Placing at position: %u\n", i);

		characters[i] = (struct character) {
			.c = buf[buf_i],
			.count = buf[buf_i + 1],
			{NULL, NULL}
		};
	}

	// debug print characters array
	for (int i = 0; i < char_count; i++)
		if (isalnum(characters[i].c))
			f_printf(
				"%c(%d):%d ",
				characters[i].c,
				characters[i].c,
				characters[i].count
			);
		else
			f_printf("(%d):%d ", characters[i].c, characters[i].count);
	f_printf("\n");
}

// deserializer
static inline void read_in(
	byte *input,
	uint bit_i,
	u256 *buf,
	uint n_bits
);

void deserialize(byte *buf, uint buf_len) {
	assert(bit_lens.top <= sizeof bit_lens.stk / sizeof *bit_lens.stk);

	uint buf_i = 0;
	uint bit_i = 0; // intra byte index
	// indexes next free, not current top

	u256 read_buf;
	memset(read_buf.a, 0, sizeof read_buf.a);

	for (;buf_i < buf_len;) {
		for (uint j = 0; j < bit_lens.top; j++) {
			read_in(
				buf + buf_i,
				bit_i,
				&read_buf,
				bit_lens.stk[j]
			);
		}
	}
}

// read n_bits into buffer

/*
The input encoded data HAS to be byte aligned or this crashes. The last byte
NEEDS to have padding in it
*/
inline void read_in(
	byte *input,
	uint bit_i,
	u256 *buf,
	uint n_bits
) { }
