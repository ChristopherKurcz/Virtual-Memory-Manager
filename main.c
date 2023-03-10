#include <sys/stat.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#include "interface.h"

// Statistics structure
int statCounter;
struct MM_stats
{
    int virt_page;
    int fault_type;
    int evicted_page;
    int write_back;
    unsigned int phy_addr;
};
struct MM_stats *stats;

#define MAX_OPS 100000
#define MAX_LINE_LEN 1024

// Command structure
struct command
{
    char operation[10];
    int pageNumber;
    int startOffset;
    int value;
};

bool read_next_op(FILE *fd, struct command *op);
void print_stats(FILE *fp);

// Main function
// Read input file and call read/write accordingly
int main(int argc, char *argv[])
{
    printf("%s: Hello Project 3!\n", __func__);
    if (argc < 4)
    {
        fprintf(stderr, "Not enough parameters provided.  Usage: ./proj3 <replacement_policy> <num_frames> <input_file>\n");
        fprintf(stderr, "  page replacement policy: 1 - FIFO\n");
        fprintf(stderr, "  page replacement policy: 2 - Third Chance\n");
        return -1;
    }

    // Verify input
    int policy = atoi(argv[1]);
    if (policy != MM_FIFO && policy != MM_THIRD)
    {
        fprintf(stderr, "Invalid option\n");
        return -1;
    }
    int num_frames = atoi(argv[2]);
    if (num_frames <= 0)
    {
        fprintf(stderr, "Invalid number of frames\n");
        return -1;
    }

    // Open input file
    FILE *input_file = fopen(argv[3], "r");
    if (input_file == NULL)
    {
        perror("fopen() error");
        return errno;
    }

    // Open output file
    FILE *output_file;
    char output_filename[MAX_LINE_LEN] = {0};
    mkdir("output", 0755);
    strcat(output_filename, "output/result-");
    strcat(output_filename, argv[1]);   // policy
    strcat(output_filename, "-");
    strcat(output_filename, argv[2]);   // number of frames
    strcat(output_filename, "-");
    strcat(output_filename, basename(argv[3]));
    output_file = fopen(output_filename, "w");
    if (output_file == NULL)
    {
        perror("fopen() error");
        return errno;
    }

    // Allocate stat structure
    statCounter = 0;
    stats = (struct MM_stats *)malloc(sizeof(struct MM_stats) * MAX_OPS);
    if (!stats)
    {
        perror("malloc() error");
        return errno;
    }

    // Allocate virtual memory
    void *vm_ptr;
    int PAGE_SIZE = sysconf(_SC_PAGE_SIZE);
    int vm_size = 16 * PAGE_SIZE;
    if (posix_memalign(&vm_ptr, PAGE_SIZE, vm_size))
    {
        fprintf(stderr, "posix_memalign failed\n");
        return -1;
    }
    fprintf(output_file, "Page Size: %d\n", PAGE_SIZE);
    fprintf(output_file, "Num Frames: %d\n", num_frames);

    // Init your memory manager
    mm_init(policy, vm_ptr, vm_size, num_frames, PAGE_SIZE);

    // Do Read/Write Operations
    struct command *op = (struct command *)malloc(sizeof(struct command));
    char *vm_ptr_char = (char *)vm_ptr; // Cast void* to char* for pointer arithmetic
    while (read_next_op(input_file, op))
    {
        if (strcmp(op->operation, "read") == 0)
        {
            int read_value = vm_ptr_char[(op->startOffset * 4) + (op->pageNumber * PAGE_SIZE)];
        }
        else if (strcmp(op->operation, "write") == 0)
        {
            vm_ptr_char[(op->startOffset * 4) + (op->pageNumber * PAGE_SIZE)] = op->value;
        }
        else
        {
            fprintf(stderr, "Incorrect input file content\n");
            return -1;
        }
    }

    print_stats(output_file);

    fclose(input_file);
    fclose(output_file);
    free(op);
    free(stats);
    free(vm_ptr);

    printf("%s: Output file: %s\n", __func__, output_filename);
    printf("%s: Bye!\n", __func__);
    return 0;
}

bool read_next_op(FILE *fd, struct command *op)
{
    char line[MAX_LINE_LEN] = {0};
    if (!fgets(line, MAX_LINE_LEN, fd))
        return false;
    
    memset(op, 0, sizeof(struct command));

    char *token;
    char delim[2] = " ";
    char *rest = line;

    if (token = strtok_r(rest, delim, &rest))
        strcpy(op->operation, token);
    else
        return false;

    if (token = strtok_r(rest, delim, &rest))
        op->pageNumber = atoi(token);
    else
        return false;

    if (token = strtok_r(rest, delim, &rest))
        op->startOffset = atoi(token);
    else
        return false;

    if (token = strtok_r(rest, delim, &rest))
        op->value = atoi(token);
    else
        return false;

    if ((strcmp(op->operation, "read") != 0) && (strcmp(op->operation, "write") != 0))
    {
        fprintf(stderr, "%s: Invalid operation in input file.\n", __func__);
        exit(EXIT_FAILURE);
    }
    if (op->pageNumber < 0 || op->startOffset < 0)
    {
        fprintf(stderr, "%s: Invalid number in input file.\n", __func__);
        exit(EXIT_FAILURE);
    }

    return true;
}

void print_stats(FILE *fp)
{
    fprintf(fp, "type\tvirt-page\tevicted-virt-page\twrite-back\tphy-addr\n");
    for (int i = 0; i < statCounter; i++)
    {
        fprintf(fp, "%d\t\t%d\t\t%d\t\t%d\t\t0x%04x\n",
                stats[i].fault_type, stats[i].virt_page, stats[i].evicted_page,
                stats[i].write_back, stats[i].phy_addr);
    }
}

void mm_logger(int virt_page, int fault_type, int evicted_page, int write_back, unsigned int phy_addr)
{
    stats[statCounter].virt_page = virt_page;
    stats[statCounter].fault_type = fault_type;
    stats[statCounter].evicted_page = evicted_page;
    stats[statCounter].write_back = write_back;
    stats[statCounter].phy_addr = phy_addr;
    statCounter++;
}
