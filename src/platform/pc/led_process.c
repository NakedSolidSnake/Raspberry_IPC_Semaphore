#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h>
#include <syslog.h>
#include <led_interface.h>

bool Init(void *object);
bool Set(void *object, uint8_t state);

int main(int argc, char *argv[])
{   

    LED_Interface led_interface = 
    {
        .Init = Init,
        .Set = Set
    };

    Semaphore_t semaphore = 
    {
        .key = 1234,
        .sema_count = 2,
        .state = locked,
        .type = slave
    };

    LED_Run(NULL, &semaphore, &led_interface);
    
    return 0;
}

bool Init(void *object)
{
    (void)object; 
    return true;
}

bool Set(void *object, uint8_t state)
{
    (void)object;    
    openlog("LED SEMAPHORE", LOG_PID | LOG_CONS , LOG_USER);
    syslog(LOG_INFO, "LED Status: %s", state ? "On": "Off");
    closelog(); 
    return true;
}
