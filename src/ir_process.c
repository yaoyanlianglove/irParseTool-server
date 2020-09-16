/**
  ******************************************************************************
  * File Name          : ir_process.c
  * Description        : This file provides the code to process the data of IR.
  ******************************************************************************
  * @attention
  
  *
  ******************************************************************************
  */
#include "ir_process.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "base64.h"
#include "debug.h"

#define SUPPORT_MAX_PLETTE_WIDTH      50
#define SUPPORT_MAX_PLETTE_HEIGHT     1024
/**
  ******************************************************************************
  * Pixel RGB struct.
  ******************************************************************************
  */
typedef struct
{
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
}Pixel; 
/**
  ******************************************************************************
  * Plette struct.  后续要一直使用，所以不用动态分配
  ******************************************************************************
  */
typedef struct
{
    Pixel pix[SUPPORT_MAX_PLETTE_HEIGHT*SUPPORT_MAX_PLETTE_WIDTH];
    int width;
    int height;
    unsigned char pletteBuff[IR_TEMP_DATA_LENGTH];
}Plette; 

Plette plette;
/**
  ******************************************************************************
  * Get the RGB value of plette.
  ******************************************************************************
  */
int Create_Color_Plette(void)
{
    FILE *fp;
    int width;
    int height;
    unsigned char header[54];
    int jpgPletteLength, i;
    fp = fopen("/home/ir-palette.bmp", "rb");  
    if(fp == NULL)  
    {
        dprintf("IR-palette open failed.\n");
        return -1;
    }
    fread(header, 1, 54, fp);
    jpgPletteLength = ((header[49] << 24) + (header[48] << 16) + (header[47] << 8) +header[46]) * 4;
    width = (header[21] << 24) + (header[20] << 16) +  (header[19] << 8) + header[18];
    height = (header[25] << 24) + (header[24] << 16) +  (header[23] << 8) + header[22];
    dprintf("IR-palette width is %d.height is %d.\n", width, height);
    if(height > 0)
    {
        fread(plette.pletteBuff, 1, height*width*4, fp + jpgPletteLength);
        for(i=0; i<height; i++)
        {
            plette.pix[height - i].blue   = plette.pletteBuff[i*width*4];
            plette.pix[height - i].green  = plette.pletteBuff[i*width*4 + 1];
            plette.pix[height - i].red    = plette.pletteBuff[i*width*4 + 2];
            plette.pix[height - i].alpha  = plette.pletteBuff[i*width*4 + 3];
        }
    }
    else
    {
        height = -height;
        fread(plette.pletteBuff, 1, height*width*4, fp + jpgPletteLength);
        for(i=0; i<height; i++)
        {
            plette.pix[i].blue   = plette.pletteBuff[i*width*4];
            plette.pix[i].green  = plette.pletteBuff[i*width*4 + 1];
            plette.pix[i].red    = plette.pletteBuff[i*width*4 + 2];
            plette.pix[i].alpha  = plette.pletteBuff[i*width*4 + 3];
        }
    } 
    plette.height = height;
    plette.width  = width;
    fclose(fp);
    return 0;
}
/**
  ******************************************************************************
  * Convert tempreture to RGB value.
  ******************************************************************************
  */
int Get_Ir_Temp_Image(char *irFilePath, IR_Temp_Data *ir_data, unsigned char *image)
{
    FILE *fp;
    unsigned long i;
    ir_data->minTemp = 0.0;
    ir_data->maxTemp = 0.0;
    if(irFilePath == NULL)
    {
        dprintf("No irFilePath\n");
        return 404;
    }
    fp = fopen(irFilePath, "rb");  
    if(fp == NULL)  
    {
        dprintf("IR-temp %s open failed.\n", irFilePath);
        return 404;
    }
    unsigned long x, y;
    unsigned long imageLength = ir_data->width * ir_data->height * 4;
    fread(image, 1, imageLength, fp);
    for(i=0; i<imageLength/4; i++)
    {
        x = (image[i*4 + 3] << 24) + (image[i*4 + 2] << 16) + (image[i*4 + 1] << 8) + image[i*4];
        memcpy(&(ir_data->temp[i]), &x, 4);
        if(i == 0)
        {
            ir_data->minTemp = ir_data->temp[i];
            ir_data->maxTemp = ir_data->temp[i];
        }
        if(ir_data->temp[i] < ir_data->minTemp)
            ir_data->minTemp = ir_data->temp[i];
        if(ir_data->temp[i] > ir_data->maxTemp)
            ir_data->maxTemp = ir_data->temp[i];
    }
    for(i=0; i<imageLength/4; i++)
    {
        y = (ir_data->temp[i] - ir_data->minTemp) *plette.height*(ir_data->maxScale - ir_data->minScale)/(ir_data->maxTemp - ir_data->minTemp)
            + ir_data->minScale * plette.height;
        if(ir_data->type == 1)
        {
            image[i*4]     = plette.pix[plette.height - y].red   * 0.5;
            image[i*4 + 1] = plette.pix[plette.height - y].green * 0.5;
            image[i*4 + 2] = plette.pix[plette.height - y].blue  * 0.5;
            image[i*4 + 3] = 0;
        }
        else if(ir_data->type == 2)
        {
            image[i*4] = (plette.pix[plette.height - y].red   * 0.3  + 
                          plette.pix[plette.height - y].green * 0.59 + 
                          plette.pix[plette.height - y].blue  * 0.11) * 0.5;
            image[i*4 + 1] = image[i*4];
            image[i*4 + 2] = image[i*4];
            image[i*4 + 3] = 0;
        }
        else
        {
            fclose(fp);
            return 405;
        }
    }
    fclose(fp);
    return 0;
}
/**
  ******************************************************************************
  * Parse json of get-image.
  ******************************************************************************
  */
