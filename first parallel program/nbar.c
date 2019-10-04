#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/wait.h>

volatile struct shared {
  int lock;
  int key;
  /* OTHER SHARED STUFF GOES HERE... */
} *shared;

inline static int
xchg(register int reg, volatile int * volatile obj)
{
  /* Atomic exchange instruction */
__asm__ __volatile__ ("xchgl %1,%0"
                      :"=r" (reg), "=m" (*obj)
                      :"r" (reg), "m" (*obj));
  return(reg);
}

inline static void
lock(void)
{
  /* Atomic spin lock... */
  while (xchg(1, &(shared->lock))) ;  
}

inline static void
unlock(void)
{
  /* Unlock... */
  shared->lock = 0;
}

int
main(int argc, char **argv)
{
  register int i, shmid;
  register int iproc = 0;
  register int nproc = 0;

  /* Allocate System V shared memory */
  shmid = shmget(IPC_PRIVATE,
                 sizeof(struct shared),
                 (IPC_CREAT | 0600));
  shared = ((volatile struct shared *) shmat(shmid, 0, 0));
  shmctl(shmid, IPC_RMID, 0);

  /* Initialize... */
  shared->lock = 0;
  shared->key = atoi(argv[1]);
  unlock();
  /* Make the processes... */
  if ((argc != 2) ||
      ((nproc = atoi(argv[1])) < 1) ||
      (nproc > 8)) {
	fprintf(stderr, "%d is not a reasonable number of processes\n", nproc);
	exit(1);
  } else {
    /* Children are 1..nproc-1 */
    while (++iproc < nproc) {
      /* Fork a child */
      if (!fork()) break;
    }
    /* Parent is 0 */
    if (iproc >= nproc) iproc = 0;
  }

  printf("I am PE%d\n", iproc);

  shared->key--;
  /*printf("key is%d\n", shared->key);*/
  
  

   while (shared->key >= 0) {
           if (shared->key <= 0) break;
           lock();
	 }
     
  
   unlock();
  



  /* DO SOMETHING CLEVER HERE */

  printf("I am still PE%d\n", iproc);

  /* Parent waits for all children to have died */
  if (iproc == 0) {
    for (i=1; i<nproc; ++i) wait(0);
    printf("All Done\n");
  }

  /* Check out */
  return(0);
}
