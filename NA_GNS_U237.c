/******************************************************************************
  * @file    NA_GNS_U237.c
  * @author  Lv
  * @version V2.03
  * @date    	06-June-2016
  * @brief   This file contains the MAIN parameters which the system tasks need. 
  ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "NA_GNS_U237.h"


static UN237_Run_struct UN237_Flag;
static GNSS_U237_Struct UN237_Data;
static UN237_Star_struct  U_Star_P;
static UN237_Star_struct  U_Star_S;

/*计算校验和:  异或和*/
u8 OXR_AND_Check(u8 *P_start,u16 len)
{
	u8 u8_Return=0;
	u16 i=0;

	u8_Return = *P_start;
	for(i=1; i<(len-1); i++)
	{
		u8_Return = u8_Return^(*(P_start+i));
	}

	return u8_Return;
}


/*******************************************************************************
* Function Name :void U237_DATA_Initiall(void)
* Description   :UN237 板卡运行变量初始化
* Other          :None
* Date           :2016.06.03
*******************************************************************************/
void U237_DATA_Initiall(void)
{
	memset((void *)&UN237_Flag,0,sizeof(UN237_Run_struct));
}

/*******************************************************************************
* Function Name :void UN237_Configure(void)
* Description   :UN237 板卡配置指令发送
* Input          :None
* Output        :None
* Other          :None
* Date           :2016.05.30
*******************************************************************************/
static void UN237_Configure(void)
{
	switch(UN237_Flag.U_CMD_Idex)
	{
		case 0:
			Serial1PutString("$JBAUD,38400,PORTC\r\n");
			break;
			
		case 1:
			Serial1PutString("$JATT,MOVEBAS,YES\r\n");
			break;
				
		case 2:
			Serial1PutString("$JASC,GPGGA,10,PORTA\r\n");
			break;

		case 3:
			Serial1PutString("$JASC,GPRMC,10,PORTA\r\n");
			break;

		case 4:
			Serial1PutString("$JASC,GPHPR,10,PORTA\r\n");
			break;

		case 5:
			Serial1PutString("$JASC,PSAT,BLV,10,PORTA\r\n");
			break;

		case 6:
			Serial1PutString("$JDIFF,RTK\r\n");
			break;

		case 7:
			Serial1PutString("$JBAUD,230400,PORTB\r\n");        //惯导用
			break;
			
		case 8:
			Serial1PutString("$JSAVE\r\n");
			break;

		case 9:
			UN237_Flag.U_NAV_Flag = 0;
			break;
			
		default:
			UN237_Flag.U_CMD_Idex = 0;
			break;
	}
}


/*******************************************************************************
* Function Name :void UN237_Confirm(void)
* Description   :UN237 板卡配置返回确认
* Input          :None
* Output        :None
* Other          :None
* Date           :2016.05.30
*******************************************************************************/
static uint8_t UN237_Confirm(void)
{
	u16 i=0;
	uint8_t status=1;
	
	switch(UN237_Flag.U_CMD_Idex)
	{
		case 0:
			if(NULL != strstr((const char *)GNSS_CYCLE_FIFO, "$>JBAUD,38400,PORTC"))
			{
				UN237_Flag.U_CMD_Idex++;
				status=0;
			}
			break;
			
		case 1:
			if(NULL != strstr((const char *)GNSS_CYCLE_FIFO, "$>JATT,MOVEBAS,OK"))
			{
				UN237_Flag.U_CMD_Idex++;
				status=0;
			}
			break;
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			if(NULL != strstr((const char *)GNSS_CYCLE_FIFO, "$>JBAUD,230400,PORTB"))
			{
				UN237_Flag.U_CMD_Idex++;
				status=0;
			}
			break;
		case 8:
			if((NULL != strstr((const char *)(GNSS_CYCLE_FIFO+i), "$>\r\n")))
			{
				UN237_Flag.U_CMD_Idex++;
				status=0;
			}			
			break;
			
		default:
			break;
	}

	return status;
}

/*******************************************************************************
* Function Name :uint8_t UN237_Congfigure(void)
* Description   :UN237 板卡初始化
* Input          :None
* Output        :None
* Other          :None
* Date           :2016.06.07
*******************************************************************************/
uint8_t UN237_Congfigure(void)
{
	static uint8_t NAV_UN237_Flag = 0;/*UN237 配置控制变量*/
	uint8_t status = 1;	

	SysTime.SysTick_EndCFG = 0;
	UN237_Flag.U_CMD_Idex = 0;
	UN237_Flag.U_NAV_Flag = 1;

	while(UN237_Flag.U_NAV_Flag)
	{
		if((SysTime.SysTick_Cfg_GNS<=2)&&(NAV_UN237_Flag==0))
		{
			memset(GNSS_CYCLE_FIFO,0x0,GNSS_LEN);
			G2P_R_cnt = 0;	
			
			UN237_Configure();
			NAV_UN237_Flag = 1;
		}
		/*=== 检查返回的状态===*/
		if((NAV_UN237_Flag==1)&&(SysTime.SysTick_Cfg_GNS>=10))
		{
			if(0 == UN237_Confirm())
			{
				NAV_UN237_Flag = 0;
				SysTime.SysTick_Cfg_GNS=0;
				status = 0;
			}
			else if(SysTime.SysTick_Cfg_GNS>=38)
			{
				NAV_UN237_Flag = 0;
				SysTime.SysTick_Cfg_GNS=0;
				UN237_Flag.U_NAV_Flag = 0;
				status = 1;
			}
		}

		if(SysTime.SysTick_EndCFG>=600)/*3s内UN237 还没有配置成功*/	
		{
			UN237_Flag.U_NAV_Flag = 0;
			status = 1;
		}
	}

	return status;
}


