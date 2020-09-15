/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : 
  ******************************************************************************
  * @attention
  
  *
  ******************************************************************************
  */

#include <stdio.h>
#include <stdlib.h>
#include "main.h" 
#include "tcp.h"
#include "ir_process.h"
#include "debug.h"
/**
  ******************************************************************************
  * Main.
  ******************************************************************************
  */
int main(int argc, char* argv[])
{
    dprintf("IR_WEBSOCKET\n");
    int port = 0;
    if(argc != 3)
    {
        dprintf("Please input IP and port...\n");
        return 0;
    }
    port = atoi(argv[2]);
    if(port<=0 || port>0xFFFF)
    {
        dprintf("Port(%d) is out of range(1-%d)\n", port, 0xFFFF);
        return 0;
    }
    if(Create_Color_Plette() != 0)
        return -1;
    Change_SIGPIPE();
    Tcp_Listen(argv[1], port);
    return 0;    
}