/*
c-pthread-queue - c implementation of a bounded buffer queue using posix threads
Copyright (C) 2008  Matthew Dickinson

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "common.h"

#include <pthread.h>
#include <stdio.h>

#ifndef _QUEUE_H
#define _QUEUE_H

#define QUEUE_INITIALIZER(buffer) { buffer, sizeof(buffer) / sizeof(buffer[0]), 0, 0, 0, 0, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, PTHREAD_COND_INITIALIZER }

#define WQUEUE_NTHREADS 8
#define QM_MAX_SIZE 50000

typedef struct queue_messages {
    int id;
	DNSMessage messages[QM_MAX_SIZE];
    int number;
} queue_messages;

typedef struct queue {
	queue_messages **qm;
	const int capacity;
	int size;
	int in;
	int out;
	int end;
	pthread_mutex_t mutex;
	pthread_cond_t cond_full;
	pthread_cond_t cond_empty;
} queue_t;

void queue_enqueue(queue_t *queue, queue_messages* value, int is_last);

queue_messages* queue_dequeue(queue_t *queue);

void queue_end(queue_t *queue);

int queue_isend(queue_t *queue);

int queue_size(queue_t *queue);

#endif