/**
  ******************************************************************************
  * File Name          : tcp.c
  * Description        : This file provides the code about tcp server listen.
  ******************************************************************************
  * @attention
  
  *
  ******************************************************************************
  */
#include "tcp.h"
#include <pthread.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <signal.h>
#include <stdlib.h>
#include "sha1.h"
#include "base64.h"
#include "cJSON.h"
#include "debug.h"

#define TCP_PACK_LENGTH          65550
#define SEC_KEY_LENGTH           256
#define WEBSOCKET_PACK_LENGTH    65535

/*根据信号的默认处理规则SIGPIPE信号的默认执行动作是terminate(终止、退出),所以进程会退出。若不想客户端退出，需要把 SIGPIPE默认执行动作屏蔽。
*/
void handle_pipe(int sig)
{
    ;
}
void Change_SIGPIPE(void)
{
    struct sigaction action;
    action.sa_handler = handle_pipe;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGPIPE, &action, NULL);
}
/**
  ******************************************************************************
  * Tcp write.
  ******************************************************************************
  */
int nwrite(int fd, char *buf, int nbytes)
{
    int len = 0;
    char *ptr = buf;
    int left = nbytes;
    int error;
    while(left)
    {
        len = write(fd, ptr, left);
        if(len < 0)
        {
            return -1;    //真正的错误
        }
        if(len == 0)
        {
            break;
        }
        left -= len;
        ptr += len;
    }
    return 0;
}
/**
  ******************************************************************************
  * How to send websocket data.
  ******************************************************************************
  */
int WebSocket_Data_Send(int fd, unsigned char *sendData, unsigned long sendLength)
{
    unsigned char sendWsp[TCP_PACK_LENGTH] = {0};
    unsigned long i;
    int sendNum = sendLength/WEBSOCKET_PACK_LENGTH;
    
    if(sendNum > 1)
    {
        for(i=0; i<sendNum; i++)
        {
            if(i == 0)
                sendWsp[0] = 0x01;
            else
                sendWsp[0] = 0x00;
            sendWsp[1] = 0x7E;
            sendWsp[2] = (WEBSOCKET_PACK_LENGTH >> 8) & 0xFF;
            sendWsp[3] = WEBSOCKET_PACK_LENGTH & 0xFF;
            memcpy(sendWsp + 4, sendData+i*WEBSOCKET_PACK_LENGTH, WEBSOCKET_PACK_LENGTH);
            if(nwrite(fd, (char *)sendWsp, WEBSOCKET_PACK_LENGTH + 4) != 0)
            {
                return -1;
            }
            memset(sendWsp, 0, TCP_PACK_LENGTH);
            sendLength = sendLength - WEBSOCKET_PACK_LENGTH;
        }
        if(sendLength > 125)
        {
            sendWsp[0] = 0x80;
            sendWsp[1] = 0x7E;
            sendWsp[2] = (sendLength >> 8) & 0xFF;
            sendWsp[3] = sendLength & 0xFF;
            memcpy(sendWsp + 4, sendData+i*WEBSOCKET_PACK_LENGTH, sendLength);
            if(nwrite(fd, (char *)sendWsp, sendLength + 4) != 0)
            {
                return -1;
            }
            memset(sendWsp, 0, TCP_PACK_LENGTH);
        }
        else
        {
            sendWsp[0] = 0x80;
            sendWsp[1] = sendLength;
            memcpy(sendWsp + 2, sendData+i*WEBSOCKET_PACK_LENGTH, sendLength);
            if(nwrite(fd, (char *)sendWsp, sendLength + 2) !=0)
            {
                return -1;
            }
            memset(sendWsp, 0, TCP_PACK_LENGTH);
        }
        
    }
    else 
    {
        if(sendLength > 125)
        {
            sendWsp[0] = 0x81;
            sendWsp[1] = 0x7E;
            sendWsp[2] = (sendLength >> 8) & 0xFF;
            sendWsp[3] = sendLength & 0xFF;
            memcpy(sendWsp + 4, sendData, sendLength);
            if(nwrite(fd, (char *)sendWsp, sendLength + 4) != 0)
            {
                return -1;
            }
            memset(sendWsp, 0, TCP_PACK_LENGTH);
        }
        else
        {
            sendWsp[0] = 0x81;
            sendWsp[1] = sendLength;
            memcpy(sendWsp + 2, sendData, sendLength);
            if(nwrite(fd, (char *)sendWsp, sendLength + 2) !=0)
            {
                return -1;
            }
            memset(sendWsp, 0, TCP_PACK_LENGTH);
        }
    }
    return 0;
}
/**
  ******************************************************************************
  * WebSocket send ping.
  ******************************************************************************
  */
