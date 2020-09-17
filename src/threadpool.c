/**
  ******************************************************************************
  * File Name          : threadpool.c
  * Description        : This file provides the code about threadpool.
  ******************************************************************************
  * @attention

  *
  ******************************************************************************
  */
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "threadpool.h"
#include "debug.h"

pthread_mutex_t pmutex;
pthread_cond_t  pcond;

void *Thread_Routine(void *arg)
{
    pthread_detach(pthread_self());
    struct timespec abstime;
    int flagTimeout = 0;
    int res = 0;
    pthread_t tid;
    tid = pthread_self();
    dprintf("Thread %d is starting\n", (int)tid);
    ThreadPool *pool = (ThreadPool *)arg;

    while(1)
    {   
        flagTimeout = 0;
        pthread_mutex_lock(&pmutex);
        pool->idle++;
        while(pool->begin == NULL && !pool->quit)
        {   
            dprintf("Thread %d is waiting\n", (int)tid);
            clock_gettime(CLOCK_REALTIME, &abstime);  
            abstime.tv_sec += 5;  //空闲进程等待600秒后关闭
            res = pthread_cond_timedwait(&pcond, &pmutex, &abstime);
            if(res == ETIMEDOUT)
            {   
                dprintf("Thread %d wait timed out\n", (int)tid);
                flagTimeout = 1;
                break;
            }   
        }   
        pool->idle--;
        if(pool->begin != NULL)
        {   
            Task *t = pool->begin;
            pool->begin = t->next;
            pthread_mutex_unlock(&pmutex);
            t->run(t->arg);
            free(t);
            pthread_mutex_lock(&pmutex);
        }   
        if(pool->quit && pool->begin == NULL)
        {
            pool->counter--;
            if(pool->counter == 0)
            {
                pthread_cond_signal(&pcond);
            }
            pthread_mutex_unlock(&pmutex);
            break;
        }
        if(flagTimeout == 1)
        {
            pool->counter--;
            pthread_mutex_unlock(&pmutex);
            break;
        }
        pthread_mutex_unlock(&pmutex);
    }
    dprintf("Thread %d is exiting\n", (int)tid);
    pthread_exit(0);
    return NULL;
}

int Threadpool_Init(ThreadPool *pool, int idleThreads, int maxThreads)
{
    int i;
    int res = 0;

    pool->begin       = NULL;
    pool->last        = NULL;
    pool->counter     = 0;
    pool->idle        = 0;
    pool->maxThreads  = maxThreads;
    pool->quit        = 0;

    res = pthread_mutex_init(&pmutex, NULL);
    if(res != 0)
    {
        dprintf("pthread_mutex_init failed\n");
        return res;
    }
    res = pthread_cond_init(&pcond, NULL);
    if(res != 0)
    {
        dprintf("pthread_cond_init failed\n");
        return res;
    }
    for(i = 0; i < idleThreads; i++)
    {
        pthread_t pid;
        pthread_create(&pid, NULL, Thread_Routine, pool);
        pool->counter++;
    }
    return res;
}

int Threadpool_Add(ThreadPool *pool, void *(*run)(void *arg), void *arg)
{
    Task *task = (Task *)malloc(sizeof(Task));
    task->run  = run;
    task->arg  = arg;
    task->next = NULL;

    pthread_mutex_lock(&pmutex);
    dprintf("idle thread is %d \n", pool->idle);
    dprintf("counter thread is %d \n", pool->counter);
    if(pool->begin == NULL)
    {
        pool->begin = task;
    }        
    else    
    {
        pool->last->next = task;
    }
    pool->last = task;
    if(pool->idle > 0)
    {
        pthread_cond_signal(&pcond);
    }
    else if(pool->counter < pool->maxThreads)
    {
        pthread_t tid;
        pthread_create(&tid, NULL, Thread_Routine, pool);
        pool->counter++;
    }
    else
    {
        pthread_mutex_unlock(&pmutex);
        return -1;
    }
    pthread_mutex_unlock(&pmutex);
    return 0;
}

void Threadpool_Destroy(ThreadPool *pool)
{
    if(pool->quit)
    {
        return;
    }
    pthread_mutex_lock(&pmutex); 
    pool->quit = 1;
    if(pool->counter > 0)
    {
        if(pool->idle > 0)
        {
            pthread_cond_broadcast(&pcond);
        }
        while(pool->counter)
        {
            pthread_cond_wait(&pcond, &pmutex);
        }
    }
    pthread_mutex_unlock(&pmutex);
    pthread_mutex_destroy(&pmutex);
    pthread_cond_destroy(&pcond);
}           





/************************ZXDQ *****END OF FILE****/




