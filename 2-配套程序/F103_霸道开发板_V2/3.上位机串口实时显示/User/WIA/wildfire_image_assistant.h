#ifndef __WIA_H
#define	__WIA_H


#include "stm32f10x.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "./usart/bsp_usart.h"

typedef __packed struct
{
  uint32_t head;     // 包头
  uint8_t addr;      // 设备地址
  uint32_t len;      // 包长度
  uint8_t cmd;       // 命令
}packet_head_t;

/* 发送数据接口 */
#define CAM_ASS_SEND_DATA(data, len)     debug_send_data(data, len)
//#define CAM_ASS_SEND_DATA(data, len)       send(SOCK_TCPC,data,len);

int8_t receiving_process(void);
void set_wincc_format(uint8_t addr, uint8_t type, uint16_t width, uint16_t height);
int write_rgb_wincc(uint8_t addr, uint16_t width, uint16_t height) ;
int write_rgb_file(uint8_t addr, uint16_t width, uint16_t height, char *file_name) ;

#endif /* __WIA_H */