int WebSocket_Ping_Send(int fd)
{
    unsigned char sendWsp[100] = {0};
    sendWsp[0] = 0x89;
    sendWsp[1] = 0x00;
    if(nwrite(fd, (char *)sendWsp, 2) != 0)
    {
        return -1;
    }
    return 0;
}
/**
  ******************************************************************************
  * Parse websocket.
  ******************************************************************************
  */
int Websocket_Data_Process(unsigned long length, int fd, WebSocketPacket *wsp)
{
    unsigned long i = 0;
    int res = 0;
    cJSON *json;
    IR_Temp_Data ir;
    char sendData[IR_TEMP_DATA_LENGTH] = {0};
    unsigned long sendLength = 0;
    char errorData[40] = {0};
    if(wsp->opcode == 0x0a)
    {
        return WebSocket_Ping_Send(fd);
    }
    if(wsp->mask == 0x80)
    {
        for(i = 0; i < length; i++)
        {
            wsp->payloadData[i] = wsp->payloadData[i] ^ wsp->maskKey[i%4];
        }
    }
    json = cJSON_Parse(wsp->payloadData);
    if(json == NULL)
    {
        dprintf("json is NULL\n");
        res = 401;
    }
    if(res == 0)
        res = Ir_Data_Process(json, &ir, &sendLength, sendData, wsp->payloadData);

    if(res != 0)
    {
        sprintf(errorData, "{\"error\":%d}", res);
        sendLength = strlen(errorData);
        memset(sendData, '\0', IR_TEMP_DATA_LENGTH);
        memcpy(sendData, errorData, sendLength);
    }
    if(WebSocket_Data_Send(fd, sendData, sendLength) != 0)
    {
        dprintf("Write data error\n");
        res = -1;
    }
    if(json != NULL)
        cJSON_Delete(json); 
    return res;
}

/**
  ******************************************************************************
  * Get secKey.
  ******************************************************************************
  */
bool Get_SecKey(unsigned char *buf, unsigned char *key, int *length)
{
    unsigned char *keyBegin;
    unsigned char *flag = "Sec-WebSocket-Key: ";
    int i = 0;
    int bufLen = 0;
    int keyLen = 0;;

    if(!buf)
        return false;
    keyBegin = strstr(buf, flag);
    if(!keyBegin)
        return false;
    keyBegin += strlen(flag);
    bufLen = strlen(buf);
    for(i=0; i<bufLen; i++)
    {
        if(keyBegin[i]==0x0A||keyBegin[i]==0x0D)
        {
            break;
        }
        key[i] = keyBegin[i];
        keyLen++;
    }
    *length = keyLen;
    return true;
}
/**
  ******************************************************************************
  * Sha1 encode.
  ******************************************************************************
  */
void Sha1_Hash(unsigned char *src, unsigned char *res)
{
    SHA1Context sha;
    int i;
    char temp[4] = {0};
    unsigned char Message_Digest[SHA1HashSize];
    SHA1Reset(&sha);
    SHA1Input(&sha, src, strlen(src));
    SHA1Result(&sha, Message_Digest);
    for(i = 0; i < 20 ; ++i)
    {
        sprintf(temp,"%02X", Message_Digest[i]);
        strcat(res, temp);
    }
}
/**
  ******************************************************************************
  * Shake hand of websocket.
  ******************************************************************************
  */
bool Shake_Hand(unsigned char *buff, int length, int fd)
{
    bool res = false;
    int keyLen = 0;
    int num = 0;
    int i;
    const char *GUID="258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    unsigned char key[SEC_KEY_LENGTH] = {0};
    unsigned char shaDataTemp[SEC_KEY_LENGTH] = {0};
    unsigned char shaData[SEC_KEY_LENGTH] = {0};
    unsigned char serverKey[SEC_KEY_LENGTH] = {0};
    unsigned char responseHeader[TCP_PACK_LENGTH] = {0};
    
    if(!buff)
        return false;
    res = Get_SecKey(buff, key, &keyLen);
    if(res == false)
        return false;
    strcat(key, GUID);
    Sha1_Hash(key, shaDataTemp);
    num = strlen(shaDataTemp);
    for(i=0 ; i<num; i+=2)
    {      
        shaData[i/2] = Htoi(shaDataTemp, i, 2);    
    } 
    Base64_Encode(shaData, serverKey, strlen(shaData)); 
    sprintf(responseHeader, "HTTP/1.1 101 Switching Protocols\r\n");
    sprintf(responseHeader, "%sUpgrade: websocket\r\n", responseHeader);
    sprintf(responseHeader, "%sConnection: Upgrade\r\n", responseHeader);
    sprintf(responseHeader, "%sSec-WebSocket-Accept: %s\r\n\r\n", responseHeader, serverKey);
 
    dprintf("Response Header:\n %s\n",responseHeader);
 
    write(fd, responseHeader, strlen(responseHeader));
    return true;
}
/**
  ******************************************************************************
  * Thread of websocket.
  ******************************************************************************
  */
