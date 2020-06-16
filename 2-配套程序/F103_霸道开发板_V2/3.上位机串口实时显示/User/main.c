/**
  ******************************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2013-xx-xx
  * @brief   摄像头火眼ov7725测试例程
  ******************************************************************************
  * @attention
  *
  * 实验平台:野火 F103-霸道 STM32 开发板 
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */
  
#include "stm32f10x.h"
#include "./ov7725/bsp_ov7725.h"
#include "./led/bsp_led.h"   
#include "./usart/bsp_usart.h"
#include "./key/bsp_key.h"  
#include "./systick/bsp_SysTick.h"
#include "./WIA/wildfire_image_assistant.h"
#include "./protocol/protocol.h"

extern uint8_t Ov7725_vsync;

unsigned int Task_Delay[NumOfTask]; 


extern OV7725_MODE_PARAM cam_mode;


/**
  * @brief  主函数
  * @param  无  
  * @retval 无
  */
int main(void) 	
{		
	
	float frame_count = 0;
	uint8_t retry = 0;
  /* 初始化 */
  protocol_init();    // 通讯协议初始化
  
  /* 注意 *//* 注意 *//* 注意 *//* 注意 *//* 注意 *//* 注意 *//* 注意 */
  /*注意上位机波特率请设置为：1500000（没有这个波特率选项请手动修改）*/
	USART_Config();
	LED_GPIO_Config();
	Key_GPIO_Config();
	SysTick_Init();
	
	/* ov7725 gpio 初始化 */
	OV7725_GPIO_Config();

	/* ov7725 寄存器默认配置初始化 */
	while(OV7725_Init() != SUCCESS)
	{
		retry++;
		if(retry>5)
		{
			LED1_ON;
			while(1);
		}
	}


	/*根据摄像头参数组配置模式*/
	OV7725_Special_Effect(cam_mode.effect);
	/*光照模式*/
	OV7725_Light_Mode(cam_mode.light_mode);
	/*饱和度*/
	OV7725_Color_Saturation(cam_mode.saturation);
	/*光照度*/
	OV7725_Brightness(cam_mode.brightness);
	/*对比度*/
	OV7725_Contrast(cam_mode.contrast);
	/*特殊效果*/
	OV7725_Special_Effect(cam_mode.effect);
	
	/*设置图像采样及模式大小*/
	OV7725_Window_Set(cam_mode.cam_sx,
                    cam_mode.cam_sy,
                    cam_mode.cam_width,
                    cam_mode.cam_height,
                    cam_mode.QVGA_VGA);

	Ov7725_vsync = 0;
	
  /* 注意 *//* 注意 *//* 注意 *//* 注意 *//* 注意 *//* 注意 *//* 注意 */
  /*注意上位机波特率请设置为：1500000（没有这个波特率选项，请手动修改）*/
	while(1)
	{
    write_rgb_wincc(0, cam_mode.cam_width, cam_mode.cam_height);
    
    receiving_process();    // 接收数据处理
	}
}




/*********************************************END OF FILE**********************/

