#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <button_interface.h>

#define _1ms 1000

static void wait_press(void *object, Button_Interface *button)
{
    while (true)
    {
        if (!button->Read(object))
        {
            usleep(_1ms * 100);
            break;
        }
        else
        {
            usleep(_1ms);
        }
    }
}

bool Button_Run(void *object, Semaphore_t *semaphore, Button_Interface *button)
{
    if (button->Init(object) == false)
        return false;

    if(Semaphore_Init(semaphore) == false)
        return false;

    while(true)
    {
        if(Semaphore_Lock(semaphore) == true)
        {
            wait_press(object, button);
            Semaphore_Unlock(semaphore);
        }
    }

    Semaphore_Destroy(semaphore);
    return false;
}
