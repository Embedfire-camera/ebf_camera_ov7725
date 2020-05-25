/**
  ******************************************************************************
  * @file    bsp_key.c
  * @author  fire
  * @version V1.0
  * @date    2013-xx-xx
  * @brief   bmp文件驱动，显示bmp图片和给液晶截图
  ******************************************************************************
  * @attention
  *
  * 实验平台:野火 F103-指南者 STM32 开发板 
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */ 
  
#include "ff.h"
#include "./lcd/bsp_ili9341_lcd.h"
#include "./bmp/bsp_bmp.h"
#include "./usart/bsp_usart.h"

#define RGB24TORGB16(R,G,B) ((unsigned short int)((((R)>>3)<<11) | (((G)>>2)<<5)	| ((B)>>3)))

BYTE pColorData[960];			/* 一行真彩色数据缓存 320 * 3 = 960 */
FIL bmpfsrc, bmpfdst; 
FRESULT bmpres;

/* 如果不需要打印bmp相关的提示信息,将printf注释掉即可
 * 如要用printf()，需将串口驱动文件包含进来
 */
#define BMP_DEBUG_PRINTF(FORMAT,...)  printf(FORMAT,##__VA_ARGS__)	 

/* 打印BMP文件的头信息，用于调试 */
static void showBmpHead(BITMAPFILEHEADER* pBmpHead)
{
    BMP_DEBUG_PRINTF("位图文件头:\r\n");
    BMP_DEBUG_PRINTF("文件大小:%ld\r\n",(*pBmpHead).bfSize);
    BMP_DEBUG_PRINTF("保留字:%d\r\n",(*pBmpHead).bfReserved1);
    BMP_DEBUG_PRINTF("保留字:%d\r\n",(*pBmpHead).bfReserved2);
    BMP_DEBUG_PRINTF("实际位图数据的偏移字节数:%ld\r\n",(*pBmpHead).bfOffBits);
		BMP_DEBUG_PRINTF("\r\n");	
}

/* 打印BMP文件的头信息，用于调试 */
static void showBmpInforHead(tagBITMAPINFOHEADER* pBmpInforHead)
{
    BMP_DEBUG_PRINTF("位图信息头:\r\n");
    BMP_DEBUG_PRINTF("结构体的长度:%ld\r\n",(*pBmpInforHead).biSize);
    BMP_DEBUG_PRINTF("位图宽:%ld\r\n",(*pBmpInforHead).biWidth);
    BMP_DEBUG_PRINTF("位图高:%ld\r\n",(*pBmpInforHead).biHeight);
    BMP_DEBUG_PRINTF("biPlanes平面数:%d\r\n",(*pBmpInforHead).biPlanes);
    BMP_DEBUG_PRINTF("biBitCount采用颜色位数:%d\r\n",(*pBmpInforHead).biBitCount);
    BMP_DEBUG_PRINTF("压缩方式:%ld\r\n",(*pBmpInforHead).biCompression);
    BMP_DEBUG_PRINTF("biSizeImage实际位图数据占用的字节数:%ld\r\n",(*pBmpInforHead).biSizeImage);
    BMP_DEBUG_PRINTF("X方向分辨率:%ld\r\n",(*pBmpInforHead).biXPelsPerMeter);
    BMP_DEBUG_PRINTF("Y方向分辨率:%ld\r\n",(*pBmpInforHead).biYPelsPerMeter);
    BMP_DEBUG_PRINTF("使用的颜色数:%ld\r\n",(*pBmpInforHead).biClrUsed);
    BMP_DEBUG_PRINTF("重要颜色数:%ld\r\n",(*pBmpInforHead).biClrImportant);
		BMP_DEBUG_PRINTF("\r\n");
}





/**
 * @brief  设置ILI9341的截取BMP图片
 * @param  x ：在扫描模式1下截取区域的起点X坐标 
 * @param  y ：在扫描模式1下截取区域的起点Y坐标 
 * @param  pic_name ：BMP存放的全路径
 * @retval 无
 */
