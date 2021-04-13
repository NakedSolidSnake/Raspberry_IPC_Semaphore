#ifndef __SEMAPHORE_H
#define __SEMAPHORE_H

#include <stdbool.h>

typedef enum 
{
  unlocked,
  locked
} Semaphore_State;

typedef enum 
{
  slave,
  master
} Semaphore_Type;

typedef struct 
{
  int id;
  int sema_count;
  int key;
  Semaphore_State state;
  Semaphore_Type type;
} Semaphore_t;

bool Semaphore_Init(Semaphore_t *semaphore);
bool Semaphore_Lock(Semaphore_t *semaphore);
bool Semaphore_Unlock(Semaphore_t *semaphore);
bool Semaphore_Destroy(Semaphore_t *semaphore);

#endif