/*******************************************************************************
* Function Name :void UN237_Data_Check(void)
* Description   :UN237 板卡数据检测
* Input          :None
* Output        :None
* Other          :None
* Date           :2016.06.03
*******************************************************************************/
uint8_t UN237_Data_Check(void)
{
	char *P_F_GGA = NULL;
	char *P_F_RMC = NULL;
	char *P_F_HPR = NULL;
	char *P_F_BLV = NULL;
	uint8_t status=0;

	/*---检测200ms 内若GNSS输出数据不正确，再配置GNSS---*/
	SysTime.SysTick_EndCFG = 0;
	while(SysTime.SysTick_EndCFG<40);

	/*===  检测GNSS数据是否正确===*/
	P_F_GGA = strstr((const char *)GNSS_CYCLE_FIFO, "$GPGGA");
	P_F_RMC = strstr((const char *)GNSS_CYCLE_FIFO, "$GPRMC");
	P_F_BLV = strstr((const char *)GNSS_CYCLE_FIFO, "$PSAT,BLV");	
	P_F_HPR = strstr((const char *)GNSS_CYCLE_FIFO, "$PSAT,HPR");
	if((P_F_GGA != NULL)&&(P_F_RMC != NULL)&&(P_F_HPR != NULL)&&(P_F_BLV != NULL))
	{/*数据正确*/
		memset(GNSS_CYCLE_FIFO,0x0,1024);
		G2P_R_cnt = 0;	
	}
	else
	{
		status = 1;
	}

	return status;
}



/*******************************************************************************
* Function Name :void UN237_Update_CFG(void)
* Description   :UN237 板卡升级配置
* Input          :None
* Output        :None
* Other          :None
* Date           :2016.05.30
*******************************************************************************/
static void UN237_Update_CFG(void)
{
	Serial1PutString("$JBAUD,19200,PORTA\r\n");
#if 0
	switch(UN237_Flag.U_CMD_Idex)
	{
		case 0:
			Serial6PutString("$JBAUD,19200,PORTA\r\n");
			break;
			
		case 1:
			Serial6PutString("$JSAVE\r\n");
			break;

		case 2:
			UN237_Flag.U_NAV_Flag = 0;
			break;
			
		default:
			UN237_Flag.U_CMD_Idex = 0;
			break;
	}
#endif
}


static uint8_t UN237_Update_Cfrm(void)
{
	u16 i=0;
	uint8_t status=1;

	for(i=0;i<1000;i++)
	{
		if((GNSS_CYCLE_FIFO[i]=='J')&&(GNSS_CYCLE_FIFO[i+2]=='A')&&(GNSS_CYCLE_FIFO[i+4]=='D'))

		{
			Initial_USART1_19200();
			status=0;
			break;
		}
	}
	
		return status;
}



/*******************************************************************************
* Function Name :void UN237_Data_Check(void)
* Description   :UN237 板卡数据检测
* Input          :None
* Output        :None
* Other          :None
* Date           :2016.06.03
*******************************************************************************/
uint8_t UN237_Upgrade_process(void)
{
	uint8_t status = 1;	

	static u8 cmd_send_flag=0;
	SysTime.SysTick_EndCFG = 0;
	SysTime.SysTick_Cfg_GNS= 0;
	UN237_Flag.U_CMD_Idex = 0;
	UN237_Flag.U_NAV_Flag = 1;

	while(UN237_Flag.U_NAV_Flag)
	{
		if((SysTime.SysTick_Cfg_GNS<=2)&&(cmd_send_flag==0))
		{
			memset(GNSS_CYCLE_FIFO,0x0,GNSS_LEN);
			G2P_R_cnt = 0;	
			
			UN237_Update_CFG();
			cmd_send_flag = 1;
		}
		/*=== 检查返回的状态===*/
		if((cmd_send_flag==1)&&(SysTime.SysTick_Cfg_GNS>=30))
		{
			if(0 == UN237_Update_Cfrm())
			{
				cmd_send_flag = 0;
				SysTime.SysTick_Cfg_GNS=0;
				UN237_Flag.U_NAV_Flag = 0;
				status = 0;
			}
			else if(SysTime.SysTick_Cfg_GNS>=38)
			{
				cmd_send_flag = 0;
				SysTime.SysTick_Cfg_GNS=0;
				UN237_Flag.U_NAV_Flag = 0;
				status = 1;
			}
		}
		
		if(SysTime.SysTick_EndCFG>=400)/*2s内UN237 还没有配置成功*/ 
		{
			UN237_Flag.U_NAV_Flag = 0;
			status = 1;
		}	
	}

	return status;
}


