/**
  ******************************************************************************
  * @file    wildfire_image_assistant.c
  * @version V1.0
  * @date    2020-xx-xx
  * @brief   野火摄像头助手接口
  ******************************************************************************
  * @attention
  *
  * 实验平台:野火 F103-指南者 STM32 开发板 
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */ 
  
#include "./WIA/wildfire_image_assistant.h"
#include "./ov7725/bsp_ov7725.h"
#include "./led/bsp_led.h"
#include "./protocol/protocol.h"
#include "./sccb/bsp_sccb.h"

extern uint8_t Ov7725_vsync;

/**
 * @brief  计算包的 CRC-16.
 *         CRC的寄存器值由rcr_init给出，这样可以不一次计算所有数据的结果
 *         （这里一帧图像有150KB，内部不能一次放下这么大的数据，需要分段计算）
 * @param  *data:  要计算的数据的数组.
 * @param  length: 数据的大小
 * @return status: 计算CRC.
 */
static uint16_t calc_crc_16(uint8_t *data, uint16_t length, uint16_t rcr_init)
{
  uint16_t crc = rcr_init;
  
  for(int n = 0; n < length; n++)
  {
    crc = data[n] ^ crc;
    
    for(int i = 0;i < 8;i++)
    {
      if(crc & 0x01)
      {
        crc = (crc >> 1) ^ 0xA001;
      }
      else
      {
        crc = crc >> 1;
      }
    }
  }
  
  return crc;    // 交换高字节和低字节位置
}

/**
 * @brief  发送设置图像格式包给上位机.
 * @param  type:   图像格式.
 * @param  width:  图像宽度.
 * @param  height: 图像高度.
 * @return void.
 */
void ack_read_wincc(uint16_t number, uint8_t val)
{
  uint16_t crc_16 = 0xFFFF;
  
  uint8_t packet_head[16] = {0x53, 0x5A, 0x48, 0x59,     // 包头
                             0x00,                       // 设备地址
                             0x10, 0x00, 0x00, 0x00,     // 包长度 (10+1+2+2+2)                
                             0x11,                       // 设置图像格式指令
                             };
  
  packet_head[10] = 0;         // 执行ok
  
  packet_head[11] = number & 0xFF;               // 寄存器对应上位机的序号
  packet_head[12] = (number >> 8 ) & 0xFF;       // 寄存器对应上位机的序号
                             
  packet_head[13] = val;       // 寄存器值
                             
  crc_16 = calc_crc_16((uint8_t *)&packet_head, sizeof(packet_head) - 2, crc_16);    // 分段计算crc—16的校验码, 计算包头的
  
  packet_head[14] = (crc_16 >> 8) & 0x00FF;
  packet_head[15] = crc_16 & 0x00FF;
                             
  CAM_ASS_SEND_DATA((uint8_t *)&packet_head, sizeof(packet_head));
}

/**
 * @brief   读寄存器
 * @param   addr：寄存器地址.
 * @return  status: 0:应答，-1：非应答.
 */
void read_rge(uint16_t number, uint8_t addr)
{
  uint8_t buff[1] = {0};
  
  if (SCCB_ReadByte(buff, 1, addr) == 1)
  {
    ack_read_wincc(number, buff[0]);
  }
}

/**
 * @brief   接收的数据处理
 * @param   void
 * @return  status: 0:应答，-1：非应答.
 */
int8_t receiving_process(void)
{
  uint8_t frame_data[128];         // 要能放下最长的帧
  uint16_t frame_len = 0;          // 帧长度
  uint8_t cmd_type = CMD_NONE;     // 命令类型
  
  while(1)
  {
    cmd_type = protocol_frame_parse(frame_data, &frame_len);
    switch (cmd_type)
    {
      case CMD_NONE:
      {
        return -1;
      }
      
      /* 写寄存器 */
      case CMD_WRITE_REG:
      {
//        uint8_t dev_add = frame_data[4];        // 设备地址
        uint8_t reg_add = frame_data[10];    // 寄存器地址
        uint8_t reg_val = frame_data[11];    // 寄存器值
        
        SCCB_WriteByte(reg_add, reg_val);
        break;
      }
      
      /* 读寄存器 */
      case CMD_READ_REG:
      {
//        uint8_t dev_add = frame_data[4];   // 设备地址
        uint16_t number = frame_data[10] | (frame_data[11] << 8);     // 序号
        uint8_t reg_add = frame_data[12];    // 寄存器地址
        
        read_rge(number, reg_add);
        break;
      }
      
      /* 接收到应答信号 */
      case CMD_ACK:
      {
        return 0;
      }

      default: 
        return -1;
    }
  }
}

