/*******************************************************************************
  * @file    	NA_GNS_U237.h
  * @author  	Lv
  * @version 	V2.03
  * @date    	06-June-2016
  * @brief   	This file contains all the initial configration for the systerm parameters. 
  *******************************************************************************/

#ifndef __A_NA_GNS_U237_H
#define __A_NA_GNS_U237_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "A_System_Globle.h"
#include "S_ABC_UART.h"
#include "B_GNS_Process.h"


#define UN237FiFo_LEN 120

/*
UN237 运行状态标记数据
*/
typedef struct 
{
	u8 U_NAV_Flag;		/*UN237配置标记*/	
	u8 U_CMD_Idex; 		/*UN237配置指令索引*/
	u8 U_BRD_Indx;		/*528G  板卡串口波特率配置索引*/
	u8 U_BRD_Flag;		/*司南RTCM波特率配置标记*/

}UN237_Run_struct;


typedef struct 
{
	u8 U_NUM_L1;		/*GPS  L1  频点收星数*/	
	u8 U_NUM_L2;		/*GPS  L2  频点收星数*/	
	u8 U_NUM_G1;		/*GLO  G1  频点收星数*/	
	u8 U_NUM_G2;		/*GLO  G2  频点收星数*/	
	u8 U_NUM_B1;		/*BD2  B1  频点收星数*/	
	u8 U_NUM_B2;		/*BD2  B2  频点收星数*/	
	u8 U_NUM_B3;		/*BD2  B3  频点收星数*/	
	u8 U_NUM_re;		/*保留*/	

	u8 U_QUA_L1;		/*GPS  L1  频点收星质量*/ 
	u8 U_QUA_L2;		/*GPS  L2  频点收星质量*/ 
	u8 U_QUA_G1;		/*GLO  G1  频点收星质量*/ 
	u8 U_QUA_G2;		/*GLO  G2  频点收星质量*/ 
	u8 U_QUA_B1;		/*BD2  B1  频点收星质量*/ 
	u8 U_QUA_B2;		/*BD2  B2  频点收星质量*/ 
	u8 U_QUA_B3;		/*BD2  B3  频点收星质量*/ 
	u8 U_QUA_re;		/*保留*/	
}UN237_Star_struct;

/*++++++++++++++++++++++++++++++++++++*/
/*          	GNSS导航数据          */
/*++++++++++++++++++++++++++++++++++++*/
typedef struct 
{
	u8 	head[4];
	
	u8 lenth;
	u8 type;
	u8 remain_1;
	u8 remain_2;
	
	u8 year;
	u8 month;
	u8 day;
	u8 hour;
	
	u8 minute;
	u8 second;
	u8 RTK_status;
	u8 sat_number;
	
	double lati;
	double lonti;
	double height;
	double POSI_X;
	double POSI_Y;
	double POSI_Z;
	
	float Speed;
	float Heading;
	float head_dot;
	float head_dot2;
	float pitch;
	float roll;
	float rtk_delay;
	float remain_f1;  /*HDOP值*/
	float remain_f2;  /*电台信号强度*/
	float remain_f3;
	float remain_f4;
	float remain_f5;
	
	u8 radio_channel;
	u8 err;
	u8 warming;
	u8 remain_3;
	
	u8 remain_4;
	u8 remain_5;
	u8 remain_6;

	u8 GGA[96];
	u8 Checksum;
}GNSS_U237_Struct;



void U237_DATA_Initiall(void);
void U237_Bord_Configure(void);
void UN237_Initial(void);
void GNS_U237_Config(void);
uint8_t UN237_Congfigure(void);
uint8_t UN237_Data_Check(void);
void U237_ISR_Process(char *P_src,u8 *P_des);
void GNS_U237_Process(void);

uint8_t GNS_UN237_Receive(char *P_G);




#ifdef __cplusplus
}
#endif

#endif 



/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/******************* (C) COPYRIGHT 2016 STMicroelectronics *****END OF FILE****/

