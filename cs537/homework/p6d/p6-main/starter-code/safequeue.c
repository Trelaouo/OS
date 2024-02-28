#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "safequeue.h"

static struct PQ_node **queue; // PQ: List of pointers to PQ_node
static int size = 0; // Current size of the queue
static int max_size; // Maximum size of the queue
static pthread_mutex_t queue_mutex; // Mutex for queue access synchronization
static pthread_cond_t queue_worker_cond; // Condition variable for worker threads

// Create a queue with a specified maximum size
void create_queue(int max_queue_size) {

    // Initialize a mutex
    pthread_mutex_init(&queue_mutex, NULL);
    // Initialize a condition variable
    pthread_cond_init(&queue_worker_cond, NULL);

    // Set the maximum size
    max_size = max_queue_size;

    queue = malloc(max_size * sizeof(struct PQ_node*));

    if (queue == NULL) {
        perror("Error: Unable to allocate memory.");
        exit(1);
    }
}

// Add new node to queue
int add_work(struct PQ_node *new_node) {
    // Lock the queue mutex to ensure thread safety
    pthread_mutex_lock(&queue_mutex);

    // Check if the queue is full
    if (size >= max_size) {
        pthread_mutex_unlock(&queue_mutex);
        perror("Error: The queue is full");
        return -1;
    }

    // Check if new node is null
    if (new_node == NULL) {
        pthread_mutex_unlock(&queue_mutex);
        perror("Error: Unable to allocate memory.");
        return -1;
    }

    // Add new node to queue and increment
    queue[size++] = new_node;

    // Signal worker threads that new work added
    pthread_cond_signal(&queue_worker_cond);

    // Unlock the queue mutex before returning.
    pthread_mutex_unlock(&queue_mutex);

    return 0;
}


// Get the highest priority work from the queue.
struct PQ_node *get_work() {
    // Lock the mutex
    pthread_mutex_lock(&queue_mutex);

    // Wait while the queue is empty. The thread will block here until signalled
    while (size <= 0) {
        pthread_cond_wait(&queue_worker_cond, &queue_mutex);
    }

    // Retrieve the index of the node with the highest priority.
    int highest_priority = -1;
    int pop_index = -1;

    for (int i = 0; i < size; i++) {
        if (queue[i]->priority > highest_priority) {
            highest_priority = queue[i]->priority;
            pop_index = i;
        }
    }
    
    struct PQ_node *pop_node = queue[pop_index];

    size--;
    // Shift all elements after the removed node one index left
    for (int i = pop_index; i < size; i++) {
        queue[i] = queue[i + 1];
    }

    pthread_mutex_unlock(&queue_mutex);

    return pop_node;
}

// Get the highest priority node from the queue without blocking.
struct PQ_node *get_work_nonblocking() {
    // Lock the mutex
    pthread_mutex_lock(&queue_mutex);

    // Check if the queue is empty
    if (size <= 0) {
        pthread_mutex_unlock(&queue_mutex);
        perror("Error: No elements on the queue.\n");
        return NULL;
    }

    // Retrieve the index of the node with the highest priority.
    int highest_priority = -1;
    int pop_index = -1;

    for (int i = 0; i < size; i++) {
        if (queue[i]->priority > highest_priority) {
            highest_priority = queue[i]->priority;
            pop_index = i;
        }
    }    
    struct PQ_node *pop_node = queue[pop_index];

    size--;
    // Shift all elements after the removed node one index left
    for (int i = pop_index; i < size; i++) {
        queue[i] = queue[i + 1];
    }

    // Unlock the mutex
    pthread_mutex_unlock(&queue_mutex);

    return pop_node;
}

// Destroy the queue and free memmory after usage
void destroy_queue() {
    pthread_mutex_lock(&queue_mutex);

    for (int i = 0; i < size; i++) {
        free(queue[i]->path);
        free(queue[i]);
    }

    free(queue);
    size = 0;

    pthread_mutex_unlock(&queue_mutex);
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&queue_worker_cond);
}

