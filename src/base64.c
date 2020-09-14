#include "base64.h"
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>


const char base[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/="; 


/* */ 
static char Find_Pos(char ch)   
{ 
    char *ptr = (char*)strrchr(base, ch);//the last position (the only) in base[] 
    return (ptr - base); 
} 
int Tolower(int c) 
{ 
    if (c >= 'A' && c <= 'Z') 
    { 
        return c + 'a' - 'A'; 
    } 
    else 
    { 
        return c; 
    } 
} 

int Htoi(const char s[],int start,int len) 
{ 
    int i,j; 
    int n = 0; 
    if (s[0] == '0' && (s[1]=='x' || s[1]=='X')) //判断是否有前导0x或者0X
    { 
        i = 2; 
    } 
    else 
    { 
        i = 0; 
    } 
    i+=start;
    j=0;
    for (; (s[i] >= '0' && s[i] <= '9') 
       || (s[i] >= 'a' && s[i] <= 'f') || (s[i] >='A' && s[i] <= 'F');++i) 
    {   
        if(j>=len)
    {
      break;
    }
        if (Tolower(s[i]) > '9') 
        { 
            n = 16 * n + (10 + Tolower(s[i]) - 'a'); 
        } 
        else 
        { 
            n = 16 * n + (Tolower(s[i]) - '0'); 
        } 
    j++;
    } 
    return n; 
} 

/* */ 
void Base64_Encode(char *data, char *ret, unsigned long dataLen) 
{ 

    int prepare = 0; 
    unsigned long ret_len; 
    int temp = 0; 
    char *f = NULL; 
    unsigned long tmp = 0; 
    char changed[4]; 
    unsigned long i = 0; 
    ret_len = dataLen / 3; 
    temp = dataLen % 3; 
    if (temp > 0) 
    { 
        ret_len += 1; 
    } 
    ret_len = ret_len*4 + 1; 
    memset(ret, 0, ret_len); 
    f = ret; 
    while (tmp < dataLen) 
    { 
        temp = 0; 
        prepare = 0; 
        memset(changed, '\0', 4); 
        while (temp < 3) 
        { 
            if (tmp >= dataLen) 
            { 
                break; 
            } 
            prepare = ((prepare << 8) | (data[tmp] & 0xFF)); 
            tmp++; 
            temp++; 
        } 
        prepare = (prepare<<((3-temp)*8)); 
        for (i = 0; i < 4 ;i++ ) 
        { 
            if (temp < i) 
            { 
                changed[i] = 0x40; 
            } 
            else 
            { 
                changed[i] = (prepare>>((3-i)*6)) & 0x3F; 
            } 
            *f = base[changed[i]]; 
            f++; 
        } 
    } 
    *f = '\0'; 
} 

/* */ 
void Base64_Decode(char *data, char *ret, unsigned long dataLen) 
{ 
    unsigned long ret_len = (dataLen / 4) * 3; 
    unsigned long equal_count = 0; 
    char *f = NULL; 
    unsigned long tmp = 0; 
    unsigned long temp = 0; 
    char need[3]; 
    unsigned long prepare = 0; 
    unsigned long i = 0; 
    if (*(data + dataLen - 1) == '=') 
    { 
        equal_count += 1; 
    } 
    if (*(data + dataLen - 2) == '=') 
    { 
        equal_count += 1; 
    } 
    if (*(data + dataLen - 3) == '=') 
    {//seems impossible 
        equal_count += 1; 
    } 
    switch (equal_count) 
    { 
    case 0: 
        ret_len += 4;//3 + 1 [1 for NULL] 
        break; 
    case 1: 
        ret_len += 4;//Ceil((6*3)/8)+1 
        break; 
    case 2: 
        ret_len += 3;//Ceil((6*2)/8)+1 
        break; 
    case 3: 
        ret_len += 2;//Ceil((6*1)/8)+1 
        break; 
    } 
    memset(ret, 0, ret_len); 
    f = ret; 
    while (tmp < (dataLen - equal_count)) 
    { 
        temp = 0; 
        prepare = 0; 
        memset(need, 0, 4); 
        while (temp < 4) 
        { 
            if (tmp >= (dataLen - equal_count)) 
            { 
                break; 
            } 
            prepare = (prepare << 6) | (Find_Pos(data[tmp])); 
            temp++; 
            tmp++; 
        } 
        prepare = prepare << ((4-temp) * 6); 
        for (i=0; i<3 ;i++ ) 
        { 
            if (i == temp) 
            { 
                break; 
            } 
            *f = (char)((prepare>>((2-i)*8)) & 0xFF); 
            f++; 
        } 
    } 
    *f = '\0'; 
}
 
