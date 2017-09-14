#ifndef FSMEM_H
#define FSMEM_H


#include <stdlib.h>
#include <stdint.h>


// Default size of each new file to be mmap()ed.
#define FSMEM_FS_SIZE      (1ul<<30)     // 1 GiB


typedef struct {
  int      name;  // Name (as integer) of this mapping
  void    *addr;  // Mapped address
  size_t   size;  // Size of the mapping
  size_t   used;  // Used portion of the mapping
} fsmap_t;


////////////////////////////////////////////////////////////////////////////////
// Interface
////////////////////////////////////////////////////////////////////////////////
#ifndef FSMEM_C
extern void* fsmem_sbrk(intptr_t increment);
#endif


#endif