void* Pthread_Websocket(void *arg)
{
    pthread_detach(pthread_self());
    
    int socketfd;
    int i;
    int tcpRevNum = 0;
    int tcpRevCounter = 0;
    bool isConnected = false;
    int  isConnectShouldClose = 0;
    char revBuff[TCP_PACK_LENGTH] = {0};
    char buff_ok[TCP_PACK_LENGTH] = {0};
    WebSocketPacket webSocket_Packet;
    unsigned long websocketDataCounter = 0;
    unsigned long payloadLengthH = 0;
    unsigned long payloadLengthL = 0;
    socketfd = *((int*)arg);
    while(1)
    {
        memset(revBuff, 0, TCP_PACK_LENGTH);
        tcpRevNum = read(socketfd, revBuff, TCP_PACK_LENGTH); 
        if(tcpRevNum <= 0)
            break;
        if(isConnected == false)
        {
            for(i=0; i<tcpRevNum; i++)
            {
                buff_ok[tcpRevCounter] = revBuff[i];
                switch(tcpRevCounter)
                {
                    case 0:
                    if(buff_ok[tcpRevCounter] == 'G')
                        tcpRevCounter++;
                    break;
                    case 1:
                    if(buff_ok[tcpRevCounter] == 'E')
                        tcpRevCounter++;
                    else
                        tcpRevCounter = 0;
                    
                    break;
                    case 2:
                    if(buff_ok[tcpRevCounter] == 'T')
                        tcpRevCounter++;
                    else
                        tcpRevCounter = 0;
                    break;
                    default:
                    if(tcpRevCounter > 2 && tcpRevCounter < 5)
                        tcpRevCounter++;
                    else if(tcpRevCounter > 4)
                    {
                        if(buff_ok[tcpRevCounter] == 0x0A && buff_ok[tcpRevCounter - 1] == 0x0D
                            && buff_ok[tcpRevCounter - 2] == 0x0A && buff_ok[tcpRevCounter - 3] == 0x0D)
                        {
                            if(Shake_Hand(buff_ok, tcpRevCounter, socketfd) == true)
                            {
                                dprintf("Shake_Hand ok!\n");
                                isConnected = true;
                                memset(buff_ok, 0, TCP_PACK_LENGTH);
                            }
                            tcpRevCounter = 0;
                        } 
                        else
                            tcpRevCounter++;
                    }
                    break;
                }
            }    
            continue;
        }
        else
        {
            for(i=0; i<tcpRevNum; i++)
            {
                buff_ok[tcpRevCounter] = revBuff[i];
                switch(tcpRevCounter)
                {
                    case 0:
                    webSocket_Packet.fin = buff_ok[tcpRevCounter] & 0x80;
                    webSocket_Packet.opcode = buff_ok[tcpRevCounter] & 0x0F;
                    tcpRevCounter++;
                    break;
                    case 1:
                    webSocket_Packet.mask = buff_ok[tcpRevCounter] & 0x80;
                    webSocket_Packet.headerLength = 2;
                    if(webSocket_Packet.mask == 0x80)
                        webSocket_Packet.maskKeyLength = 4;
                    else
                        webSocket_Packet.maskKeyLength = 0;

                    webSocket_Packet.payloadLength = buff_ok[tcpRevCounter] & 0x7F;
                    tcpRevCounter++;
                    break;
                    default:
                    if(tcpRevCounter < webSocket_Packet.payloadLength + webSocket_Packet.headerLength + webSocket_Packet.maskKeyLength -1)
                    {
                        tcpRevCounter++;
                        if(tcpRevCounter == (webSocket_Packet.headerLength + 1) && webSocket_Packet.payloadLength == 126)
                        {
                            webSocket_Packet.payloadLength = (buff_ok[webSocket_Packet.headerLength] << 8) + buff_ok[webSocket_Packet.headerLength + 1];
                            webSocket_Packet.headerLength = 4;
                        }
                        if(tcpRevCounter == (webSocket_Packet.headerLength + 7) && webSocket_Packet.payloadLength == 127)
                        {
                            payloadLengthH = (buff_ok[webSocket_Packet.headerLength] << 24) + (buff_ok[webSocket_Packet.headerLength + 1] << 16) 
                                     + (buff_ok[webSocket_Packet.headerLength + 2] << 8) + buff_ok[webSocket_Packet.headerLength + 3];
                            payloadLengthL = (buff_ok[webSocket_Packet.headerLength + 4] << 24) + (buff_ok[webSocket_Packet.headerLength + 5] << 16) 
                                     + (buff_ok[webSocket_Packet.headerLength + 6] << 8)  + buff_ok[webSocket_Packet.headerLength + 7];
                            webSocket_Packet.payloadLength = (payloadLengthH << 32) + payloadLengthL;
                            webSocket_Packet.headerLength = 10;
                        }
                    }
                    else //a websocket slice end
                    {
                        if(webSocket_Packet.mask == 0x80)
                        {
                            webSocket_Packet.maskKey[0] = buff_ok[webSocket_Packet.headerLength];
                            webSocket_Packet.maskKey[1] = buff_ok[webSocket_Packet.headerLength + 1];
                            webSocket_Packet.maskKey[2] = buff_ok[webSocket_Packet.headerLength + 2];
                            webSocket_Packet.maskKey[3] = buff_ok[webSocket_Packet.headerLength + 3];
                        }
                        if(websocketDataCounter < IR_TEMP_DATA_LENGTH)
                            memcpy(webSocket_Packet.payloadData+websocketDataCounter, buff_ok+webSocket_Packet.headerLength+webSocket_Packet.maskKeyLength, webSocket_Packet.payloadLength);
                        else
                            isConnectShouldClose = 1;
                        websocketDataCounter += webSocket_Packet.payloadLength;
                        if(webSocket_Packet.fin == 0x80)   //recive all slice
                        {
                            switch(webSocket_Packet.opcode)
                            {
                                case 0x00:
                                case 0x01:
                                case 0x02:
                                case 0x0a:
                                if(Websocket_Data_Process(websocketDataCounter, socketfd, &webSocket_Packet) == -1)
                                    isConnectShouldClose = 1;
                                break;
                                case 0x08:
                                isConnectShouldClose = 1;
                                break;
                                case 0x09:
                                ;
                                break;
                                default:
                                isConnectShouldClose = 1;
                            }
                            memset(webSocket_Packet.payloadData, 0, IR_TEMP_DATA_LENGTH);
                            websocketDataCounter = 0;  
                        }
                        memset(buff_ok, 0, TCP_PACK_LENGTH);
                        tcpRevCounter  = 0; 
                    }
                }
            }
            
        }
        if(isConnectShouldClose == 1)
        {
            break;
        }

    }
    dprintf("socketfd %d: Pthread_Websocket close!\n", socketfd);
    close(socketfd);
    pthread_exit(0);
}
/**
  ******************************************************************************
  * Tcp listen.
  ******************************************************************************
  */
