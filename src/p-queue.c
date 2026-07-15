#include "common.h"
#include <stdio.h>

/*
I wanted to make this a dynamic array, but if we're hardcoding the hash map as
a direct map, might as well do the same for the priority queue
*/
static struct character *queue[HM_LEN];
static unsigned int front = 0, back = 0;

void pq_reset() {
	front = back = 0;
}

bool pq_enqueue(struct character *c) {
	// overflow
	if (back >= HM_LEN) return false;
	
	// enqueue the actual character
	int i = front;
	for (; i < back && queue[i]->count <= c->count; i++);
	back++;

	for (int j = back; j != i; j--)
		queue[j] = queue[j-1];

	queue[i] = c;
	return true;
}

struct character *pq_dequeue() {
	// queue full
	if (front == back) return NULL;
	return queue[front++];
}

void pq_print() {
	for (int i = front; i < back; i++)
		if (queue[i]->c > ' ' && queue[i]->c <= '~')
			printf("%c(%d):%d ", queue[i]->c, queue[i]->c, queue[i]->count);
		else
			printf("(%d):%d ", queue[i]->c, queue[i]->count);
	printf("\n");
}
