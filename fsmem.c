#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#define FSMEM_C
#include "fsmem.h"


static unsigned long   FnCounter;
static fsmap_t         FsMap[1024];
static long           nFsMap;


////////////////////////////////////////////////////////////////////////////////
// FileSystem-level code
////////////////////////////////////////////////////////////////////////////////


static fsmap_t* fsmem_create(size_t size)
{
  void        *p; 
  char         b[128],n='\0';
  int          fd;

  // Generate unique file name from incremented counter
  FnCounter++;
  sprintf(b,"fsmem_block-%lu",FnCounter);
  if( (fd=open(b,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR)) < 0 ) {
    fprintf(stderr,"fsmem: could not open new backend file \"%s\".\n",b);
    exit(1);
  }
  if( lseek(fd,size,SEEK_SET) < 0 ) {
    fprintf(stderr,"fsmem: could not seek to grow new backend file.\n");
    exit(1);
  }
  if( write(fd,&n,1) != 1 ) {
    fprintf(stderr,"fsmem: could not write to grow new backend file.\n");
    exit(1);
  }
  if( close(fd) < 0 ) {
    fprintf(stderr,"fsmem: could not close to grow new backend file.\n");
    exit(1);
  }

  // Open and map the new file
  fd = open(b, O_RDWR, S_IRUSR|S_IWUSR);
  if( fd < 0 ) {
    fprintf(stderr,"fsmem: Failed to reopen new backend file.\n");
    exit(1);
  }
  p = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
  if( p == MAP_FAILED ) {
    fprintf(stderr,"fsmem: Failed to mmap new backend file.\n");
    exit(1);
  }
  close(fd);

  // Record this mapping in the list
  nFsMap++;
  if( nFsMap > 1024 ) {
    fprintf(stderr,"fsmem: Primary list of fs mappings filled (size=1024).\n");
    exit(1);    
  }
  FsMap[nFsMap-1].name     = FnCounter;
  FsMap[nFsMap-1].addr     = p;
  FsMap[nFsMap-1].size     = size;
  FsMap[nFsMap-1].used     = 0;

  // Return the newly created mapping.
  return &FsMap[nFsMap-1];
}


// !!av:  These could be handy at some point if they had some work
//        done to them.
/*
static void fsmem_delete(fsmap_t *fsmap)
{ 
  char        b[128];

  // Unmap the fsmap
  munmap(fsmap->addr, fsmap->size);

  // Remove the file from disk
  sprintf(b,"fsmem_block-%d",fsmap->name);
  unlink(b);

  // !!av: what about removal from main list?
}


static void fsmem_enlarge(fsmap_t *fsmap, size_t size)
{
  void        *p; 
  char         b[128],n='\0';
  int          fd;

  // Unmap the fsmap
  munmap(fsmap->addr, fsmap->size);

  // Open the file read/write; then enlarge
  sprintf(b,"fsmem_block-%d",fsmap->name);
  if( (fd=open(b,O_RDWR,S_IRUSR|S_IWUSR)) < 0 ) {
    fprintf(stderr,"fsmem: could not open backend file \"%s\".\n",b);
    exit(1);
  }
  if( lseek(fd,fsmap->size+size,SEEK_SET) < 0 ) {
    fprintf(stderr,"fsmem: could not seek to grow backend file.\n");
    exit(1);
  }
  if( write(fd,&n,1) != 1 ) {
    fprintf(stderr,"fsmem: could not write to grow backend file.\n");
    exit(1);
  }
  if( close(fd) < 0 ) {
    fprintf(stderr,"fsmem: could not close to grow backend file.\n");
    exit(1);
  }

  // Open and map the existing file
  fd = open(b, O_RDWR, S_IRUSR|S_IWUSR);
  if( fd < 0 ) {
    fprintf(stderr,"fsmem: Failed to reopen backend file.\n");
    exit(1);
  }
  p = mmap(fsmap->addr, fsmap->size+size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, fd, 0);
  if( p == MAP_FAILED ) {
    fprintf(stderr,"fsmem: Failed to mmap backend file.\n");
    exit(1);
  }
  close(fd);

  // Clear the new memory and record the larger sized mapping
  memset(p+fsmap->size,0,size);
  fsmap->addr  = p;
  fsmap->size += size;
}
*/


////////////////////////////////////////////////////////////////////////////////
// Internal utility code: init, print stats, etc.
////////////////////////////////////////////////////////////////////////////////


static void pstats()
{
  int     i;

  // Time
  printf("----------------------------------------\n");
  printf("time: %lu\n\n",time(NULL));

  // fsmap[] stats
  for(i=0;i<nFsMap;i++) {
    printf("FsMap[%d]:  size: %15lu  used: %15lu\n (%d%c)",
	   i,FsMap[i].size,FsMap[i].used,
	   ((int)(((double)FsMap[i].used*100.0)/((double)FsMap[i].size))),'%');

  }
  printf("\n");

  // sbrk() stats
  /*
  if( f ) {
    printf("f: %lu\n",f);
    for(i=47; i>=0; i--) {
      if( fd[i] ) {
	printf("%lu:%lu  ",((1ul)<<i),fd[i]);
      }
    }
    printf("\n");
  }
  */

  printf("----------------------------------------\n");
}


// We want an init so that we can do some setup
static inline void init()
{
  static int   ini = 0;
  static long  ot,t;
  fsmap_t     *fsmap;


  // Time?
  t = time(NULL);

  // Init if needed
  if( !ini ) {
    ini = 1;
    // Let's install an atexit() or something to print out
    // the stats that we collected.
    fsmap = fsmem_create( FSMEM_FS_SIZE * 1024 );
    atexit(pstats);
  }
  
  // Print trace update every few secs or so
  if( t-ot > 1 ) {
    ot = t;
    pstats();
  }

}


////////////////////////////////////////////////////////////////////////////////
// sbrk() interface
////////////////////////////////////////////////////////////////////////////////


void* fsmem_sbrk(intptr_t increment)
{
  size_t size;

  init();

  
  // Check for reduction
  if( increment < 0 ) {
    // !!av: Handle reductions?
    return ((void*)(-1));
  }

  // If increment == 0, return one byte past end of allocated space
  if( !increment ) {
    return FsMap[0].addr+FsMap[0].used;
  }

  // Chunk new space off
  size           = FsMap[0].used;
  FsMap[0].used += increment;
  if( FsMap[0].used > FsMap[0].size ) {

    // !!av: Enlarge area?
    // fsmem_enlarge(&(FsMap[0]), increment);

    fprintf(stderr,"fsmem: out of fsmem memory (%lu/%lu bytes).\n",
	    ((unsigned long)FsMap[0].used), 
	    ((unsigned long)FsMap[0].size));
    return ((void*)(-1));
  }

  // !!av: Stat counting for sbrk?

  return FsMap[0].addr + size;
}
