#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <led_interface.h>

#define _1ms 1000

bool LED_Run(void *object, Semaphore_t *semaphore, LED_Interface *led)
{
    int state = 0;
    if(led->Init(object) == false)
        return false;

    if(Semaphore_Init(semaphore) == false)
        return false;

    while(true)
    {
        if(Semaphore_Lock(semaphore) == true)
        {
            led->Set(object, state);
            state ^= 0x01;
            Semaphore_Unlock(semaphore);
        }
        else 
            usleep(_1ms);
    }
}
