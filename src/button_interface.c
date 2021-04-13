#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <button_interface.h>

#define _1ms 1000

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
            while (true)
            {
                if (!button->Read(object))
                {
                    usleep(_1ms * 100);
                    break;
                }
                else
                    usleep(_1ms);
            }

            Semaphore_Unlock(semaphore);
        }
    }

    Semaphore_Destroy(semaphore);
    return false;
}