int Get_Ir_Temp_Image_Process(cJSON *json, IR_Temp_Data *ir, unsigned char *image, unsigned long *sendLength)
{
    int res = 0;
    cJSON *itemHeight, *itemWidth, *itemIrFilePath, *itemType, *itemMaxScale, *itemMinScale;
    itemWidth = cJSON_GetObjectItem(json, "width");
    if(itemWidth == NULL)
    {
        dprintf("itemWidth  is null\n");
        return 403;
    }
    if(itemWidth->type != cJSON_Number)
    {
        dprintf("itemWidth  is not number\n");
        return 403;
    }
    ir->width = itemWidth->valueint;
    if(ir->width > SUPPORT_MAX_IMAGE_WIDTH)
        return 405;
    itemHeight = cJSON_GetObjectItem(json, "height");
    if(itemHeight == NULL)
    {
        dprintf("itemHeight  is null\n");
        return 403;
    }
    if(itemHeight->type != cJSON_Number)
    {
        dprintf("itemHeight  is not number\n");
        return 403;
    }
    ir->height = itemHeight->valueint;
    if(ir->height > SUPPORT_MAX_IMAGE_HEIGHT)
        return 405;
    itemIrFilePath = cJSON_GetObjectItem(json, "path");
    if(itemIrFilePath == NULL)
    {
        dprintf("itemIrFilePath  is null\n");
        return 403;
    }

    itemType = cJSON_GetObjectItem(json, "type");
    if(itemType == NULL)
    {
        dprintf("itemType  is null\n");
        return 403;
    }
    if(itemType->type != cJSON_Number)
    {
        dprintf("itemType  is not number\n");
        return 403;
    }
    ir->type = itemType->valueint;


    itemMaxScale = cJSON_GetObjectItem(json, "maxScale");
    if(itemMaxScale == NULL)
    {
        dprintf("itemMaxScale  is null\n");
        return 403;
    }
    if(itemMaxScale->type != cJSON_Number)
    {
        dprintf("itemMaxScale  is not number\n");
        return 403;
    }
    ir->maxScale = (itemMaxScale->valueint)/100.0;
    if(itemMaxScale->valueint > 100)
        return 405;
    itemMinScale = cJSON_GetObjectItem(json, "minScale");
    if(itemMinScale == NULL)
    {
        dprintf("itemMinScale  is null\n");
        return 403;
    }
    if(itemMinScale->type != cJSON_Number)
    {
        dprintf("itemMinScale  is not number\n");
        return 403;
    }
    ir->minScale = (itemMinScale->valueint)/100.0;
    if(itemMinScale->valueint > 100)
        return 405;
    *sendLength = ir->width * ir->height * 4;
    res = Get_Ir_Temp_Image((char*)itemIrFilePath->valuestring, ir, image);

    return res;
}
/**
  ******************************************************************************
  * Get IR-Temp data in area, MAX, MIN, AVG.
  ******************************************************************************
  */
int Get_Ir_Temp_Data(IR_Temp_Data *ir)
{
    int x1, x2, y1, y2;
    float max, min, avg, fx;
    if(ir->x2 >= ir->x1 && ir->y2 >= ir->y1)
    {
        x1 = ir->x1;
        x2 = ir->x2;
        y1 = ir->y1;
        y2 = ir->y2;
    }
    else
    {
        x1 = ir->x2;
        x2 = ir->x1;
        y1 = ir->y2;
        y2 = ir->y1;
    }
    int i, j;
    max = ir->temp[x1 + y1 * ir->width];
    min = max;
    avg = max;
    for(i = 0; i < y2 - y1 + 1; i++)
    {
        for(j = 0; j < x2 - x1 + 1; j++)
        {
            fx = ir->temp[x1 + j + (y1 + i) * ir->width];
            if(fx > max)
                max = fx;
            if(fx < min)
                min = fx;
            avg = (fx + avg)/2;
        } 
    }
    ir->maxTempOfArea = max;
    ir->minTempOfArea = min;
    ir->avgTempOfArea = avg;
    return 0;
}
/**
  ******************************************************************************
  * Parse json of get-temp.
  ******************************************************************************
  */
