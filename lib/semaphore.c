#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include "semaphore.h"

union semun{
  int val;
  struct semid_ds *buf;
  unsigned short *array;
};

int semaphore_init(sema_t *s, int key)
{
  if(s == NULL)
    return -1;

  s->id = semget((key_t) key, s->sema_count, 0666 | IPC_CREAT);
  if(s->id < 0)
    return -1;

  if(s->master)
  {
     union semun u;
     u.val = 1;

    if(semctl(s->id, 0, SETVAL, u) < 0)
      return -1;  
  }

  return 0;
}

int semaphore_lock(sema_t *s)
{
  struct sembuf p = {0, -1, SEM_UNDO};

  if(s == NULL)
    return -1;

  if(semop(s->id, &p, 1) < 0)
    return -1;

  s->state = LOCKED;

  return 0;
}

int semaphore_unlock(sema_t *s)
{
  struct sembuf v = {0, 1, SEM_UNDO};
  
  if(s == NULL)
    return -1;

  if(semop(s->id, &v, 1) < 0)
    return -1;

  s->state = UNLOCKED;

  return 0;
}

int semaphore_destroy(sema_t *s)
{
  union semun sem_union;
  if(s == NULL)
    return -1;

  if(semctl(s->id, 0, IPC_RMID, sem_union) < 0)
    return -1;

  s->state = UNLOCKED;

  return 0;

}
