#ifndef PROJECT_CANON_CODES
#define PROJECT_CANON_CODES

/*
NOTE: this file exists solely because both encoder and decoder require the
generation of canon codes
*/

#include "common.h"

static struct {
	uint64_t bits[HM_LEN / (sizeof(uint64_t) * 8)];
	uint8_t n_bits;
} encodings[HM_LEN];
enum { TYPE_LEN_BITS = sizeof *(*encodings).bits * 8 };

// generate canonical codes and fill in encodings[]
static inline void gen_canon_codes(
	struct character *current,
	struct character *prev
) {
	unsigned int c_in = current->c;
	encodings[c_in].n_bits = current->count;

	// copy previous bits over
	memcpy(
		encodings[c_in].bits,
		encodings[prev->c].bits,
		sizeof (*encodings).bits
	);

	// add 1
	for (int i = 0; !++encodings[c_in].bits[i++];);

	// shift bits
	unsigned int shift = encodings[c_in].n_bits - encodings[prev->c].n_bits;

	/*
	Okay, so basically if shift > 64, we have a problem. Shifting by > 64
	bits means we lose the everything in that array cell.

	So, we compute where we'll end up if we were to shift there, copy the
	bytes over. Only then do we shift by (shift % TYPE_LEN_BITS)
	*/
	{
		unsigned int current_byte = encodings[c_in].n_bits;
		unsigned int index = (encodings[c_in].n_bits + shift) / TYPE_LEN_BITS;
		for (int i = current_byte; i >= 0 && index > current_byte; index--, i--) {
			encodings[c_in].bits[index] = encodings[c_in].bits[i];
			encodings[c_in].bits[i] = 0;
		}
	}

	/*
	now we handle the remaining shift % 64 bits of shifting
	Note that if shift % 64 is 0, then TYPE_LEN_BITS - shift is UB (can't
	shift 64 bits) hence the if (shift)
	*/
	shift &= TYPE_LEN_BITS - 1;
	if (shift) for (int i = 1; i < HM_LEN / TYPE_LEN_BITS; i++) {
		encodings[c_in].bits[i] =
			(encodings[c_in].bits[i] << shift)
			|
			(encodings[c_in].bits[i - 1] >> (TYPE_LEN_BITS - shift));
	}
	encodings[c_in].bits[0] <<= shift;
}
#endif