int Get_Ir_Temp_Data_Process(cJSON *json, IR_Temp_Data *ir)
{
    int res = 0;
    cJSON *itemX1, *itemX2, *itemY1, *itemY2;
    itemX1 = cJSON_GetObjectItem(json, "x1");
    if(itemX1 == NULL)
    {
        dprintf("itemX1  is null\n");
        return 403;
    }
    if(itemX1->type != cJSON_Number)
    {
        dprintf("itemX1  is not number\n");
        return 403;
    }
    ir->x1 = itemX1->valueint;
    if(ir->x1 > ir->width)
        return 405;

    itemX2 = cJSON_GetObjectItem(json, "x2");
    if(itemX2 == NULL)
    {
        dprintf("itemX2  is null\n");
        return 403;
    }
    if(itemX2->type != cJSON_Number)
    {
        dprintf("itemX2  is not number\n");
        return 403;
    }
    ir->x2 = itemX2->valueint;
    if(ir->x2 > ir->width)
        return 405;

    itemY1 = cJSON_GetObjectItem(json, "y1");
    if(itemY1 == NULL)
    {
        dprintf("itemY1  is null\n");
        return 403;
    }
    if(itemY1->type != cJSON_Number)
    {
        dprintf("itemY1  is not number\n");
        return 403;
    }
    ir->y1 = itemY1->valueint;
    if(ir->y1 > ir->height)
        return 405;

    itemY2 = cJSON_GetObjectItem(json, "y2");
    if(itemY2 == NULL)
    {
        dprintf("itemY2  is null\n");
        return 403;
    }
    if(itemY2->type != cJSON_Number)
    {
        dprintf("itemY2  is not number\n");
        return 403;
    }
    ir->y2 = itemY2->valueint;
    if(ir->y2 > ir->height)
        return 405;
    
    res = Get_Ir_Temp_Data(ir);

    return res;
}
/**
  ******************************************************************************
  * Parse json.
  ******************************************************************************
  */
int Ir_Data_Process(cJSON *json, IR_Temp_Data *ir, unsigned long *sendLength, unsigned char *sendData, unsigned char *image)
{
    int res = 0;
    unsigned long i , j;
    int headerLength = 0;
    char header[100]={0};
    int listLength = cJSON_GetArraySize(json);
    dprintf("listLength  is  %d \n", listLength);
    if(listLength == 0)
        return 401;
    cJSON *itemCode;
    itemCode = cJSON_GetObjectItem(json, "code");
    if(itemCode == NULL)
    {
        dprintf("itemCode  is null\n");
        return 402;
    }
    if(itemCode->type != cJSON_Number)
    {
        dprintf("itemCode  is not number\n");
        return 402;
    }
    dprintf("code is %d\n", itemCode->valueint);
    switch(itemCode->valueint)
    {
        case 1:
        if(listLength == 7)
        {
            res = Get_Ir_Temp_Image_Process(json, ir, image, sendLength);
            if(res == 0)
            {
                memset(sendData, 0, IR_TEMP_DATA_LENGTH);
                sprintf(header, "{\"code\":%d,\"maxTempOfAll\":%.1f,\"minTempOfAll\":%.1f,\"image\":\"", itemCode->valueint, ir->maxTemp, ir->minTemp);
                headerLength = strlen(header);
                memcpy(sendData, header, headerLength);
                Base64_Encode(image, sendData + headerLength, *sendLength);
                i = (*sendLength)/3;
                j = (*sendLength)%3;
                if(j > 0)
                    *sendLength = (i + 1) * 4;
                else
                    *sendLength = (i) * 4;
                *sendLength = (*sendLength) + headerLength;
                memcpy(sendData + (*sendLength), "\"}", 2);
                *sendLength = (*sendLength) + 2;
            }
        }
        else
            return 401;
        break;
        case 2:
        if(listLength == 5)
        {
            res = Get_Ir_Temp_Data_Process(json, ir);
            if(res == 0)
            {
                memset(sendData, 0, IR_TEMP_DATA_LENGTH);
                sprintf(header, "{\"code\":%d,\"maxTempOfArea\":%.1f,\"minTempOfArea\":%.1f,\"avgTempOfArea\":%.1f}", 
                          itemCode->valueint, ir->maxTempOfArea, ir->minTempOfArea, ir->avgTempOfArea);
                *sendLength = strlen(header);
                memcpy(sendData, header, *sendLength);
            }
        }
        else
            return 401;
        break;
        default:
            return 402;
    }
    return res;
}