void LCD_Show_BMP ( uint16_t x, uint16_t y, char * pic_name )
{
	int i, j, k;
	int width, height, l_width;

	BYTE red,green,blue;
	BITMAPFILEHEADER bitHead;
	BITMAPINFOHEADER bitInfoHead;
	WORD fileType;

	unsigned int read_num;

	bmpres = f_open( &bmpfsrc , (char *)pic_name, FA_OPEN_EXISTING | FA_READ);	
/*-------------------------------------------------------------------------------------------------------*/
	if(bmpres == FR_OK)
	{
		BMP_DEBUG_PRINTF("打开文件成功\r\n");

		/* 读取文件头信息  两个字节*/         
		f_read(&bmpfsrc,&fileType,sizeof(WORD),&read_num);     

		/* 判断是不是bmp文件 "BM"*/
		if(fileType != 0x4d42)
		{
			BMP_DEBUG_PRINTF("这不是一个 .bmp 文件!\r\n");
			return;
		}
		else
		{
			BMP_DEBUG_PRINTF("这是一个 .bmp 文件\r\n");	
		}        

		/* 读取BMP文件头信息*/
		f_read(&bmpfsrc,&bitHead,sizeof(tagBITMAPFILEHEADER),&read_num);        
		showBmpHead(&bitHead);

		/* 读取位图信息头信息 */
		f_read(&bmpfsrc,&bitInfoHead,sizeof(BITMAPINFOHEADER),&read_num);        
		showBmpInforHead(&bitInfoHead);
	}    
	else
	{
		BMP_DEBUG_PRINTF("打开文件失败!错误代码：bmpres = %d \r\n",bmpres);
		return;
	}    
/*-------------------------------------------------------------------------------------------------------*/
	width = bitInfoHead.biWidth;
	height = bitInfoHead.biHeight;

	/* 计算位图的实际宽度并确保它为32的倍数	*/
	l_width = WIDTHBYTES(width* bitInfoHead.biBitCount);	

	if(l_width > 960)
	{
		BMP_DEBUG_PRINTF("\n 本图片太大，无法在液晶屏上显示 (<=320)\n");
		return;
	}
	
	
	/* 开一个图片大小的窗口*/
	ILI9341_OpenWindow(x, y, width, height);
	ILI9341_Write_Cmd (CMD_SetPixel ); 

	
	/* 判断是否是24bit真彩色图 */
	if( bitInfoHead.biBitCount >= 24 )
	{
		for ( i = 0; i < height; i ++ )
		{
			/*从文件的后面读起，BMP文件的原始图像方向为右下角到左上角*/
      f_lseek ( & bmpfsrc, bitHead .bfOffBits + ( height - i - 1 ) * l_width );	
			
			/* 读取一行bmp的数据到数组pColorData里面 */
			f_read ( & bmpfsrc, pColorData, l_width, & read_num );				

			for(j=0; j<width; j++) 											   //一行有效信息
			{
				k = j*3;																	 //一行中第K个像素的起点
				red = pColorData[k+2];
				green = pColorData[k+1];
				blue = 	pColorData[k];
				ILI9341_Write_Data ( RGB24TORGB16 ( red, green, blue ) ); //写入LCD-GRAM
			}            			
		}        		
	}    	
	else 
	{        
		BMP_DEBUG_PRINTF("这不是一个24位真彩色BMP文件！");
		return ;
	}
	
	f_close(&bmpfsrc);  
  
}




/**
 * @brief  设置ILI9341的截取BMP图片
 * @param  x ：截取区域的起点X坐标 
 * @param  y ：截取区域的起点Y坐标 
 * @param  Width ：区域宽度
 * @param  Height ：区域高度 
 * @retval 无
  *   该参数为以下值之一：
  *     @arg 0 :截图成功
  *     @arg -1 :截图失败
 */