/*******************************************************************************
* Function Name :void UN237_RTCM_Cfirm(void)
* Description   :UN237 板卡RTCM  差分数据接收检测
* Input          :None
* Output        :None
* Other          :None
* Date           :2016.06.03
*******************************************************************************/
uint8_t UN237_RTCM_Cfirm(void)
{
	uint8_t	status=1;
	uint16_t i=0;

	for(i=0;i<GNSS_LEN;i++)
	{
		if((0xD3 == GNSS_CYCLE_FIFO[i])&&(0x00 == GNSS_CYCLE_FIFO[i+1]))
		{
			status=0;
			break;
		}
	}

	return status;
}


/*******************************************************************************
* Function Name :void UN237_RTCM_Check(void)
* Description   :UN237 板卡RTCM  差分数据检测
* Input          :None
* Output        :None
* Other          :None
* Date           :2016.06.03
*******************************************************************************/
uint8_t UN237_RTCM_Check(void)
{
	uint8_t status = 1;
	
	memset(GNSS_CYCLE_FIFO,0x0,GNSS_LEN);
	G2P_R_cnt = 0;	
	SysTime.SysTick_Cfg_GNS = 0;
	Serial1PutString("$JCONN,PORTA,PORTC\r\n");/*连接A  口和 C  口C口收到的RTCM  数据将从A 口输出*/
	
	/*=== 检查返回的状态===*/
	while(SysTime.SysTick_Cfg_GNS <= 28);
	if(NULL != strstr((const char *)GNSS_CYCLE_FIFO, "$>"))
	{
		status=0;
	}

	return status;
}



/*******************************************************************************
* Function Name :void UN237_RTCM_Eeend(void)
* Description   :UN237 板卡RTCM  差分数据检测结束
* Input          :None
* Output        :None
* Other          :None
* Date           :2016.06.03
*******************************************************************************/
uint8_t UN237_RTCM_Eeend(void)
{
	uint8_t status = 1;

	memset(GNSS_CYCLE_FIFO,0x0,GNSS_LEN);
	G2P_R_cnt = 0;	
	SysTime.SysTick_Cfg_GNS = 0;
	Serial1PutString("$JCONN,X\r\n");

	/*=== 检查返回的状态===*/
	while(SysTime.SysTick_Cfg_GNS <= 28);
	if(NULL != strstr((const char *)GNSS_CYCLE_FIFO, "$>"))
	{
		status=0;
	}

	return status;
}



/*******************************************************************************
* Function Name :void UN237_Star_CFG(void)
* Description   :UN237 板卡收星情况配置
* Input          :None
* Output        :None
* Other          :None
* Date           :2016.05.30
*******************************************************************************/
static void UN237_Star_CFG(void)
{
	switch(UN237_Flag.U_CMD_Idex)
	{
		case 0:
			Serial1PutString("$JASC,PSAT,RTKSTAT,1,PORTA\r\n");
			break;
			
		case 1:
			Serial1PutString("$JASC,PSAT,ATTSTAT,1,PORTA\r\n");
			break;

		default:
			UN237_Flag.U_CMD_Idex = 0;
			break;
	}

}

/*******************************************************************************
* Function Name :void UN237_Star_Confirm(void)
* Description   :UN237 板卡配置返回确认
* Input          :None
* Output        :None
* Other          :None
* Date           :2016.05.30
*******************************************************************************/
static uint8_t UN237_Star_Confirm(void)
{
	uint8_t status=1;
	
	switch(UN237_Flag.U_CMD_Idex)
	{
		case 0:
			if(NULL != strstr((const char *)GNSS_CYCLE_FIFO, "$>"))
			{
				UN237_Flag.U_CMD_Idex++;
				status=0;
			}
			break;
			
		case 1:
			if(NULL != strstr((const char *)GNSS_CYCLE_FIFO, "$>"))
			{
				UN237_Flag.U_NAV_Flag = 0;
				status=0;
			}
			break;
			
		case 2:		
			break;
			
		default:
			break;
	}

	return status;
}


