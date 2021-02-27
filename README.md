<p align="center">
  <img src="https://cdn.pixabay.com/photo/2012/04/10/23/30/semaphore-27029_960_720.png">
</p>
            
# Semaphore
## Introdução
## Implementação
### Biblioteca
#### semaphore.h
```c
#ifndef __SEMAPHORE_H
#define __SEMAPHORE_H

#define LOCKED      1
#define UNLOCKED    0

#define SLAVE       0
#define MASTER      1

typedef struct sema 
{
  int id;
  int sema_count;
  int state;
  int master;
}sema_t;

int semaphore_init(sema_t *s, int key);

int semaphore_lock(sema_t *s);

int semaphore_unlock(sema_t *s);

int semaphore_destroy(sema_t *s);

#endif

```
#### semaphore.c
```c
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

```
### launch_processes.c
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int pid_button, pid_led;
    int button_status, led_status;

    pid_button = fork();

    if(pid_button == 0)
    {
        //start button process
        char *args[] = {"./button_process", NULL};
        button_status = execvp(args[0], args);
        printf("Error to start button process, status = %d\n", button_status);
        abort();
    }   

    pid_led = fork();

    if(pid_led == 0)
    {
        //Start led process
        char *args[] = {"./led_process", NULL};
        led_status = execvp(args[0], args);
        printf("Error to start led process, status = %d\n", led_status);
        abort();
    }

    return EXIT_SUCCESS;
}
```
### button_process.c
```c
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore/semaphore.h>
#include <signal.h>
#include <button.h>

#define _1ms 1000

static void closeApp(int s);
static int end = 0;

int main(int argc, char *argv[])
{

    static Button_t button = {
        .gpio.pin = 7,
        .gpio.eMode = eModeInput,
        .ePullMode = ePullModePullUp,
        .eIntEdge = eIntEdgeFalling,
        .cb = NULL};

    sema_t sema = {
        .id = -1,
        .sema_count = 2,
        .state = LOCKED,
        .master = MASTER};

    if (Button_init(&button))
        return EXIT_FAILURE;

    signal(SIGINT, closeApp);

    semaphore_init(&sema, 1234);

    while (end == 0)
    {
        if (semaphore_lock(&sema) == 0)
        {
            while (1)
            {
                if (!Button_read(&button))
                {
                    usleep(_1ms * 40);
                    while (!Button_read(&button))
                        ;
                    usleep(_1ms * 40);
                    break;
                }
                else
                {
                    usleep(_1ms);
                }
            }
            semaphore_unlock(&sema);
        }
    }

    semaphore_destroy(&sema);
    exit(EXIT_SUCCESS);
}

static void closeApp(int s)
{
    end = 1;
}
```
### led_process.c
```c
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore/semaphore.h>
#include <led.h>

int main(int argc, char *argv[])
{
    int state = 0;
    sema_t sema = {
        .id = -1,
        .sema_count = 2,
        .state = LOCKED,
        .master = SLAVE
    };

     LED_t led =
    {
        .gpio.pin = 0,
        .gpio.eMode = eModeOutput
    };

    if (LED_init(&led))
        return EXIT_FAILURE;

    if(semaphore_init(&sema, 1234))
        return EXIT_FAILURE;

    while(1)
    {
        if (semaphore_lock(&sema) == 0)
        {            
            LED_set(&led, (eState_t)state);
            state ^= 0x01; 
            semaphore_unlock(&sema);
        }
        else
        {
            usleep(100);    
        }
    }

    exit(EXIT_SUCCESS);
}

```
## Conclusão
