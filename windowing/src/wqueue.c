#include "wqueue.h"

#include <pthread.h>
#include <stdio.h>

#define QM_MAX_SIZE 50000


void queue_enqueue(queue_t *queue, queue_messages* value, int is_last) {
	pthread_mutex_lock(&(queue->mutex));
	while (queue->size == queue->capacity)
		pthread_cond_wait(&(queue->cond_full), &(queue->mutex));
	queue->qm[queue->in] = value;
	queue->end = is_last;
#ifdef WQUEUE_DEBUG
	if(is_last) {
		printf("set end to 1\n");
	}
#endif
	++ queue->size;
	++ queue->in;
	queue->in %= queue->capacity;
	pthread_mutex_unlock(&(queue->mutex));
	pthread_cond_broadcast(&(queue->cond_empty));
}

queue_messages* queue_dequeue(queue_t *queue) {
	int isover;
	pthread_mutex_lock(&(queue->mutex));
	while (queue->size == 0 && !queue->end)
		pthread_cond_wait(&(queue->cond_empty), &(queue->mutex));
	// we get here if enqueue happened otherwise if we are over.
	if (queue->size > 0) {
		queue_messages* value = queue->qm[queue->out];
		-- queue->size;
		++ queue->out;
		queue->out %= queue->capacity;
#ifdef WQUEUE_DEBUG
		printf("dequeue %d\n", value->id);
#endif
		pthread_mutex_unlock(&(queue->mutex));
		pthread_cond_broadcast(&(queue->cond_full));
		return value;
	} else {
#ifdef WQUEUE_DEBUG
		printf("isover \n");
#endif
		pthread_mutex_unlock(&(queue->mutex));
		return NULL;
	}
}

void queue_end(queue_t *queue) {
	pthread_mutex_lock(&(queue->mutex));
	queue->end = 1;
	pthread_mutex_unlock(&(queue->mutex));
	pthread_cond_broadcast(&(queue->cond_empty));
}

int queue_isend(queue_t *queue)
{
	int end;
	pthread_mutex_lock(&(queue->mutex));
	end = queue->end && queue->size == 0;
	pthread_mutex_unlock(&(queue->mutex));
	if (end) {
		pthread_cond_broadcast(&(queue->cond_empty));
	}
	return end;
}

int queue_size(queue_t *queue)
{
	pthread_mutex_lock(&(queue->mutex));
	int size = queue->size;
	pthread_mutex_unlock(&(queue->mutex));
	return size;
}