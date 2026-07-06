#ifndef PROJECT_COMMON
#define PROJECT_COMMON
// character to store in p_queue
struct treelink {
	struct character *left;
	struct character *right;
};

struct character {
	char c; // idk if this is signed or unsigned, I just need it to store a raw byte
	unsigned int count;
	struct treelink link;
};

enum {MAX = 128};

constexpr unsigned int HM_LEN = pow(2, sizeof(char) * 8);
#endif