int Tcp_Listen(const char *ip, int port)
{
    int err, connfd, i;
    pthread_t pthreadWebsocket;
    void *ret;
    int listenfd;
    int opt = SO_REUSEADDR;
    struct sockaddr_in local;
    struct sockaddr_in client_addr;
    socklen_t sock_client_size;
    sock_client_size = sizeof(client_addr);
    char cli_ip[16] = "";  
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd < 0)
    {
        dprintf("socket failed\n");
        return -1;
    }
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    local.sin_family = AF_INET;
    local.sin_port   = htons(port);
    local.sin_addr.s_addr = inet_addr(ip);
    if(bind(listenfd, (struct sockaddr *)&local, sizeof(local)) < 0)
    {
        dprintf("bind failed\n");
        close(listenfd);
        return -1;
    }
    if(listen(listenfd, 5) < 0)
    {
        dprintf("listen failed\n");
        close(listenfd);
        return -1;;
    }
    dprintf("Waiting client connect...\n"); 
    while(1)
    {
        //获得一个已经建立的连接     
        connfd = accept(listenfd, (struct sockaddr*)&client_addr, &sock_client_size);                                
        if(connfd < 0)  
        {  
            dprintf("accept no\n");  
            continue;  
        }   
        // 打印客户端的 ip 和端口  
        inet_ntop(AF_INET, &client_addr.sin_addr, cli_ip, 16);  
        dprintf("----------------------------------------------\n");  
        dprintf("client ip=%s   port=%d connfd=%d\n", cli_ip, ntohs(client_addr.sin_port), connfd);  
        if(connfd > 0)  
        { 
            //由于同一个进程内的所有线程共享内存和变量，因此在传递参数时需作特殊处理，值传递。  
            err = pthread_create(&pthreadWebsocket, NULL, Pthread_Websocket, (void *)&connfd);  //创建线程  
            if (err != 0) 
            {
                dprintf("pthread_create error\n");
                close(connfd);
                break;
            }
        }  
    }
    close(listenfd);
    return -1;
}

