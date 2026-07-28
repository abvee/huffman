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

So, we only actually need to check encoding[].bits.a[0], but I've added a pointer
to the entire array, cuz why not, it's type checking and it doesn't take up any
more space.
*/
struct {
	u256 *start;
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
void deserialize(
	byte *buf,
	uint buf_len,
	struct character *characters,
	dynarray *output
);
static inline uint nearest_larger_pow2(uint in);

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
	memset(encodings[characters[0].c].bits.a, 0, sizeof (*encodings).bits.a);
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
	for (int i = 0; i < sizeof (*encodings).bits.a / sizeof *(*encodings).bits.a; i++)
		f_printf("0x%016lx ", encodings[characters[0].c].bits.a[i]);
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

			bit_range_index = characters[i].count - 1;
			bit_ranges[bit_range_index].start = &encodings[characters[i].c].bits;
			bit_ranges[bit_range_index].marker = i;
			f_printf("Bit marker for %u placed at %u\n", bit_range_index + 1, i);

			// new bit started, the len should be 0
			assert(bit_ranges[bit_range_index].r_index == 0);

		} else bit_ranges[bit_range_index].r_index++;
		// debug print
		if (isalnum(characters[i].c))
			f_printf("%c(%d): %d ->\t", characters[i].c, characters[i].c, characters[i].count);
		else
			f_printf("(%d): %d ->\t", characters[i].c, characters[i].count);
		for (int j = 0; j < sizeof (*encodings).bits.a / sizeof *(*encodings).bits.a; j++)
			f_printf("0x%016lx ", encodings[characters[i].c].bits.a[j]);
		f_printf("\n");
	}
	bit_lens.stk[bit_lens.top++] = bit_range_index + 1; // Don't forget to write the last bit length

	/*
	Unlike encoding, we don't know the length of the output, so we need to use a
	dynarray here
	*/
	dynarray output = {NULL, nearest_larger_pow2(buf_len), 0};
	output.ptr = malloc(sizeof *output.ptr * output.capacity);

	deserialize(buf + buf_i, buf_len - buf_i, characters, &output);

	fwrite(output.ptr, sizeof *output.ptr, output.len, stdout);
	free(characters);
	free(output.ptr);
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

// bit smearing
inline uint nearest_larger_pow2(uint in) {
	if (in <= 1) return 1;

	in--; // why ?

	in |= in >> 1;
	in |= in >> 2;
	in |= in >> 4;
	in |= in >> 8;
	in |= in >> 16;

	return in + 1;
}

// deserializer
static inline void read_in(
	byte *input,
	uint bit_i,
	u256 *buf,
	uint n_bits
);
static inline int offset_diff(const u256 *buf, uint n_bits);

void deserialize(
	byte *buf,
	uint buf_len,
	struct character *characters,
	dynarray *output
) {
	assert(bit_lens.top <= sizeof bit_lens.stk / sizeof *bit_lens.stk);
	uint buf_i = 0;
	uint bit_i = 0; // intra byte index
	// indexes next free, not current top

	u256 read_buf;

	for (; buf_i < buf_len;) {
		uint j = 0;
		for (; j < bit_lens.top; j++) {
			memset(read_buf.a, 0, sizeof read_buf.a);
			read_in(
				buf + buf_i,
				bit_i,
				&read_buf,
				bit_lens.stk[j]
			);

			// difference testing
			int d = offset_diff(&read_buf, bit_lens.stk[j]);
			if (d <= bit_ranges[bit_lens.stk[j] - 1].r_index) {

				byte c = characters[bit_ranges[bit_lens.stk[j] - 1].marker + d].c;
				// debug print
				f_printf("Read buffer for %u bits->\t", bit_lens.stk[j]);
				for (int i = 0; i < sizeof read_buf.a / sizeof *read_buf.a; i++)
					f_printf("0x%016lx ", read_buf.a[i]);
				f_printf("\n");
				f_printf("Character written: (%d)\n", c);

				output->ptr[output->len++] = c;
				if (output->len >= output->capacity)
					output->ptr = realloc(output->ptr, sizeof *output->ptr * (output->capacity *= 2));
				break;
			}
		}
		buf_i += (bit_i + bit_lens.stk[j]) / BYTE_BIT_LEN;
		bit_i = (bit_i + bit_lens.stk[j]) % BYTE_BIT_LEN;
		f_printf("New byte index: %u\nNew bit index: %u\n\n", buf_i, bit_i);
	}
}


/*
This is an O(2n) solution, there is probably an O(n) solution somewhere
*/
inline void read_in(
	byte *input,
	uint bit_i,
	u256 *buf,
	uint n_bits
) {
	assert(bit_i < BYTE_BIT_LEN);
	assert(n_bits <= HM_LEN);

	uint original_n_bits = n_bits;
	uint byte_index = (n_bits - 1) / BYTE_BIT_LEN;

	for (; n_bits >= BYTE_BIT_LEN; n_bits -= BYTE_BIT_LEN) {
		buf->bytes[byte_index--] =
			*input << bit_i | *(input + 1) >> (BYTE_BIT_LEN - bit_i);
		input++;
	}

	if (n_bits == 0) return;

	assert(n_bits == original_n_bits % BYTE_BIT_LEN);
	assert(byte_index == 0);

	if (n_bits < BYTE_BIT_LEN - bit_i)
		buf->bytes[0] = (byte) (*input << bit_i) >> (BYTE_BIT_LEN - n_bits);
	else
		buf->bytes[0] =
			(byte) ((*input << bit_i) | *(input + 1) >> (BYTE_BIT_LEN - bit_i))
			>> (BYTE_BIT_LEN - n_bits);


	// here's the O(2n) solution
	byte_index = (original_n_bits - 1) / BYTE_BIT_LEN;
	for (uint i = 0; i < byte_index; i++) {
		buf->bytes[i] |= buf->bytes[i + 1] << n_bits;
		buf->bytes[i + 1] >>= (BYTE_BIT_LEN - n_bits);
	}

	/*
	For an O(n) solution, you have to read some number of bits... down from the
	input instead of shifting them all at the end

	Ideally a u256 would be typedef u64 u256[4] instead of a union, but the
	shifting gets infinitely messier when trying to cross a 64 bit boundry.
	You would need to read into the correct bytes for a certain range before
	being able to write into the buffer directly without any computation.

	I guess TODO: change this entire thing to the u64 bit version
	*/
}

inline int offset_diff(const u256 *buf, uint n_bits) {
	assert(n_bits > 0 && n_bits <= HM_LEN);
	/*
	the difference CANNOT be more than 255
	if it is, then return 257, guaranteed to be more than any r_index
	*/
	uint current_word = (n_bits - 1) / U64_BIT_LEN;
	for (;
		current_word > 0
			&&
		bit_ranges[n_bits - 1].start->a[current_word] == buf->a[current_word];
		current_word--
	)
	if (current_word != 0) return HM_LEN + 1;
	return *buf->a - *(bit_ranges[n_bits - 1].start->a);
}
