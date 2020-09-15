/**
  ******************************************************************************
  * File Name          : threadpool.h
  * Description        : This file provides the code about threadpool.
  ******************************************************************************
  * @attention

  *
  ******************************************************************************
  */


#ifndef _THREAD_POOL_H_                                                                                                                                                                                                                       
#define _THREAD_POOL_H_

#define MAX_THREADS  10
#define IDLE_NUM     2

typedef struct Task
{
    void *(*run)(void *args);  
    void *arg;              
    struct Task *next;      
}Task;


//下面是线程池结构体
typedef struct ThreadPool
{  
    Task *begin;       
    Task *last;        
    int counter;         
    int idle;            
    int maxThreads;   
    int quit;         
}ThreadPool;


int Threadpool_Init(ThreadPool *pool, int idleThreads, int maxThreads);

int Threadpool_Add(ThreadPool *pool, void *(*run)(void *arg), void *arg);

void Threadpool_Destroy(ThreadPool *pool);
#endif
/************************ZXDQ *****END OF FILE****/

