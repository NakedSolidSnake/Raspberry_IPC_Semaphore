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