/*******************************************************************************
* Function Name :uint8_t UN237_Star_Status(void)
* Description   :UN237 板卡收星状态配置
* Input          :None
* Output        :None
* Other          :None
* Date           :2016.06.07
*******************************************************************************/
uint8_t UN237_Star_Status(void)
{
	static uint8_t NAV_UN237_Flag = 0;/*UN237 配置控制变量*/
	uint8_t status = 1;	

	SysTime.SysTick_EndCFG = 0;
	SysTime.SysTick_Cfg_GNS= 0;
	UN237_Flag.U_CMD_Idex = 0;
	UN237_Flag.U_NAV_Flag = 1;

	while(UN237_Flag.U_NAV_Flag)
	{
		if((SysTime.SysTick_Cfg_GNS<=2)&&(NAV_UN237_Flag==0))
		{
			memset(GNSS_CYCLE_FIFO,0x0,GNSS_LEN);
			G2P_R_cnt = 0;	
			
			UN237_Star_CFG();
			NAV_UN237_Flag = 1;
		}
		/*=== 检查返回的状态===*/
		if((NAV_UN237_Flag==1)&&(SysTime.SysTick_Cfg_GNS>=10))
		{
			if(0 == UN237_Star_Confirm())
			{
				NAV_UN237_Flag = 0;
				SysTime.SysTick_Cfg_GNS=0;
				status = 0;
			}
			else if(SysTime.SysTick_Cfg_GNS>=38)
			{
				NAV_UN237_Flag = 0;
				SysTime.SysTick_Cfg_GNS=0;
				UN237_Flag.U_NAV_Flag = 0;
				status = 1;
			}
		}

		if(SysTime.SysTick_EndCFG>=200)/*1s内UN237 还没有配置成功*/	
		{
			UN237_Flag.U_NAV_Flag = 0;
			status = 1;
		}
	}

	return status;
}

/*******************************************************************************
* Function Name :uint8_t UN237_Star_Closes(void)
* Description   :UN237 板卡收星状态关闭
* Input          :None
* Output        :None
* Other          :None
* Date           :2016.06.07
*******************************************************************************/
uint8_t UN237_Star_Closes(void)
{
	uint8_t status = 0;	

	SysTime.SysTick_Cfg_GNS= 0;
	Serial1PutString("$JASC,PSAT,RTKSTAT,0,PORTA\r\n");
	while(SysTime.SysTick_Cfg_GNS<=38);

	SysTime.SysTick_Cfg_GNS= 0;
	Serial1PutString("$JASC,PSAT,ATTSTAT,0,PORTA\r\n");
	while(SysTime.SysTick_Cfg_GNS<=38);

	return status;
}