int Screen_Shot( uint16_t x, uint16_t y, uint16_t Width, uint16_t Height, char * filename)
{
	/* bmp  文件头 54个字节 */
	unsigned char header[54] =
	{
		0x42, 0x4d, 0, 0, 0, 0, 
		0, 0, 0, 0, 54, 0, 
		0, 0, 40,0, 0, 0, 
		0, 0, 0, 0, 0, 0, 
		0, 0, 1, 0, 24, 0, 
		0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 
		0, 0, 0
	};
	
	int i;
	int j;
	long file_size;     
	long width;
	long height;
	unsigned char r,g,b;	
	unsigned int mybw;
	unsigned int read_data;
	char kk[4]={0,0,0,0};
	
	uint8_t ucAlign;//
	
	
	/* 宽*高 +补充的字节 + 头部信息 */
	file_size = (long)Width * (long)Height * 3 + Height*(Width%4) + 54;		

	/* 文件大小 4个字节 */
	header[2] = (unsigned char)(file_size &0x000000ff);
	header[3] = (file_size >> 8) & 0x000000ff;
	header[4] = (file_size >> 16) & 0x000000ff;
	header[5] = (file_size >> 24) & 0x000000ff;
	
	/* 位图宽 4个字节 */
	width=Width;	
	header[18] = width & 0x000000ff;
	header[19] = (width >> 8) &0x000000ff;
	header[20] = (width >> 16) &0x000000ff;
	header[21] = (width >> 24) &0x000000ff;
	
	/* 位图高 4个字节 */
	height = Height;
	header[22] = height &0x000000ff;
	header[23] = (height >> 8) &0x000000ff;
	header[24] = (height >> 16) &0x000000ff;
	header[25] = (height >> 24) &0x000000ff;
		
	/* 新建一个文件 */
	bmpres = f_open( &bmpfsrc , (char*)filename, FA_CREATE_ALWAYS | FA_WRITE );
	
	/* 新建文件之后要先关闭再打开才能写入 */
	f_close(&bmpfsrc);
		
	bmpres = f_open( &bmpfsrc , (char*)filename,  FA_OPEN_EXISTING | FA_WRITE);

	if ( bmpres == FR_OK )
	{    
		/* 将预先定义好的bmp头部信息写进文件里面 */
		bmpres = f_write(&bmpfsrc, header,sizeof(unsigned char)*54, &mybw);		
			
		ucAlign = Width % 4;
		
		for(i=0; i<Height; i++)					
		{
			for(j=0; j<Width; j++)  
			{					
				read_data = ILI9341_GetPointPixel ( x + j, y + Height - 1 - i );	    // 读一个RGB565像素				
				
        /* 拆分为 R G B */
				r =  GETR_FROM_RGB16(read_data);
				g =  GETG_FROM_RGB16(read_data);
				b =  GETB_FROM_RGB16(read_data);

        /* 写入文件 */
				bmpres = f_write(&bmpfsrc, &b,sizeof(unsigned char), &mybw);
				bmpres = f_write(&bmpfsrc, &g,sizeof(unsigned char), &mybw);
				bmpres = f_write(&bmpfsrc, &r,sizeof(unsigned char), &mybw);
			}
				
			if( ucAlign )				/* 如果不是4字节对齐 */
				bmpres = f_write ( & bmpfsrc, kk, sizeof(unsigned char) * ( ucAlign ), & mybw );

		}/* 截屏完毕 */

		f_close(&bmpfsrc); 
		
		return 0;
		
	}	
	else/* 截屏失败 */
		return -1;
}

#include "./ov7725/bsp_ov7725.h"

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

/* 数据头结构体 */
typedef __packed struct
{
  uint32_t head;
  uint8_t addr;
  uint32_t len;
  uint8_t cmd;
}packet_head_t;

