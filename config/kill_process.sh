#!/bin/bash

if [ `pgrep button_process` > 0 ]; then
    echo "Killing button_process"
    kill `pgrep button_process`
fi

if [ `pgrep led_process` > 0 ]; then    
    echo "Killing led_process"
    kill `pgrep led_process`
fi

ID=$(ipcs -s | grep `printf 0x"%08x\n" 1234` | awk -F' ' '{print $2}')
if [ $ID > 0 ]; then 
    echo "Removing semaphore ID=${ID}"
    ipcrm -s $ID
fi
