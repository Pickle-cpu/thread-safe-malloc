#ifndef __MY_MALLOC_H__
#define __MY_MALLOC_H__

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

// struct for node
// element of linked list
struct single_node {
  void * user_start_address;
  size_t n_size;
  size_t free_status;
  struct single_node * prev;
  struct single_node * next;
};
typedef struct single_node single_node_t;

// useful methods
void initialnode(single_node_t * node);
single_node_t * asknewspace(size_t size, size_t lockdecision);
void replacenode(size_t size, single_node_t * temp, single_node_t * replace, single_node_t ** head);
void splitspace(size_t size, single_node_t * temp, single_node_t ** head);
single_node_t * bfselectfree(size_t size, single_node_t ** head);
void deletenode(single_node_t * node, single_node_t ** head);
void insertnode(single_node_t * node, single_node_t ** head);
void mergenode(single_node_t * node);

//Best Fit malloc/free
void *bf_malloc(size_t size, size_t lockdecision, single_node_t ** head);
void bf_free(void *ptr, size_t lockdecision, single_node_t ** head);

//Thread Safe malloc/free: locking version
void *ts_malloc_lock(size_t size);
void ts_free_lock(void *ptr);

//Thread Safe malloc/free: non-locking version
void *ts_malloc_nolock(size_t size);
void ts_free_nolock(void *ptr);

#endif