int write_rgb_file(uint16_t width, uint16_t height, char * filename) 
{
  uint16_t i, j; 
  uint16_t crc_16 = 0xFFFF;
	uint16_t Camera_Data[700];
  unsigned int mybw;

//  packet_head_t packet_head =
//  {
//    .head = 0x59485A53,     // 包头
//    .addr = 0x00,           // 设备地址
//    .len  = 0x00000011,     // 包长度 (10+320*240*2+2)
//    .cmd  = 02,             // 图像数据命令
//  };
  
  uint8_t packet_head[10] = {0x59, 0x48, 0x5A, 0x53,     // 包头
                             0x00,                       // 设备地址
                             0x00, 0x02, 0x58, 0x0C,     // 包长度 (10+320*240*2+2)
                             0x02                        // 图像数据命令
                             };
  
  uint32_t data_len = 10 + width * height * 2 + 2;
                             
  packet_head[5] = data_len >> 24;
  packet_head[6] = data_len >> 16;
  packet_head[7] = data_len >> 8;
  packet_head[8] = data_len;
  
  memset(Camera_Data, 0xDD, sizeof(Camera_Data));
  	/* 新建一个文件 */
	bmpres = f_open( &bmpfsrc , (char*)filename, FA_CREATE_ALWAYS | FA_WRITE );
	
	/* 新建文件之后要先关闭再打开才能写入 */
	f_close(&bmpfsrc);
		
	bmpres = f_open( &bmpfsrc , (char*)filename,  FA_OPEN_EXISTING | FA_WRITE);

	if ( bmpres == FR_OK )    // 文件打开成功
  {
    /* 保存头 */
    f_write(&bmpfsrc, (uint8_t *)&packet_head, sizeof(packet_head), &mybw);        // 发送包头
    crc_16 = calc_crc_16((uint8_t *)&packet_head, sizeof(packet_head), crc_16);    // 分段计算crc—16的校验码, 计算包头的
    
    /* 保存图像数据 */
    for(i = 0; i < width; i++)
    {
      for(j = 0; j < height; j++)
      {
        READ_FIFO_PIXEL(Camera_Data[j]);		// 从FIFO读出一个rgb565像素到Camera_Data变量
      }
      
      f_write(&bmpfsrc, Camera_Data, j*2, &mybw);        // 发送图像数据
      crc_16 = calc_crc_16((uint8_t *)Camera_Data, j*2, crc_16);    // 分段计算crc—16的校验码，计算一行图像数据
    }
    
    /*保存校验数据*/
    crc_16 = ((crc_16&0x00FF)<<8)|((crc_16&0xFF00)>>8);    //  交换高字节和低字节位置
    f_write(&bmpfsrc, (uint8_t *)&crc_16, 2, &mybw);       // 发送crc校验数据
    
    f_close(&bmpfsrc);       // 关闭文件
  }
  else
  {
    f_close(&bmpfsrc);     // 关闭文件
    return -1;    // 返回失败
  }
  return 0;    // 返回成功
}

void set_upper_computer_format(uint8_t type, uint16_t width, uint16_t height)
{
  uint16_t crc_16 = 0xFFFF;
  
  uint8_t packet_head[17] = {0x59, 0x48, 0x5A, 0x53,     // 包头
                             0x00,                       // 设备地址
                             0x00, 0x00, 0x00, 0x11,     // 包长度 (10+1+2+2+2)                
                             0x01,                       // 设置图像格式指令
                             };
  
  packet_head[10] = type;
                             
  packet_head[11] = width >> 8;
  packet_head[12] = width & 0xFF;
                             
  packet_head[13] = height >> 8;
  packet_head[14] = height & 0xFF;
                             
  crc_16 = calc_crc_16((uint8_t *)&packet_head, sizeof(packet_head) - 2, crc_16);    // 分段计算crc—16的校验码, 计算包头的
  
  packet_head[15] = crc_16 & 0x00FF;
  packet_head[16] = (crc_16 >> 8) & 0x00FF;
                             
//  printf("0x%x\r\n", packet_head[15]);
                             
  debug_send_data((uint8_t *)&packet_head, sizeof(packet_head));
}

