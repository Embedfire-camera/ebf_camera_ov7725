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

extern uint8_t Ov7725_vsync;

/**
 * @brief   计算包的 CRC-16.
 *          CRC的寄存器值由rcr_init给出，这样可以不一次计算所有数据的结果
 *          （这里一帧图像有150KB，内部不能一次放下这么大的数据，需要分段计算）
 * @param   *data:  要计算的数据的数组.
 * @param   length: 数据的大小
 * @return  status: 计算CRC.
 */
uint16_t calc_crc_16(uint8_t *data, uint16_t length, uint16_t rcr_init)
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
 * @brief  没有附加数据的应答.
 * @param  timeout: 超时时间.
 * @return -1：超时返回；非-1：收到应答，返回上位机应答的确认字.
 */
int no_ack_with_data_attached(uint32_t timeout)
{
  uint8_t* data = NULL;
  uint32_t packet_head = 0;
  
  while(get_rx_flag() != 1 && --timeout)
  {
    
  }
  
  if (timeout <= 0)
  {
    return -1;    /* 返回超时错误 */
  }
  else
  {
    /* 收到数据 */
    data = get_rx_data();
    
    packet_head = *data << 24 | *(data+1) << 16 | *(data+2) << 8 | *(data+3);    // 合成包头
    
    if (packet_head == FRAME_HEADER)    // 校验包头是否正确
    {
      uint32_t pack_len = *(data+5) << 24 | *(data+6) << 16 | *(data+7) << 8 | *(data+8);    // 合成长度
      
      if (calc_crc_16(data, pack_len-2, 0xFFFF) == (*(data+pack_len-1) | *(data+pack_len-2) << 8))    // 校验 CRC-16
      {
        reset_rx_data();    // 重置数据接收
        
        return *(data + 10);    // 返回确认字
      }
    }
    
    return -1;
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
  
  uint8_t packet_head[17] = {0x59, 0x48, 0x5A, 0x53,     // 包头
                             0x00,                       // 设备地址
                             0x00, 0x00, 0x00, 0x11,     // 包长度 (10+1+2+2+2)                
                             0x01,                       // 设置图像格式指令
                             };
  
  packet_head[4] = addr;    // 修改设备地址
  
  packet_head[10] = type;
                             
  packet_head[11] = width >> 8;
  packet_head[12] = width & 0xFF;
                             
  packet_head[13] = height >> 8;
  packet_head[14] = height & 0xFF;
                             
  crc_16 = calc_crc_16((uint8_t *)&packet_head, sizeof(packet_head) - 2, crc_16);    // 分段计算crc—16的校验码, 计算包头的
  
  packet_head[15] = crc_16 & 0x00FF;
  packet_head[16] = (crc_16 >> 8) & 0x00FF;
                             
//  printf("0x%x\r\n", packet_head[15]);
                             
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

//  packet_head_t packet_head =
//  {
//    .head = 0x59485A53,     // 包头
//    .addr = 0x00,           // 设备地址
//    .len  = 0x00000011,     // 包长度 (10+320*240*2+2)
//    .cmd  = 02,             // 图像数据命令
//  };
  
  if (flag)    /* 设置一次上位机的图像格式 */
  {
    
    do
    {
      reset_rx_data();    // 重置数据接收
      
      set_wincc_format(addr, PIC_FORMAT_RGB565, width, height);
      flag++;
    }
    while(no_ack_with_data_attached(0x1FFFFFF) != 0 && flag < 10);
    
    if (flag >= 10)
    {
      LED1_ON;
    }
    
    flag = !flag;
  }
  
  
  if (Ov7725_vsync == 2)    // 采集完成
  {
    FIFO_PREPARE;  			/*FIFO准备*/
  
    uint8_t packet_head[10] = {0x59, 0x48, 0x5A, 0x53,     // 包头
                               0x00,                       // 设备地址
                               0x00, 0x02, 0x58, 0x0C,     // 包长度 (10+320*240*2+2)
                               0x02                        // 图像数据命令
                               };
    
    packet_head[4] = addr;    // 修改设备地址
    
    uint32_t data_len = 10 + width * height * 2 + 2;    // 计算包长度
                               
    /* 修改包长度 */
    packet_head[5] = data_len >> 24;
    packet_head[6] = data_len >> 16;
    packet_head[7] = data_len >> 8;
    packet_head[8] = data_len;
    
    memset(Camera_Data, 0xDD, sizeof(Camera_Data));

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
//      crc_16 = calc_crc_16((uint8_t *)Camera_Data, j*2, crc_16);    // 分段计算crc—16的校验码，计算一行图像数据 (不计算校验会快点，上位机也不要勾选校验)
    }

    /*发送校验数据*/
    crc_16 = ((crc_16&0x00FF)<<8)|((crc_16&0xFF00)>>8);    //  交换高字节和低字节位置
    CAM_ASS_SEND_DATA((uint8_t *)&crc_16, 2);
    
    Ov7725_vsync = 0;		 // 开始下次采集
  }

  return 0;    // 返回成功
}

/*********************************************************************************************/