/**
 * @brief  发送设置图像格式包给上位机.
 * @param  type:   图像格式.
 * @param  width:  图像宽度.
 * @param  height: 图像高度.
 * @return void.
 */
void set_wincc_format(uint8_t addr, uint8_t type, uint16_t width, uint16_t height)
{
  uint16_t crc_16 = 0xFFFF;
  
  uint8_t packet_head[17] = {0x53, 0x5A, 0x48, 0x59,     // 包头
                             0x00,                       // 设备地址
                             0x11, 0x00, 0x00, 0x00,     // 包长度 (10+1+2+2+2)                
                             0x01,                       // 设置图像格式指令
                             };
  
  packet_head[4] = addr;    // 修改设备地址
  
  packet_head[10] = type;
                             
  packet_head[11] = width & 0xFF;
  packet_head[12] = width >> 8;
                             
  packet_head[13] = height & 0xFF;
  packet_head[14] = height >> 8;
                             
  crc_16 = calc_crc_16((uint8_t *)&packet_head, sizeof(packet_head) - 2, crc_16);    // 分段计算crc—16的校验码, 计算包头的
  
  packet_head[15] = (crc_16 >> 8) & 0x00FF;
  packet_head[16] = crc_16 & 0x00FF;
                             
  CAM_ASS_SEND_DATA((uint8_t *)&packet_head, sizeof(packet_head));
}

/**
 * @brief  发送图像数据包给上位机.
 * @param  addr:   设备地址，0 or 1.
 * @param  width:  图像宽度.
 * @param  height: 图像高度.
 * @return 0：成功，-1：失败.
 */
int write_rgb_wincc(uint8_t addr, uint16_t width, uint16_t height) 
{
  uint16_t i, j; 
  uint16_t crc_16 = 0xFFFF;
	uint16_t Camera_Data[640];
  static uint8_t flag = 1;

  /* 发送图像包头*/
  packet_head_t packet_head =
  {
    .head = FRAME_HEADER,   // 包头
    .addr = 0x00,           // 设备地址
    .len  = 0x00,           // 包长度
    .cmd  = CMD_PIC_DATA,   // 发送图像数据包
  };                        
  
  if (flag)    /* 设置一次上位机的图像格式 */
  {
    receiving_process();    // 处理一下前面接收到的数据
    do{
      set_wincc_format(addr, PIC_FORMAT_RGB565, width, height);     // 发送设置图像格式指令
      flag++;
      if (flag > 5)
        LED1_ON;
    }while(receiving_process() != 0);    // 判断是否收到应答
    
    flag = 0;
    LED1_OFF;
  }

  if (Ov7725_vsync == 2)    // 采集完成
  {
    FIFO_PREPARE;  			/*FIFO准备*/
    
    packet_head.addr = addr;    // 修改设备地址
                      
    /* 修改包长度 */
    packet_head.len = 10 + width * height * 2 + 2;    // 计算包长度

    /* 发送头 */
    CAM_ASS_SEND_DATA((uint8_t *)&packet_head, sizeof(packet_head));
    crc_16 = calc_crc_16((uint8_t *)&packet_head, sizeof(packet_head), crc_16);    // 分段计算crc—16的校验码, 计算包头的

    /* 发送图像数据 */
    for(i = 0; i < width; i++)
    {
      for(j = 0; j < height; j++)
      {
        READ_FIFO_PIXEL(Camera_Data[j]);		// 从FIFO读出一个rgb565像素到Camera_Data变量
      }

      CAM_ASS_SEND_DATA((uint8_t *)Camera_Data, j*2);           		// 分段发送像素数据
//      crc_16 = calc_crc_16((uint8_t *)Camera_Data, j*2, crc_16);    // 分段计算crc—16的校验码，计算一行图像数据
    }

    /*发送校验数据*/
    crc_16 = ((crc_16&0x00FF)<<8)|((crc_16&0xFF00)>>8);    //  交换高字节和低字节位置
    CAM_ASS_SEND_DATA((uint8_t *)&crc_16, 2);              // 上位机不勾选CRC-16也需要发送这两个字节
    
    Ov7725_vsync = 0;		 // 开始下次采集
    LED2_TOGGLE;
  }

  return 0;    // 返回成功
}