int write_rgb_usart(uint16_t width, uint16_t height) 
{
  uint16_t i, j; 
  uint16_t crc_16 = 0xFFFF;
	uint16_t Camera_Data[700];

//  packet_head_t packet_head =
//  {
//    .head = 0x59485A53,     // 包头
//    .addr = 0x00,           // 设备地址
//    .len  = 0x00000011,     // 包长度 (10+320*240*2+2)
//    .cmd  = 02,             // 图像数据命令
//  };
  
  uint8_t packet_head[10] = {0x59, 0x48, 0x5A, 0x53,     // 包头
                             0x00,                       // 设备地址
                             0x00, 0x02, 0x58, 0x0C,     // 包长度 (10+320*240*2+2)
                             0x02                        // 图像数据命令
                             };
  
  uint32_t data_len = 10 + width * height * 2 + 2;
                             
  packet_head[5] = data_len >> 24;
  packet_head[6] = data_len >> 16;
  packet_head[7] = data_len >> 8;
  packet_head[8] = data_len;
  
  memset(Camera_Data, 0xDD, sizeof(Camera_Data));

  /* 发送头 */
  debug_send_data((uint8_t *)&packet_head, sizeof(packet_head));
  crc_16 = calc_crc_16((uint8_t *)&packet_head, sizeof(packet_head), crc_16);    // 分段计算crc—16的校验码, 计算包头的

  /* 发送图像数据 */
  for(i = 0; i < width; i++)
  {
    for(j = 0; j < height; j++)
    {
      READ_FIFO_PIXEL(Camera_Data[j]);		// 从FIFO读出一个rgb565像素到Camera_Data变量
    }

    debug_send_data((uint8_t *)Camera_Data, j*2);
    crc_16 = calc_crc_16((uint8_t *)Camera_Data, j*2, crc_16);    // 分段计算crc—16的校验码，计算一行图像数据
  }

  /* 发送校验数据 */
  crc_16 = ((crc_16&0x00FF)<<8)|((crc_16&0xFF00)>>8);    //  交换高字节和低字节位置
  debug_send_data((uint8_t *)&crc_16, 2);

  return 0;    // 返回成功
}

/**
  * @brief  设置显示位置
	* @param  sx:x起始显示位置
	* @param  sy:y起始显示位置
	* @param  width:显示窗口宽度,要求跟OV7725_Window_Set函数中的width一致
	* @param  height:显示窗口高度，要求跟OV7725_Window_Set函数中的height一致
  * @param  pic_name:文件路径
  * @retval 无
  */
void FileImagDisp(uint16_t sx,uint16_t sy,uint16_t width,uint16_t height, char *pic_name)
{
	uint16_t i, j; 
	uint16_t color_data[1];
  unsigned int read_num;
	
	ILI9341_OpenWindow(sx,sy,width,height);
	ILI9341_Write_Cmd ( CMD_SetPixel );	
  
  bmpres = f_open( &bmpfsrc , (char *)pic_name, FA_OPEN_EXISTING | FA_READ);	

  if ( bmpres == FR_OK )    // 文件打开成功
  {
    /* 跳过前面的头，直接从像素数据开始读 */
    f_lseek ( & bmpfsrc, 10);
    
    for ( i = 0; i < height; i ++ )
		{
			for(j=0; j<width; j++) 											   //一行有效信息
			{
        /* 读取一个RGB565到 color_data */
        f_read ( & bmpfsrc, color_data, 2, &read_num);	
        
				ILI9341_Write_Data(color_data[0]);    // 写入 LCD 显存
			}            			
		}
    
    f_close(&bmpfsrc);       // 关闭文件
  }
  else
  {
    f_close(&bmpfsrc);     // 关闭文件
  }
}

/**
 * @brief  将rgb转换成bmp
 * @param  x ：起点X坐标 
 * @param  y ：起点Y坐标 
 * @param  Width ：区域宽度
 * @param  Height ：区域高度 
 * @param  pic_name ：bmp文件名 
 * @param  filename ：rgb数据文件名 
 * @retval 
  *   该参数为以下值之一：
  *     @arg 0 :转换成功
  *     @arg -1 :转换失败
 */
