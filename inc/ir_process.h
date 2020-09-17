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


#define SUPPORT_MAX_IMAGE_WIDTH      1024
#define SUPPORT_MAX_IMAGE_HEIGHT     768

#define SUPPORT_MAX_PLETTE_WIDTH     50
#define SUPPORT_MAX_PLETTE_HEIGHT    1024

#define IR_TEMP_DATA_LENGTH  (SUPPORT_MAX_IMAGE_WIDTH*SUPPORT_MAX_IMAGE_HEIGHT*4 + 100)

typedef struct
{
    int height;
    int width;
    float *temp;
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
    int code;
}IR_Temp_Data;

int Create_Color_Plette(void);
int Ir_Data_Process(cJSON *json, IR_Temp_Data *ir, unsigned long *sendLength, unsigned char *sendData, unsigned char *image);

#endif
/************************ZXDQ *****END OF FILE****/
