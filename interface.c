#include "interface.h"
#include "vmm.h"

// Interface implementation
// Implement APIs here...

// Global variables
QUEUE* queue;
int managerPolicy;
int pageSize;
int numFrames;
void* vm_ptr;


void mm_init(enum policy_type policy, void *vm, int vm_size, int num_frames, int page_size) {   
    // initialize global variables
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    numFrames = num_frames;
    managerPolicy = policy;
    pageSize = page_size;
    vm_ptr = vm;
    int protCheck;

    // link segfaults to our segfault handler
    sa.sa_sigaction = sigsegv_handler;
    sigaction(SIGSEGV, &sa, NULL);

    // initialize the queue to store pages
    queue = init_queue();

    // set the protection on the virtual memory to none so that a fault occurs on any access
    protCheck = mprotect(vm, vm_size, PROT_NONE);
}


void sigsegv_handler(int sig, siginfo_t* info, void* ucontext){
    int faultType;
    int pageNum;
    int offset;
    char* virtAddr;
    unsigned int physAddr;
    PAGE* referencedPage = NULL;
    PAGE* newPage = NULL;
    PAGE* evictedPage;
    int evictedPageNum;
    int writeback;

    // Get address of fault
    virtAddr = (char*)info->si_addr;

    // Get page in which the fault occured
    pageNum = (virtAddr - (char*)vm_ptr) / pageSize;
    offset = (virtAddr - (char*)vm_ptr)% pageSize;
    
    // Figure out page is already in the queue
    referencedPage = find_in_queue(queue, pageNum, managerPolicy);

    if(referencedPage != NULL) {
        physAddr = referencedPage->frameNum * pageSize + offset;
        evictedPageNum = -1;
        writeback = 0;
    }
    // Else, page is not in queue so create one
    else {
        newPage = init_page(pageNum);
        newPage->referenced = 1;
        evictedPage = queue_insert(queue, newPage, numFrames, vm_ptr, pageSize, managerPolicy);
        // If we did not evict a page becuase queue is not full
        if(evictedPage == NULL) {
            // If the queue is not yet full
            // Before the Queue is filled, the frames are in order so we can use queue size
            physAddr = (queue->size - 1) * pageSize + offset; 
            evictedPageNum = -1;
            writeback = 0;
        }
        // Else we had to evict something
        else {
            // Since we evicted something, we use evicted pages frame number 
            physAddr = evictedPage->frameNum * pageSize + offset;
            evictedPageNum = evictedPage->pageNum;
            writeback = evictedPage->modified;
            // Protect evicted pages virtual address with PROT_NONE so we get a signal if its referenced again
            mprotect((char*)vm_ptr + (evictedPageNum * pageSize), pageSize, PROT_NONE);
        }
    }

    // Get the type of fault by using ucontext struct to access ERR register
    ucontext_t* context = (ucontext_t*) ucontext;
    faultType = get_fault_type(context, queue, referencedPage, managerPolicy);
    switch (faultType)
    {
    case READ_FAULT:
        // Fault type: Read access to a non-present page
        // So, set the protection to read and set the referenced bit
        mprotect((char*)vm_ptr + (pageNum * pageSize), pageSize, PROT_READ);
        newPage->canRead = true;
        newPage->referenced = 1;
        break;
    case WRITE_FAULT:
        // Fault type: Write access to a non-present page
        // So, set the protection to read/wrtie and set the referenced/modified bits
        mprotect((char*)vm_ptr + (pageNum * pageSize), pageSize, PROT_READ | PROT_WRITE);
        newPage->canWrite = true;
        newPage->canRead = true;
        newPage->referenced = 1;
        newPage->modified = 1;
        break;
    case PERM_FAULT:
        // Fault type: Write access to a currently Read-only page
        // So, update the protection to be read/write and set the referenced/modified bits 
        mprotect((char*)vm_ptr + (pageNum * pageSize), pageSize, PROT_READ | PROT_WRITE);
        referencedPage->canWrite = true;
        referencedPage->referenced = 1;
        referencedPage->modified = 1;
        break;
    case TRACK_READ_FAULT:
        // Fault type: Track a "read" reference to the page that has Read and/or Write permissions on
        // So, update the the protection to read and reset the refernced bit and the third chance
        mprotect((char*)vm_ptr + (pageNum * pageSize), pageSize, PROT_READ);
        referencedPage->referenced = 1;
        referencedPage->thirdChanceTaken = false;
        break;
    case TRACK_WRITE_FAULT:
        // Fault type: Track a "write" reference to the page that has Read-Write permissions on
        // So, update the protection to read/write and set the referenced/modified bits and the third chance
        mprotect((char*)vm_ptr + (pageNum * pageSize), pageSize, PROT_READ | PROT_WRITE);
        referencedPage->referenced = 1;
        referencedPage->modified = 1;
        referencedPage->thirdChanceTaken = false;
        break;
    default:
        break;
    }

    // log the fault that occured with all the collected data
    mm_logger(pageNum, faultType, evictedPageNum, writeback, physAddr); 
}