///////////////////////////////////////////////////////////////////////////////////
//////////////////////////////                         ///////////////////////////////////////
///////////////////////////////数据解析///////////////////////////////////////
///////////////////////////////                     ////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
/*******************************************************************************
* Function Name :void UN237_process_GPGGA(void)
* Description   :UN237 板卡GGA数据提取
* Input          :None
* Output        :None
* Other          :None
* Date           :2016.05.30
*******************************************************************************/
static void UN237_process_GPGGA(char *P_GPGGA,GNSS_U237_Struct *U_GGA)
{
	u8 i=0,dou=0;
	static u8 step=0;
	float Gtmp=0.0f;
	int latitude_Degree=0;
	double latitude_Cent_double=0.0;

	step=0;
	for(i=0;i<UN237FiFo_LEN;i++)
	{
		if(*(P_GPGGA+i) == ',')		dou++;

		if((dou==2)&&(0==step))		/*纬度数据*/	
		{
			if((*(P_GPGGA+i+2)>=0x30)&&(*(P_GPGGA+i+2)<=0x39))
			{
				latitude_Degree = (*(P_GPGGA+i+1)-0x30)*10.0+(*(P_GPGGA+i+2)-0x30);
				latitude_Cent_double = (*(P_GPGGA+i+3) - 0x30)*10.0 + (*(P_GPGGA+i+4) - 0x30) + \
								(*(P_GPGGA+i+6) - 0x30)/10.0 + (*(P_GPGGA+i+7) - 0x30)/100.0 + \
								(*(P_GPGGA+i+8) - 0x30)/1000.0 + (*(P_GPGGA+i+9) - 0x30)/10000.0 + \
								(*(P_GPGGA+i+10) - 0x30)/100000.0 + (*(P_GPGGA+i+11) - 0x30)/1000000.0 + \
								(*(P_GPGGA+i+12) - 0x30)/10000000.0 + (*(P_GPGGA+i+13) - 0x30)/100000000.0 ;

				U_GGA->lati = latitude_Degree+latitude_Cent_double/60.0;
			}
			else
			{
				U_GGA->err = 50;	/*主天线连接异常*/
			}
			step = 1;
		}
		
		if((dou==3)&&(1==step))		/**/
		{
			if(('N' == *(P_GPGGA+i+1))||('S' == *(P_GPGGA+i+1)))
			{
			}
			else
			{
				break;
			}
			step = 2;
		}

		if((dou==4)&&(2==step))		/*经度数据*/	
		{
			if((*(P_GPGGA+i+2)>=0x30)&&(*(P_GPGGA+i+2)<=0x39))
			{
				latitude_Degree = 0;
				latitude_Cent_double = 0;

				latitude_Degree = (*(P_GPGGA+i+1) - 0x30)*100 + (*(P_GPGGA+i+2) - 0x30)*10 + (*(P_GPGGA+i+3) - 0x30);
				latitude_Cent_double = (*(P_GPGGA+i+4) - 0x30)*10 + (*(P_GPGGA+i+5) - 0x30) + \
								(*(P_GPGGA+i+7) - 0x30)/10.0 + (*(P_GPGGA+i+8) - 0x30)/100.0 +	\
								(*(P_GPGGA+i+9) - 0x30)/1000.0 + (*(P_GPGGA+i+10) - 0x30)/10000.0 +	\
								(*(P_GPGGA+i+11) - 0x30)/100000.0 + (*(P_GPGGA+i+12) - 0x30)/1000000.0 +	\
								(*(P_GPGGA+i+13) - 0x30)/10000000.0 + (*(P_GPGGA+i+14) - 0x30)/100000000.0 ;

				U_GGA->lonti = latitude_Degree+latitude_Cent_double/60.0;
			}

			step = 3;
		}

		if((dou==5)&&(3==step))		/**/
		{
			if(('E' == *(P_GPGGA+i+1))||('W' == *(P_GPGGA+i+1)))
			{
			}
			else
			{
				break;
			}

			step = 4;
		}
		
		if((dou==6)&&(4==step))		/*RTK  状态*/	
		{
			if((*(P_GPGGA+i+1)>=0x30)&&(*(P_GPGGA+i+1)<=0x39))
			{
				U_GGA->RTK_status = *(P_GPGGA+i+1)-0x30;	/*0:无效解；1:单点定位；2:伪距差分；*//*4:固定解；5:浮动解*/
			}
			else
			{
				break;
			}

			step = 5;
		}

		if((dou==7)&&(5==step))		/*卫星数*/	
		{
			if((*(P_GPGGA+i+1)>=0x30)&&(*(P_GPGGA+i+1)<=0x39)&&(*(P_GPGGA+i+2)>=0x30)&&(*(P_GPGGA+i+2)<=0x39))
			{
				U_GGA->sat_number = (*(P_GPGGA+i+1) - 0x30)*10 + (*(P_GPGGA+i+2) - 0x30);

				if((0==U_GGA->sat_number)&&(0==U_GGA->RTK_status))
					U_GGA->err = 50;	/*主天线连接异常*/
			}
			else
			{
				break;
			}

			step = 6;
		}

		if((dou==8)&&(6==step))		/*HDOP  值*/	
		{
			if(*(P_GPGGA+i+1) != ',')
			{
				U_GGA->remain_f1 = atof(P_GPGGA+i+1);
			}

			step = 7;
		}
		
		if((dou==9)&&(7==step))		/*天线海拔高度值*/	
		{
			if(*(P_GPGGA+i+2) != ',')
			{
				Gtmp = atof(P_GPGGA+i+1);
			}

			step = 8;
		}

		if((dou==10)&&(8==step))		/*M*/
		{
			if('M' != *(P_GPGGA+i+1))
			{
				break;
			}

			step = 9;
		}

		if((dou==11)&&(9==step))		/*海平面分离度*/
		{
			if(',' != *(P_GPGGA+i+2))
			{
				U_GGA->height = atof(P_GPGGA+i+1)+Gtmp;/*椭球高度*/
			}

			step = 10;
		}

		if((dou==13)&&(10==step))		/*差分延迟*/
		{
			if(*(P_GPGGA+i+1) != ',')
			{
				U_GGA->rtk_delay = atof(P_GPGGA+i+1);
			}
			else
			{
				U_GGA->rtk_delay = 99;
			}	
			step = 11;
			break;
		}	
	}
}

