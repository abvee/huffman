#include "common.h"
#include <stdio.h>

/*
I wanted to make this a dynamic array, but if we're hardcoding the hash map as
a direct map, might as well do the same for the priority queue
*/
static struct character queue[HM_LEN];
static unsigned int front = 0, back = 0;

constexpr unsigned int MASK = HM_LEN - 1; // should be 2 ** 8 - 1

bool pq_enqueue(struct character c) {
	// overflow
	if ((back + 1) & MASK == front) return false;
	
	// enqueue the actual character
	int i = front;
	for (; i < back && queue[i].count <= c.count; i = (i + 1) & MASK);
	back = (back + 1) & MASK;

	for (int j = back; j != i; j = (j - 1) & MASK)
		queue[j] = queue[(j - 1) & MASK];

	queue[i] = c;
	return true;
}

struct character pq_dequeue() {
	// queue full
	if (front == back) return (struct character) {'\0', 0};

	unsigned int f = front; // tmp
	front = (front + 1) & MASK;
	return queue[f];
}

void pq_print() {
	for (int i = front; i < back; i++) {
		if ((queue[i].c >= 'A' && queue[i].c <= 'Z') || (queue[i].c >= 'a' && queue[i].c <= 'z'))
			printf("%c(%d):%d ", queue[i].c, queue[i].c, queue[i].count);
		else
			printf("(%d):%d ", queue[i].c, queue[i].count);
	}
	printf("\n");
}
