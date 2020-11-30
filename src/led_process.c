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