/*******************************************************************************
* Function Name :void UN237_process_GPRMC(void)
* Description   :UN237 板卡RMC数据提取
* Input          :None
* Output        :None
* Other          :None
* Date           :2016.05.30
*******************************************************************************/
static void UN237_process_GPRMC(char *P_RMC,GNSS_U237_Struct *U_RMC)
{	
	u8 i=0,dou=0;
	static u8 step=0;

	step = 0;
	for(i=0;i<UN237FiFo_LEN;i++)
	{
		if(*(P_RMC+i) == ',')		dou++;

		if((1 == dou)&&(0==step))	/*时分秒*/
		{
			if(*(P_RMC+i+7) == '.')
			{
				U_RMC->hour 	= (*(P_RMC+i+1)-0x30)*10+(*(P_RMC+i+2)-0x30);/*时*/
				U_RMC->minute	= (*(P_RMC+i+3)-0x30)*10+(*(P_RMC+i+4)-0x30);/*分*/
				U_RMC->second	= (*(P_RMC+i+5)-0x30)*10+(*(P_RMC+i+6)-0x30);/*秒*/
			}
			else
			{
				break;
			}

			step = 1;
		}

		if((4 == dou)&&(1==step))		/**/
		{
			if(('N' == *(P_RMC+i+1))||('S' == *(P_RMC+i+1)))
			{
			}
			else
			{
				break;
			}

			step = 2;
		}

		if((6 == dou)&&(2==step))		/**/
		{
			if(('E' == *(P_RMC+i+1))||('W' == *(P_RMC+i+1)))
			{
			}
			else
			{
				break;
			}

			step = 3;
		}


		if((7 == dou)&&(3==step))		/*地面速度*/	
		{
			if(*(P_RMC+i+1) != ',')
			{
				U_RMC->Speed = atof(P_RMC+i+1)*0.51444444;
			}

			step = 4;
		}
		

		if((9 == dou)&&(4==step))
		{
			if(*(P_RMC+i+1) != ',')
			{
				U_RMC->day 	= (*(P_RMC+i+1)-0x30)*10+(*(P_RMC+i+2)-0x30); 	/*日*/
				U_RMC->month= (*(P_RMC+i+3)-0x30)*10+(*(P_RMC+i+4)-0x30);	/*月*/
				U_RMC->year	= (*(P_RMC+i+5)-0x30)*10+(*(P_RMC+i+6)-0x30);	/*年*/
			}

			step = 5;
			break;
		}
	}
}
	
/*******************************************************************************
* Function Name :void UN237_process_GPVTG(void)
* Description   :UN237 板卡航向数据提取
* Input          :None
* Output        :None
* Other          :None
* Date           :2016.05.30
*******************************************************************************/
static void UN237_process_GPHPR(char *P_hpr,GNSS_U237_Struct *U_HPR)
{
    u8 dou = 0,i = 0;
	static u8 step=0;

	step=0;
    for(i=0; i<UN237FiFo_LEN; i++)
    {
		if(*(P_hpr+i) == ',')	dou++;
	
		if((3 == dou)&&(0==step))
		{
			if(*(P_hpr+i+1) != ',')
			{
				U_HPR->Heading = atof(P_hpr+i+1);
			}
			else
			{
				if(U_HPR->err!=50)	U_HPR->err= 51;	/*副天线连接异常*/
			}
			step = 1;
		}

		if((4 == dou)&&(1==step))
		{
			if(*(P_hpr+i+1) != ',')
			{
				U_HPR->pitch = atof(P_hpr+i+1);
			}
			else
			{
				if(U_HPR->err!=50)	U_HPR->err= 56; /*没有侧倾角数据*/
			}
			step = 2;
			break;
		}
    }
}


/*******************************************************************************
* Function Name :void UN237_process_GPBLV(void)
* Description   :UN237 板卡BLV数据提取
* Input          :None
* Output        :None
* Other          :None
* Date           :2016.05.30
*******************************************************************************/
static void UN237_process_GPBLV(char *P_blv,GNSS_U237_Struct *U_BLV)
{
	u8 i=0,dou=0;
	static u8 step=0;

	step=0;
	for(i=0; i<UN237FiFo_LEN; i++)
    {
		if(*(P_blv+i) == ',')	dou++;
	
		if((4 == dou)&&(0==step))
		{
			if(*(P_blv+i+1) != ',')
			{
				U_BLV->POSI_Y = atof(P_blv+i+1);
			}

			step = 1;
		}

		if((5 == dou)&&(1==step))
		{
			if(*(P_blv+i+1) != ',')
			{
				U_BLV->POSI_X = atof(P_blv+i+1);
			}
			step = 2;
		}

		if((6 == dou)&&(2==step))
		{
			if(*(P_blv+i+1) != ',')
			{
				U_BLV->POSI_Z = atof(P_blv+i+1);
			}
			step = 3;
			break;
		}
	}		
}

/*******************************************************************************
* Function Name :void UN237_process_GPRTK(void)
* Description   :UN237 板卡天线收星情况提取
* Input          :None
* Output        :None
* Other          :None
* Date           :2017.05.30
*******************************************************************************/
static void UN237_process_GPRTK(char *P_rtk,UN237_Star_struct *U_RTK)
{
	u8 i=0,dou=0,j=0,shuN=0;
	static u8 step=0;
	u8 *P_num=&U_RTK->U_NUM_L1;
	u8 *P_qua=&U_RTK->U_QUA_L1;
	
	step=0;
	for(i=0; i<UN237FiFo_LEN; i++)
    {
		if(*(P_rtk+i) == '(')	dou++;
	
		if((2 == dou)&&(0==step))
		{	
			shuN=0;
			for(j=0;j<20;j++)
			{
				if(*(P_rtk+i+j) == ',')		/*找到一个有效数据的开始*/
				{
					
					if(*(P_rtk+i+j+2)==',')	/*有效数据最多两位*/
					{
						*(P_num+shuN) = (*(P_rtk+i+j+1)-0x30);
					}
					else
					{
						*(P_num+shuN) = (*(P_rtk+i+j+1)-0x30)*10+(*(P_rtk+i+j+2)-0x30); 					
					}

					*(P_rtk+i+j) = 0x30;
					shuN++;
				}	

				if(shuN>=7)	break;
			}
			
			step = 1;
		}

		if((3 == dou)&&(1==step))
		{
			for(shuN=0;shuN<7;shuN++)
			{
				*(P_qua+shuN) = *(P_rtk+i+2+2*shuN)-0x40;
			}

			step = 2;
			break;
		}
	}
}

