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

/* 图像格式宏定义 */
#define PIC_FORMAT_JPEG       0x01u    // 图像为 JPEG 图片
#define PIC_FORMAT_BMP        0x02u    // 图像为 BMP  图片
#define PIC_FORMAT_PNG        0x03u    // 图像为 PNG  图片
#define PIC_FORMAT_RGB565     0x04u    // 图像为 RGB565 数据
#define PIC_FORMAT_RGB888     0x05u    // 图像为 RGB888 数据
#define PIC_FORMAT_WB         0x06u    // 图像为二值化数据

/* 帧头宏定义 */
#define FRAME_HEADER    0x59485A53u    // 帧头

/* 命令宏定义 */
#define CMD_ACK          0x00u   // 应答包指令
#define CMD_FORMAT       0x01u   // 设置上位机图像格式、宽高指令
#define CMD_PIC_DATA     0x02u   // 发送图像数据指令
#define CMD_WRITE_REG    0x10u   // 写寄存器指令
#define CMD_READ_REG     0x20u   // 读寄存器指令

/* 发送数据接口 */
#define CAM_ASS_SEND_DATA(data, len)     debug_send_data(data, len)
//#define CAM_ASS_SEND_DATA(data, len)       send(SOCK_TCPC,data,len);

void set_wincc_format(uint8_t addr, uint8_t type, uint16_t width, uint16_t height);
int write_rgb_wincc(uint8_t addr, uint16_t width, uint16_t height) ;
int write_rgb_file(uint8_t addr, uint16_t width, uint16_t height, char *file_name) ;

#endif /* __WIA_H */

