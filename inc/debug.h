/**
  ******************************************************************************
  * File Name          : debug.h
  * Description        : This file provides the code to redefine the printf 
                         function.
  ******************************************************************************
  * @attention
  
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __debug_H
#define __debug_H


#include <string.h>

#define DEBUG_OUT  
#ifdef DEBUG_OUT
    #define dprintf (void)printf
#else
    extern int dprintf_none(const char *format,...);
    #define dprintf (void)dprintf_none
#endif




#endif
/************************ZXDQ *****END OF FILE****/
  