#if 0

/*******************************************************************************
* Function Name :void Anomaly_Detection_data(void)
* Description   :接收数据异常值判断
* Input          :异常值FIFO，实时值，门限，跟踪尺度
* Output        :差值异常
* Other          :None
* Date           :2016.06.07
*******************************************************************************/
static u8 Anomaly_Detection_data(double *a,double *v,float gate,u8 *cnt)
{
	u8 i;
	double diff[5]={0},fsum=0.0,ftmp=0.0;
	u8 freturn;

	for(i=0;i<5;i++)
	{
		diff[i] = *(a+i+1)-*(a+i); 
		fsum 	= fsum+diff[i];
	}
	fsum = fsum/5.0;	/*差值的平均值*/
	ftmp = *v-*(a+5);	/*相近两次采集值差*/
	
	if(fabs(fsum-ftmp)>gate)	
	{
		*cnt = *cnt+1;
		if(*cnt > 2)/*跳变*/
		{
			*cnt = 0;
			freturn = 2;

			for(i=0;i<5;i++)
			{
				*(a+i) = *(a+i+1);	
			}
			*(a+4) = *v;
			*(a+5) = *v;
		}
		else	/*飞点*/
		{
			freturn = 1;
			*v = *(a+5);
		}		
	}
	else
	{
		freturn = 0;
		*cnt = 0;
		for(i=0;i<5;i++)
		{
			*(a+i) = *(a+i+1);	
		}
		*(a+5) = *v;
	}	
	
	return freturn;
}


/*******************************************************************************
* Function Name :void GNS_U237_DataProcess(void)
* Description   :UN237  板卡数据异常值检测
* Input          :如果在RTK异常情况下数据异常怎么办澹�
* Output        :飞点剔除，趋势跟踪
* Other          :None
* Date           :2016.06.14
*******************************************************************************/
void GNS_U237_DataProcess(UN237_struct *U_237)
{
	u8 status=0;

	/*定位数据异常检测*/
	status = Anomaly_Detection_data(laltitude,&U_237->U_GGA_Lati,0.2,SM_count+0);
	if(1 == status)	U_237->U_GNS_Worn = 54;
	else if(2 == status)	U_237->U_GNS_Eror = 52;

	status = Anomaly_Detection_data(longitude,&U_237->U_GGA_Logi,0.2,SM_count+1);
	if(1 == status)	U_237->U_GNS_Worn = 54;
	else if(2 == status)	U_237->U_GNS_Eror = 52;
	
	status = Anomaly_Detection_data(position_x,&U_237->U_BLV_PosX,2,SM_count+2);
	if(1 == status)	U_237->U_GNS_Worn = 54;
	else if(2 == status)	U_237->U_GNS_Eror = 52;

	status = Anomaly_Detection_data(position_y,&U_237->U_BLV_PosY,2,SM_count+3);
	if(1 == status)	U_237->U_GNS_Worn = 54;
	else if(2 == status)	U_237->U_GNS_Eror = 52;	

	status = Anomaly_Detection_data(position_z,(double *)&U_237->U_BLV_PosZ,2,SM_count+4);
	if(1 == status) U_237->U_GNS_Worn = 54;
	else if(2 == status)	U_237->U_GNS_Eror = 52; 

	/*姿态数据异常检测*/
	status = Anomaly_Detection_data(heading,(double *)&U_237->U_HPR_Hang,3,SM_count+5);
	if(1 == status) U_237->U_GNS_Worn = 55;
	else if(2 == status)	U_237->U_GNS_Eror = 53; 

	status = Anomaly_Detection_data(pitch,(double *)&U_237->U_HPR_Pich,0.5,SM_count+6);
	if(1 == status) U_237->U_GNS_Worn = 56;
	else if(2 == status)	U_237->U_GNS_Eror = 54; 

	/*速度数据异常检测*/
	status = Anomaly_Detection_data(speed_V,(double *)&U_237->U_RMC_Sped,0.5,SM_count+7);
	if(1 == status) U_237->U_GNS_Worn = 57;
	else if(2 == status)	U_237->U_GNS_Eror = 55; 
}
#endif

/*******************************************************************************
* Function Name :void UN237_Data_Pack(void)
* Description   :按格式打包237数据
* Input          :
* Output        :
* Other          :None
* Date           :2018.06.14
*******************************************************************************/
void UN237_Data_Pack(void)
{
	UN237_Data.head[0] = 'G';
	UN237_Data.head[1] = 'N';
	UN237_Data.head[2] = 'S';
	UN237_Data.head[3] = 'S';

	UN237_Data.Checksum = OXR_AND_Check((u8 *)&UN237_Data,sizeof(GNSS_U237_Struct));
}



