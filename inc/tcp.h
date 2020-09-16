/**
  ******************************************************************************
  * File Name          : tcp.h
  * Description        : This file provides the code about tcp server listen.
  ******************************************************************************
  * @attention
  
  *
  ******************************************************************************
  */
#ifndef _TCP_H_ 
#define _TCP_H_
#include <sys/socket.h>
#include <stdint.h>
#include "ir_process.h"

void Change_SIGPIPE(void);
int Tcp_Listen(const char *ip, int port);
typedef struct 
{
	unsigned char fin;
	unsigned char opcode;
	unsigned char mask;
	unsigned char maskKey[4];
	unsigned long payloadLength;
	int headerLength;
	int maskKeyLength;
	unsigned char payloadData[IR_TEMP_DATA_LENGTH];
    unsigned char sendData[IR_TEMP_DATA_LENGTH];
    IR_Temp_Data  ir;
}WebSocketPacket;

#endif

/************************ZXDQ *****END OF FILE****/

