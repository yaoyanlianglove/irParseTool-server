/**
  ******************************************************************************
  * File Name          : ir_process.h
  * Description        : This file provides the code to process the data of IR.
  ******************************************************************************
  * @attention
  
  *
  ******************************************************************************
  */
#ifndef _IR_PROCESS_H_ 
#define _IR_PROCESS_H_
#include <stddef.h>
#include "cJSON.h"

#define IR_TEMP_DATA_LENGTH  1638500

typedef struct 
{
    int height;
    int width;
    float temp[640*480+1];
    float maxTemp;
    float minTemp;
    float maxScale;
    float minScale;
    int type;
    float maxTempOfArea;
    float minTempOfArea;
    float avgTempOfArea;
    int x1;
    int x2;
    int y1;
    int y2;
}IR_Temp_Data;

int Create_Color_Plette(void);
int Ir_Data_Process(cJSON *json, IR_Temp_Data *ir, unsigned long *sendLength, unsigned char *sendData, unsigned char *image);

#endif
/************************ZXDQ *****END OF FILE****/