char UN237_FIFO[120]={0};

/*******************************************************************************
* Function Name :void U237_ISR_Process(void)
* Description   :U237 板卡数据处理
* Input          :None
* Output        :None
* Other          :None
* Date           :2016.06.14
*******************************************************************************/
void U237_ISR_Process(char *P_src,u8 *P_des)
{
	char *ptr = NULL;
	
	ptr = strstr(P_src, "$GPGGA");
	if(ptr != NULL)
	memcpy(UN237_Data.GGA,(u8 *)ptr,96);	
		

		
	////////////////*解GGA  信息*///////////////////
	memset((void*)&UN237_FIFO,0x0,UN237FiFo_LEN);
	ptr = NULL;
	ptr = strstr((const char *)P_src, "$GPGGA");
	if((ptr != NULL) && ((ptr-P_src)<(GNSS_LEN-UN237FiFo_LEN)))
	{		
		memcpy(UN237_FIFO,ptr,UN237FiFo_LEN);		
		UN237_process_GPGGA(UN237_FIFO,&UN237_Data);
	}
	else
	{
		UN237_Data.err = 58;
	}
	
	////////////////*解RMC   信息*///////////////////
	memset((void*)&UN237_FIFO,0x0,UN237FiFo_LEN);
	ptr = NULL;
	ptr = strstr((const char *)P_src, "$GPRMC");
	if((ptr != NULL) && ((ptr-P_src)<(GNSS_LEN-UN237FiFo_LEN)))
	{			
		memcpy(UN237_FIFO,ptr,UN237FiFo_LEN);		
		UN237_process_GPRMC(UN237_FIFO,&UN237_Data);
	}
	else
	{
		UN237_Data.err = 59;
	}

	////////////////*解BLV   信息*///////////////////
	memset((void*)&UN237_FIFO,0x0,UN237FiFo_LEN);
	ptr = NULL;
	ptr = strstr((const char *)P_src, "$PSAT,BLV");
	if((ptr != NULL)&&((ptr-P_src)<(GNSS_LEN-UN237FiFo_LEN)))
	{			
		memcpy(UN237_FIFO,ptr,UN237FiFo_LEN);		
		UN237_process_GPBLV(UN237_FIFO,&UN237_Data);
	}
	else
	{
	}

	///////////*解GPHPR  方位角信息*/////////////
	memset((void*)&UN237_FIFO,0x0,UN237FiFo_LEN);
	ptr = NULL;
	ptr = strstr((const char *)P_src, "$PSAT,HPR");
	if((ptr != NULL) && ((ptr-P_src)<(GNSS_LEN-UN237FiFo_LEN)))
	{
		memcpy(UN237_FIFO,ptr,UN237FiFo_LEN);		
		UN237_process_GPHPR(UN237_FIFO,&UN237_Data);
	}	
	else
	{
	}

	///////////*解主天线收星信息*/////////////
	memset((void*)&UN237_FIFO,0x0,UN237FiFo_LEN);
	ptr = NULL;
	//ptr = strstr((const char *)P_src, "$PSAT,RTKSTAT");
	if((ptr != NULL) && ((ptr-P_src)<(GNSS_LEN-UN237FiFo_LEN)))
	{
		memcpy(UN237_FIFO,ptr,UN237FiFo_LEN);		
		UN237_process_GPRTK(UN237_FIFO,&U_Star_P);
	}	

	///////////*解副天线收星信息*/////////////
	memset((void*)&UN237_FIFO,0x0,UN237FiFo_LEN);
	ptr = NULL;
	//ptr = strstr((const char *)P_src, "$PSAT,ATTSTAT");
	if((ptr != NULL) && ((ptr-P_src)<(GNSS_LEN-UN237FiFo_LEN)))
	{
		memcpy(UN237_FIFO,ptr,UN237FiFo_LEN);		
		UN237_process_GPRTK(UN237_FIFO,&U_Star_S);
	}	
	
	UN237_Data_Pack();
	memcpy(P_des,(u8 *)&UN237_Data,sizeof(GNSS_U237_Struct));	

	memset((void *)&UN237_Data,0,sizeof(GNSS_U237_Struct));
	memset((void*)P_src,0x0,GNSS_LEN);
}


/*******************************************************************************
* Function Name :void GNS_UN237_Receive(void)
* Description   :U237 板卡数据周期性接收
* Input          :None
* Output        :返回0  周期未到，返回1  周期到了
* Other          :None
* Date           :2016.06.14
*******************************************************************************/
uint8_t GNS_UN237_Receive(char *P_G)
{
	uint8_t status=0;
	
	if(NULL != strstr(P_G, "$PSAT,BLV"))
	{
		status = 1;
	}

	return status;
}



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


