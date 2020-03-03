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
  * 实验平台:野火 F103-指南者 STM32 开发板 
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */
  
#include "stm32f10x.h"
#include "./ov7725/bsp_ov7725.h"
#include "./lcd/bsp_ili9341_lcd.h"
#include "./led/bsp_led.h"   
#include "./usart/bsp_usart.h"
#include "./key/bsp_key.h"  
#include "./systick/bsp_SysTick.h"
#include "./WIA/wildfire_Image_assistant.h"
#include "ff.h"


extern uint8_t Ov7725_vsync;

unsigned int Task_Delay[NumOfTask]; 

extern OV7725_MODE_PARAM cam_mode;

FATFS fs;													/* FatFs文件系统对象 */
FRESULT res_sd;                /* 文件操作结果 */


/**
  * @brief  主函数
  * @param  无  
  * @retval 无
  */
int main(void) 	
{		
	
	float frame_count = 0;
	uint8_t retry = 0;
  int x = 0;

	/* 液晶初始化 */
	ILI9341_Init();
	ILI9341_GramScan ( 3 );
	
	LCD_SetFont(&Font8x16);
	LCD_SetColors(RED,BLACK);

  ILI9341_Clear(0,0,LCD_X_LENGTH,LCD_Y_LENGTH);	/* 清屏，显示全黑 */
	
	/********显示字符串示例*******/
  ILI9341_DispStringLine_EN(LINE(0),"BH OV7725 Test Demo");

	USART_Config();
	LED_GPIO_Config();
	Key_GPIO_Config();
	SysTick_Init();
	
	/*挂载sd文件系统*/
	res_sd = f_mount(&fs,"0:",1);
	if(res_sd != FR_OK)
	{
		printf("\r\n请给开发板插入已格式化成fat格式的SD卡。\r\n");
	}
	
//	printf("\r\n ** OV7725摄像头实时液晶显示例程** \r\n"); 
//	while(1);
	/* ov7725 gpio 初始化 */
	OV7725_GPIO_Config();
	
	LED_BLUE;
	/* ov7725 寄存器默认配置初始化 */
	while(OV7725_Init() != SUCCESS)
	{
		retry++;
		if(retry>5)
		{
			printf("\r\n没有检测到OV7725摄像头\r\n");
			ILI9341_DispStringLine_EN(LINE(2),"No OV7725 module detected!");
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

	/* 设置液晶扫描模式 */
	ILI9341_GramScan( cam_mode.lcd_scan );
	
	
	
	ILI9341_DispStringLine_EN(LINE(2),"OV7725 initialize success!");
//	printf("\r\nOV7725摄像头初始化完成\r\n");
	
	Ov7725_vsync = 0;

//  while(Key_Scan(KEY2_GPIO_PORT,KEY2_GPIO_PIN) != KEY_ON)
//  {
//    /*检测按键*/
//    if( Key_Scan(KEY1_GPIO_PORT,KEY1_GPIO_PIN) == KEY_ON  )
//    {		
//      set_wincc_format(0x02, cam_mode.cam_width, cam_mode.cam_height);
//      
//      /* 此处应当检测上位机的应答信号 */
//    }
//  }
	
	while(1)
	{
//    if (x % 2 == 0)
//    {
      /* 写图像数据到上位机 */
      write_rgb_wincc(x % 2, cam_mode.cam_width, cam_mode.cam_height);
      LED3_TOGGLE;
//    }
//    else
//    {
//      /* 接收到新图像进行显示实时（LCD） */
//      if( Ov7725_vsync == 2 )
//      {
//        frame_count++;
//        FIFO_PREPARE;  			/*FIFO准备*/
//        ImagDisp(cam_mode.lcd_sx,
//                  cam_mode.lcd_sy,
//                  cam_mode.cam_width,
//                  cam_mode.cam_height);			/*采集并显示*/
//        
//        Ov7725_vsync = 0;
//        LED1_TOGGLE;
//      }
//    }

    /*检测按键*/
    if( Key_Scan(KEY1_GPIO_PORT,KEY1_GPIO_PIN) == KEY_ON  )
    {		
      set_wincc_format(x % 2, PIC_FORMAT_RGB565, cam_mode.cam_width, cam_mode.cam_height);
      
      while(Key_Scan(KEY2_GPIO_PORT,KEY2_GPIO_PIN) != KEY_ON);
      /* 此处应当检测上位机的应答信号 */
    }
		
		/*检测按键*/
//		if( Key_Scan(KEY1_GPIO_PORT,KEY1_GPIO_PIN) == KEY_ON  )
//		{
//			static uint8_t name_count = 0;
//			char name[40];
//      
//      while(Ov7725_vsync != 2);    // 等待采集完成
//      FIFO_PREPARE;  			/*FIFO准备*/
//			
//			//用来设置截图名字，防止重复，实际应用中可以使用系统时间来命名。
//			name_count++; 
//			sprintf(name,"0:/photo2_%dX%d_%d.dat",cam_mode.cam_width, cam_mode.cam_height,name_count);

//			LED_BLUE;
//			printf("\r\n正在保存到文件...");
//			
//			if(write_rgb_file(0, cam_mode.cam_width, cam_mode.cam_height, name) == 0)
//			{
//				printf("\r\n保存到文件完成！");
//				LED_GREEN;
//			}
//			else
//			{
//				printf("\r\n保存到文件失败！");
//				LED_RED;
//			}
//      
//      Ov7725_vsync = 0;		 // 开始下次采集
//		}
    
		/* 检测按键切换实时显示或者发到上位机 */
		if( Key_Scan(KEY2_GPIO_PORT,KEY2_GPIO_PIN) == KEY_ON  )
		{
			x++;
    }
   }
}




/*********************************************END OF FILE**********************/

