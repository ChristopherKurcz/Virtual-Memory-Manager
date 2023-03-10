#include "vmm.h"

// Memory Manager implementation
// Implement all other functions here...


////////////////////////////////////////////////////////////////////////////////
//
// Function     : init_queue
// Description  : Initializes a queue
//                  
//
// Inputs       : None
// Outputs      : Returns the created queue, null if failed

QUEUE* init_queue(){
    QUEUE* queue = (QUEUE*) malloc(sizeof(QUEUE));
    if(queue) {
        queue->head = NULL;
        queue->tail = NULL;
        queue->hand = NULL;
        queue->size = 0;
        return queue;
    } else {
        return NULL;
    }
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : init_page
// Description  : Initializes a page
//                  
//
// Inputs       : int pageNum - pageNum for the associated page
// Outputs      : Returns the created page, null if failed

PAGE* init_page(int pageNum){
    PAGE* newPage = malloc(sizeof(PAGE));

    newPage->pageNum = pageNum;
    newPage->referenced = 0; // Set depending on fault type later
    newPage->modified = 0;   // Set depending on fault type later
    newPage->frameNum = -1;  // Set when inserted into queue
    newPage->canRead = false;
    newPage->canWrite = false;
    newPage->thirdChanceTaken = false;
    newPage->next = NULL;

    return newPage;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : queue_insert
// Description  : Inserts a given page into the queue, evicting a page if needed
//                  
//
// Inputs       : QUEUE* queue - queue instance to insert the page into
//              : PAGE* newPage - page instance to be inserted into the queue
//              : int numFrames - number of frames in the queue
//              : void* vm_ptr - pointer to the start of virtual memory
//              : int pageSize - the size of memory for each page
//              : int policy - the virtual memory manager's policy, either FIFO or THIRD
// Outputs      : Returns the evicted page if one was evicted, null otherwise

PAGE* queue_insert(QUEUE* queue, PAGE* newPage, int numFrames, void* vm_ptr, int pageSize, int policy){
    PAGE* evictedPage = NULL;
    
    if(queue->size == 0) {
        // if queue empty, set the page as the queue's head
        newPage->frameNum = 0;
        queue->head = newPage;
        queue->tail = newPage;
        if (policy == MM_THIRD){
            queue->hand = newPage;
        }
        queue->size ++;
    }
    else if(queue->size < numFrames) {
        // if queue is not yet full, add the page to the end of the queue
        newPage->frameNum = queue->size;
        queue->tail->next = newPage;
        queue->tail = newPage;
        queue->size ++;
        if ((queue->size == numFrames) && (policy == MM_THIRD)){
            // if the new page added fills the queue, for a third chance policy the last page
            // must point to the head, making it a circular queue
            newPage->next = queue->head;
        }
    }
    else if(queue->size == numFrames){
        if (policy == MM_FIFO){
            // if the queue is full, evict the head and add the page to the tail
            evictedPage = queue->head;
            queue->head = queue->head->next;
            newPage->frameNum = evictedPage->frameNum;
            queue->tail->next = newPage;
            queue->tail = newPage;
            // Needed in the case of single available frame
            if(queue->head == NULL) {
                queue->head = newPage;
            }
        } else if ((policy == MM_THIRD) && (numFrames > 1)){
            // for third chance policy, find the page needed to be evicted and replace it
            evictedPage = find_eviction_page(queue, vm_ptr, pageSize);
            PAGE* previousPage = find_previous_page(evictedPage);
            newPage->frameNum = evictedPage->frameNum;
            newPage->next = evictedPage->next;
            previousPage->next = newPage;
            queue->hand = newPage->next;
        } else if ((policy == MM_THIRD) && (numFrames == 1)){
            // if the queue is only of size one, just replace the existing page
            evictedPage = queue->hand;
            newPage->frameNum = evictedPage->frameNum;
            queue->hand = newPage;
        }
    } 
    
    return evictedPage;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : find_eviction_page
// Description  : Given a circular queue, find the next page to be evicted following
//                  the third chance policy
//                  
//
// Inputs       : QUEUE* queue - queue instance
//              : void* vm_ptr - pointer to the start of virtual memory
//              : int pageSize - the size of memory for each page
// Outputs      : Returns the page to be evicted

PAGE* find_eviction_page(QUEUE* queue, void* vm_ptr, int pageSize){
    PAGE* currentPage = queue->hand;

    while (true){
        if (currentPage->referenced == 1){ // same case for modified == 1 or 0
            // if the page was referenced, reset the bit and set protection to none
            // to catch any future references
            currentPage->referenced = 0;
            mprotect((char*)vm_ptr + (currentPage->pageNum * pageSize), pageSize, PROT_NONE);
            currentPage = currentPage->next;
            continue;
        } else if ((currentPage->referenced == 0) && (currentPage->modified == 1)){
            if (currentPage->thirdChanceTaken == false){
                // allow pages to take a third chance before being evicted
                currentPage->thirdChanceTaken = true;
                currentPage = currentPage->next;
                continue;
            } else if (currentPage->thirdChanceTaken == true){
                break;
            }
        } else if ((currentPage->referenced == 0) && (currentPage->modified == 0)){
            break;
        }
    }

    return currentPage;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : find_previous_page
// Description  : Given a page in a circular queue, find the previous page
//                  in the cycle that points to the given page
//                  
//
// Inputs       : PAGE* page - page instance
// Outputs      : Returns the previous page

PAGE* find_previous_page(PAGE* page){
    PAGE* currentPage = page->next;

    while (currentPage->next != page){
        currentPage = currentPage->next;
    }

    return currentPage;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : find_in_queue
// Description  : finds a page in a queue given its pageNum
//                  
//
// Inputs       : QUEUE* queue - queue instance
//              : int pageNum - pageNum of requested page
//              : int policy - the virtual memory manager's policy, either FIFO or THIRD
// Outputs      : Returns the previous page

PAGE* find_in_queue(QUEUE* queue, int pageNum, int policy){
    if(queue->size == 0) {
        return NULL;
    }
    
    // For FIFO policy, if page is head, return true
    if ((policy == MM_FIFO) && (queue->head->pageNum == pageNum)) {
        return queue->head;
    }

    // If page is hand, return true
    if ((policy == MM_THIRD) && (queue->hand->pageNum == pageNum)) {
        return queue->hand;
    }

    PAGE* currentPage;

    if (policy == MM_FIFO){
        currentPage = queue->head;
    } else if (policy == MM_THIRD){
        currentPage = queue->hand;
    }
    
    // Traverse queue until we find the page
    while((currentPage->next != NULL) && (currentPage->next != queue->hand)){
        if(currentPage->next->pageNum == pageNum) {
            return currentPage->next;
        }
        currentPage = currentPage->next;
    }

    // If we reach this, the page is not in queue so return NULL
    return NULL;
}


////////////////////////////////////////////////////////////////////////////////
//
// Function     : get_fault_type
// Description  : Returns the type of fault that occured
//                  
//
// Inputs       : ucontext_t* context - context of the fault
//              : QUEUE* queue - queue instance
//              : PAGE* referencedPage - the page that the fault occured with
//              : int policy - the virtual memory manager's policy, either FIFO or THIRD
// Outputs      : Returns the previous page

int get_fault_type(ucontext_t* context, QUEUE* queue, PAGE* referencedPage, int policy){
    int faultType;
    // We & the reg with 0x2 to only access second bit (the bit that tells us if read or write caused fault)
    // A read will result being 0x0, a write will result being 0x2
    faultType = context->uc_mcontext.gregs[REG_ERR] & 0x2;

    // If fault was a write
    if(faultType != 0) {

        // If we dont have a reference to that page in our queue already
        if(referencedPage == NULL) {
            faultType = 1; 
        }
        // Else, it is in our queue
        else {
            if ((policy == MM_THIRD) && (referencedPage->canWrite)){
                faultType = 4;
            } else {
                faultType = 2;
            }
        }
    }

    // If fault was a read
    else if(faultType == 0) {
        if((policy == MM_FIFO) || (referencedPage == NULL)){
            faultType = 0;
        }
        else if (referencedPage->canRead || referencedPage->canWrite){
            faultType = 3;
        }
    }
    
    // In case something goes wrong
    else {
        faultType = -1;
    }
    return faultType;
}
