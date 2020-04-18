#include <stdint.h>
#include <stddef.h>

#define HEAP_START 0x600000
#define TOTAL_HEAP_SIZE 1024*1024
#define HEAP_END 0x95FFFFF

void malloc(size_t wanted_size, void ** ret_val);

void free( void * p);

void getFreeHeapSize(size_t * resp);

void getTotalHeapSize(size_t * resp);

void getTakenHeapSize(size_t * resp);