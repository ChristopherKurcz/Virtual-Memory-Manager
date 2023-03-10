#ifndef VMM_H
#define VMM_H

#include "interface.h"

// Declare your own data structures and functions here...

typedef struct queue_struct QUEUE;
typedef struct page_struct PAGE;

enum fault_type
{
    READ_FAULT = 0,
    WRITE_FAULT = 1,
    PERM_FAULT = 2,
    TRACK_READ_FAULT = 3,
    TRACK_WRITE_FAULT = 4
};


struct queue_struct
{
    PAGE* head;
    PAGE* tail;
    PAGE* hand;
    int size;
};


struct page_struct
{
    int frameNum;
    int pageNum;
    unsigned int referenced : 1;
    unsigned int modified : 1;
    bool canRead;
    bool canWrite;
    bool thirdChanceTaken;
    PAGE* next;
};


QUEUE* init_queue();
    // Initializes a queue

PAGE* init_page(int pageNum);
    // Initializes a page

PAGE* queue_insert(QUEUE* queue, PAGE* newPage, int numFrames, void* vm_ptr, int pageSize, int policy);
    // Inserts a given page into the queue, evicting a page if needed

PAGE* find_eviction_page(QUEUE* queue, void* vm_ptr, int pageSize);
    // Given a circular queue, find the next page to be evicted following the third chance policy

PAGE* find_previous_page(PAGE* page);
    // Given a page in a circular queue, find the previous page in the cycle that points to the given page

PAGE* find_in_queue(QUEUE* queue, int pageNum, int policy);
    // finds a page in a queue given its pageNum

int get_fault_type(ucontext_t* context, QUEUE* queue, PAGE* referencedPage, int policy);
    // Returns the type of fault that occured

#endif
