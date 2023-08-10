#include "my_malloc.h"

// global variables
single_node_t * free_head = NULL;
__thread single_node_t * nolock_free_head = NULL;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// initial a node
void initialnode(single_node_t * node){

    node->user_start_address = (void *)node + sizeof(single_node_t);
    node->prev = NULL;
    node->next = NULL;

}

// allocate a new space with size of struct + size
single_node_t * asknewspace(size_t size, size_t lockdecision){

    single_node_t * node = NULL;
    // sbrk for allocation
    if(!lockdecision){
        pthread_mutex_lock(&lock);
        node = sbrk(size+sizeof(single_node_t));
        pthread_mutex_unlock(&lock);
    }else{
        node = sbrk(size+sizeof(single_node_t));
    }

    if(node == (void *)-1){
        return NULL;
    }

    // initial
    initialnode(node);
    node->free_status = 0;
    node->n_size = size;

    return node;

}


void replacenode(size_t size, single_node_t * temp, single_node_t * replace, single_node_t ** head){

    // since the remaining still works
    // change the details of replace
    replace->n_size = temp->n_size-size-sizeof(single_node_t);
    replace->free_status = 1;

    if((*head) == temp){

        single_node_t * tempnext = temp->next;

        temp->prev = NULL;
        temp->next = NULL;

        (*head) = replace;
        replace->prev = NULL;
        replace->next = tempnext;
        if(tempnext != NULL){
            tempnext->prev = replace;
        }
    }else if(temp->next == NULL){

        single_node_t * tempprev = temp->prev;

        temp->prev = NULL;
        temp->next = NULL;

        tempprev->next = replace;
        replace->prev = tempprev;
        replace->next = NULL;
    }else{

        single_node_t * tempprev = temp->prev;
        single_node_t * tempnext = temp->next;

        temp->prev = NULL;
        temp->next = NULL;
        
        tempprev->next = replace;
        tempnext->prev = replace;
        replace->prev = tempprev;
        replace->next = tempnext;
    }

    // change the details of temp
    temp->n_size = size;
    temp->free_status = 0;
    

}

// splite space into two parts
// first part will be used
// second part will still be in free list and wait to be used later
void splitspace(size_t size, single_node_t * temp, single_node_t ** head){
    
    // check if the remaining still can be used
    if((temp->n_size-size)<=sizeof(single_node_t)){

        // too small so just delete it
        deletenode(temp,head);
    
    }else{

        // create a new available node
        single_node_t * replace = (single_node_t *)(temp->user_start_address + size);
        initialnode(replace);
        // replace node in free list
        replacenode(size, temp, replace, head);
        
    }

}

// select free space for best fit
single_node_t * bfselectfree(size_t size, single_node_t ** head){

    single_node_t * temp = *head;
    single_node_t * temptouse = NULL;
    while(temp != NULL){
        // try to find the best matched one
        if(temp->n_size == size){
            temptouse = temp;
            break;
        }else if(temp->n_size > size){
            if((temptouse == NULL) || (temptouse->n_size > temp->n_size)){
                temptouse = temp;
            }
        }
        temp = temp->next;
    }

    if(temptouse != NULL){
        splitspace(size,temptouse,head);
    }
    
    return temptouse;

}

// delete node from linked list (free list)
void deletenode(single_node_t * node,single_node_t ** head){

    if((*head) == node){
        if((*head)->next == NULL){
            (*head) = NULL;
        }else{
            (*head) = node->next;
            (*head)->prev = NULL;
        }
    }else if(node->next == NULL){
        node->prev->next = NULL;
    }else{
        node->prev->next = node->next;
        node->next->prev = node->prev;
    }

    node->free_status = 0;
    node->prev = NULL;
    node->next = NULL;
}

// insert node to linked list (free list)
void insertnode(single_node_t * node, single_node_t ** head){

    // nothing inside
    if((*head) == NULL){

        (*head) = node;
        return;
    }

    // before head
    single_node_t * temp = (*head);
    if((node->user_start_address+node->n_size)<(void *)(*head)){

        node->next = temp;
        temp->prev = node;
        (*head) = node;
        return;
    }
    
    // at least has one in free list
    temp = (*head);
    while(temp != NULL){

        if((void *)node>=(temp->user_start_address+temp->n_size)){

            if(temp->next==NULL){

                temp->next = node;
                node->prev = temp;
                return;
            }else if((node->user_start_address+node->n_size)<=(void *)temp->next){

                single_node_t * tempnext= temp->next;
                temp->next = node;
                node->prev = temp;
                node->next = tempnext;
                tempnext->prev = node;
                return;
            }
        }

        temp = temp->next;
    }

}

// merge nodes
// situation 1, merge prev and node
// situation 2, merge node and next
// situation 3, merge prev, node, and next
void mergenode(single_node_t * node){

    // check if next is a free one
    if(node->next != NULL && node->next->free_status == 1){
        single_node_t * nodenext = node->next;

        // check if next and node are adjacent
        if((void *)nodenext == (node->user_start_address+node->n_size)){

            node->n_size = node->n_size + sizeof(single_node_t) + nodenext->n_size;
            node->next = nodenext->next;
            if(nodenext->next != NULL){
                nodenext->next->prev = node;
            }

            nodenext->next = NULL;
            nodenext->prev = NULL;

        }

    }

    // check if prev is a free one
    if(node->prev != NULL && node->prev->free_status == 1){
        single_node_t * nodeprev = node->prev;

        // check if prev and node are adjacent
        if((void *)node == (nodeprev->user_start_address+nodeprev->n_size)){

            nodeprev->n_size = nodeprev->n_size + sizeof(single_node_t) + node->n_size;
            nodeprev->next = node->next;
            if(node->next != NULL){
                node->next->prev = nodeprev;
            }
            

            node->next = NULL;
            node->prev = NULL;

        }

    }

}

//Best Fit malloc/free
void *bf_malloc(size_t size, size_t lockdecision, single_node_t ** head){

    single_node_t * node = bfselectfree(size,head);

    // cannot find one in free list
    // allocate a new one
    if(node == NULL){
        node = asknewspace(size,lockdecision);
    }

    return node->user_start_address;

}
void bf_free(void *ptr, size_t lockdecision, single_node_t ** head){
    
    single_node_t * node = (single_node_t *)(ptr - sizeof(single_node_t));
    node->free_status = 1;
    insertnode(node,head);
    mergenode(node);

}

//Thread Safe malloc/free: locking version
void *ts_malloc_lock(size_t size){

    pthread_mutex_lock(&lock);
    void * node_start_address = bf_malloc(size, 1, &free_head);
    pthread_mutex_unlock(&lock);
    return node_start_address;
}
void ts_free_lock(void *ptr){

    pthread_mutex_lock(&lock);
    bf_free(ptr, 1, &free_head);
    pthread_mutex_unlock(&lock);

}

//Thread Safe malloc/free: non-locking version
void *ts_malloc_nolock(size_t size){

    void * node_start_address = bf_malloc(size, 0, &nolock_free_head);
    return node_start_address;
}
void ts_free_nolock(void *ptr){
    bf_free(ptr, 0, &nolock_free_head);
}

