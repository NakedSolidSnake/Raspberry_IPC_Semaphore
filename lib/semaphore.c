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

bool Semaphore_Init(Semaphore_t *semaphore)
{
  bool status = false;

  do 
  {
    if(!semaphore)
      break;

    semaphore->id = semget((key_t) semaphore->key, semaphore->sema_count, 0666 | IPC_CREAT);
    if(semaphore->id < 0)
      break;

    if (semaphore->type == master)
    {
      union semun u;
      u.val = 1;

      if (semctl(semaphore->id, 0, SETVAL, u) < 0)
        break;
    }

    status = true;

  } while(false);

  return status;
}

bool Semaphore_Lock(Semaphore_t *semaphore)
{
  bool status = false;
  struct sembuf p = {0, -1, SEM_UNDO};

  do
  {
    if(!semaphore)
      break;

    if(semop(semaphore->id, &p, 1) < 0)
      break;

    semaphore->state = locked;
    status = true;
  } while(false);

  return status;
}

bool Semaphore_Unlock(Semaphore_t *semaphore)
{
  bool status = false;
  struct sembuf v = {0, 1, SEM_UNDO};

  do
  {
    if(!semaphore)
      break;

    if(semop(semaphore->id, &v, 1) < 0)
      break;

    semaphore->state = unlocked;
    status = true;

  } while(false);

  return status;
}
bool Semaphore_Destroy(Semaphore_t *semaphore)
{
  union semun sem_union;
  bool status = false;

  do 
  {
    if(!semaphore)
      break;

    if(semctl(semaphore->id, 0, IPC_RMID, sem_union) < 0)
      break;

    semaphore->state = unlocked;
    status = true;
  } while(false);

  return status;
}
