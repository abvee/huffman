#ifndef PROJECT_COMMON
#define PROJECT_COMMON

#include <stdint.h>
typedef uint8_t byte;

// character to store in p_queue
struct treelink {
	struct character *left;
	struct character *right;
};

struct character {
	byte c;
	unsigned int count;
	struct treelink link;
};

enum {MAX = 128};

constexpr unsigned int HM_LEN = pow(2, sizeof(char) * 8);

#endif