int rgb_to_bmp( uint16_t x, uint16_t y, uint16_t Width, uint16_t Height, char *pic_name, char * filename)
{
	/* bmp  文件头 54个字节 */
	unsigned char header[54] =
	{
		0x42, 0x4d, 0, 0, 0, 0, 
		0, 0, 0, 0, 54, 0, 
		0, 0, 40,0, 0, 0, 
		0, 0, 0, 0, 0, 0, 
		0, 0, 1, 0, 24, 0, 
		0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 
		0, 0, 0
	};
	
	int i;
	int j;
	long file_size;     
	long width;
	long height;
	unsigned char r,g,b;	
	unsigned int mybw;
	char kk[4]={0,0,0,0};
  
  uint16_t color_data[1];
  unsigned int read_num;
	
	uint8_t ucAlign;//
	
	
	/* 宽*高 +补充的字节 + 头部信息 */
	file_size = (long)Width * (long)Height * 3 + Height*(Width%4) + 54;		

	/* 文件大小 4个字节 */
	header[2] = (unsigned char)(file_size &0x000000ff);
	header[3] = (file_size >> 8) & 0x000000ff;
	header[4] = (file_size >> 16) & 0x000000ff;
	header[5] = (file_size >> 24) & 0x000000ff;
	
	/* 位图宽 4个字节 */
	width=Width;	
	header[18] = width & 0x000000ff;
	header[19] = (width >> 8) &0x000000ff;
	header[20] = (width >> 16) &0x000000ff;
	header[21] = (width >> 24) &0x000000ff;
	
	/* 位图高 4个字节 */
	height = Height;
	header[22] = height &0x000000ff;
	header[23] = (height >> 8) &0x000000ff;
	header[24] = (height >> 16) &0x000000ff;
	header[25] = (height >> 24) &0x000000ff;
		
	/* 新建一个文件 */
	bmpres = f_open( &bmpfsrc , (char*)pic_name, FA_CREATE_ALWAYS | FA_WRITE );
  if (bmpres == FR_OK)
  {
    printf("打开文件 %s 成功\r\n", pic_name);
  }
  else
  {
    printf("删除失败->(%d)\r\n", bmpres);
  }
  
  bmpres = f_open( &bmpfdst , (char *)filename, FA_OPEN_EXISTING | FA_READ);	    // 打开rgb数据文件
  if (bmpres == FR_OK)
  {
    printf("打开文件 %s 成功\r\n", filename);
  }
  else
  {
    printf("删除失败->(%d)\r\n", bmpres);
  }
	
	/* 新建文件之后要先关闭再打开才能写入 */
	f_close(&bmpfsrc);
		
	bmpres = f_open( &bmpfsrc , (char*)pic_name,  FA_OPEN_EXISTING | FA_WRITE);
  if (bmpres == FR_OK)
  {
    printf("打开文件 %s 成功\r\n", pic_name);
  }
  else
  {
    printf("删除失败->(%d)\r\n", bmpres);
  }

	if ( bmpres == FR_OK )
	{    
		/* 将预先定义好的bmp头部信息写进文件里面 */
		bmpres = f_write(&bmpfsrc, header,sizeof(unsigned char)*54, &mybw);		
			
		ucAlign = Width % 4;
    
    /* 跳过前面的头，直接从像素数据开始读 */
    f_lseek ( & bmpfdst, 10);
		
		for(i=0; i<Height; i++)					
		{
			for(j=0; j<Width; j++)  
			{
        /* 读取一个RGB565到 color_data */
        f_read ( & bmpfdst, color_data, 2, &read_num);
				//read_data = ILI9341_GetPointPixel ( x + j, y + Height - 1 - i );	    // 读一个RGB565像素				
				
        /* 拆分为 R G B */
				r =  GETR_FROM_RGB16(color_data[0]);
				g =  GETG_FROM_RGB16(color_data[0]);
				b =  GETB_FROM_RGB16(color_data[0]);

        /* 写入文件 */
				bmpres = f_write(&bmpfsrc, &b,sizeof(unsigned char), &mybw);
				bmpres = f_write(&bmpfsrc, &g,sizeof(unsigned char), &mybw);
				bmpres = f_write(&bmpfsrc, &r,sizeof(unsigned char), &mybw);
			}
				
			if( ucAlign )				/* 如果不是4字节对齐 */
				bmpres = f_write ( & bmpfsrc, kk, sizeof(unsigned char) * ( ucAlign ), & mybw );

		}/* 截屏完毕 */

		bmpres = f_close(&bmpfsrc); 
    if (bmpres == FR_OK)
    {
      printf("关闭文件成功\r\n");
    }
    else
    {
      printf("删除失败->(%d)\r\n", bmpres);
    }
  
    bmpres = f_close(&bmpfdst); 
    if (bmpres == FR_OK)
    {
      printf("关闭文件成功\r\n");
    }
    else
    {
      printf("删除失败->(%d)\r\n", bmpres);
    }
		
		return 0;
		
	}	
	else/* 截屏失败 */
		return -1;
}
