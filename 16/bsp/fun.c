#include "fun.h"

uint8_t test;

uint8_t B1_state;
uint8_t B1_last_state=1;
uint8_t B2_state;
uint8_t B2_last_state=1;
uint8_t B3_state;
uint8_t B3_last_state=1;
uint8_t B4_state;
uint8_t B4_last_state=1;

uint8_t lcd_text[25];
uint8_t lcd_ui=0;					//0--2

RTC_TimeTypeDef sTime = {0};
RTC_DateTypeDef sDate = {0};

uint8_t lcd_ui3;					//0--3
uint8_t lcd_ui_old=0;

uint8_t i;

uint32_t fre,capture_value;	//频率和捕获值
uint8_t lcd_temp;

//PWM 监控界面
uint16_t CF;
uint8_t CD;
uint16_t DF;
uint8_t ST=0;					//0不锁 1锁

double R37_DS_DR;
double R38_FS_FR;

//RECD统计界面
uint16_t CF_RECD;
uint8_t CD_RECD;
uint16_t DF_RECD;
int16_t XF;
int16_t XF_RECD;
int8_t PA15_PA17_1000;	//0正常 1异常
int8_t RECD_lock;
RTC_TimeTypeDef sTime_RECD = {0};
RTC_DateTypeDef sDate_RECD = {0};


//PAPR 参数界面
uint8_t DS=1;
uint8_t DR=80;
uint16_t FS=100;
uint16_t FR=2000;
uint8_t DS_t=1;
uint8_t DR_t=80;
uint16_t FS_t=100;
uint16_t FR_t=2000;



void key_scan(void)	//此函数添加到while中
{
	B1_state = HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0);
	B2_state = HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_1);
	B3_state = HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_2);
	B4_state = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0);

	if(B1_state == 0 && B1_last_state ==1)		//按键B1按下
	{
		lcd_ui_old=lcd_ui;
		lcd_ui++;
		if(lcd_ui==3) lcd_ui=0;
		
		if(lcd_ui_old==2)
		{
			if((DS_t+10) <= DR_t && (FS_t + 1000) <= FR_t )
			{
				DS=DS_t;
				DR=DR_t;
				FS=FS_t;
				FR=FR_t;
			}
		}
	}

	if(lcd_ui==0)
	{
			if(B2_state == 0 && B2_last_state ==1)		//按键B2按下
			{
				TIM4->CNT=0;	
			}
			else if(B2_state ==0 && B2_last_state==0)	//按键B2一直按着
			{
					if(TIM4->CNT >=20000)	//按键B2长按2s
					{
						MX_RTC_Init();
					}
			}
			else if(B2_state ==1 && B2_last_state ==0)	//按键B2松开
			{
					if(TIM4->CNT <20000)	//按键B2短按
					{
						ST = ! ST;
					}
			}
	}
	else if(lcd_ui==2)
	{
			if(B2_state ==0 && B2_last_state ==1)
			{
				lcd_ui3++;
				if(lcd_ui3==4) lcd_ui3=0;
			}
			if(B3_state ==0 && B3_last_state ==1)
			{
				switch(lcd_ui3)
				{
					case 0:
					{
						DS_t++;
					}
					break;
					
					case 1:
					{
						DR_t+=10;
					}
					break;
					
					case 2:
					{
						FS_t+=100;
					}
					break;
					
					case 3:
					{
						FR_t+=1000;
					}
					break;
				}
			}
			if(B4_state ==0 && B4_last_state ==1)
			{
				switch(lcd_ui3)
				{
					case 0:
					{
						DS_t--;
					}
					break;
					
					case 1:
					{
						DR_t-=10;
					}
					break;
					
					case 2:
					{
						FS_t-=100;
					}
					break;
					
					case 3:
					{
						FR_t-=1000;
					}
					break;
				}
			}
	}


	
	B1_last_state = B1_state;
	B2_last_state = B2_state;
	B3_last_state = B3_state;
	B4_last_state = B4_state;
}

//方案1
void lcd_pwm1(void)
{
	 R37_DS_DR=(DR-10)/DS +1;	//份数
	 R38_FS_FR=(FR-1000)/FS +1;
	if(!ST)
	{
		for( i = DR/DS; i>0 ;i--)
		{
			if(get_vol(&hadc2)>(3.3/R37_DS_DR)*i)
			{
				CD =10+DS*i;
				TIM3 ->CCR2=CD;
				break;
			}
		}
		if(i==0)
		{
			CD =10;
			TIM3 ->CCR2=CD;
		}
		
			for(i =R38_FS_FR; i>0 ;i--)
		{
			if(get_vol(&hadc1)>(3.3/R38_FS_FR)*i)
			{
				CF =1000+FS*i;
				TIM3->PSC = 80000000/(CF*100);
				break;
			}
		}
		if(i==0)
		{
			CF =1000;
			TIM3->PSC = 80000000/(CF*100);
		}
	}
	
	
}

void lcd_pwm2(void)
{
	if(!ST)
	{
		// 1. 获取当前电压
		double vol_R37 = get_vol(&hadc2);
		double vol_R38 = get_vol(&hadc1);
		
		// 2. 计算总份数 (区间数量)
		// 公式：(目标最大值 - 基础起始值) / 步长 + 1
			uint8_t parts_CD = (DR - 10) / DS + 1; 
			uint8_t parts_CF = (FR - 1000) / FS + 1; 

		// 3. 计算每份的电压跨度 (比如 3.3 / 8 = 0.4125V)
		double step_vol_CD = 3.3 / parts_CD;
		double step_vol_CF = 3.3 / parts_CF;
		
		// 4. 计算当前电压落在了“第几份” (区间索引 i: 0 到 parts-1)
		// 用当前电压除以步进电压，直接得到当前的阶梯索引 i
		uint8_t i_CD = (uint8_t)(vol_R37 / step_vol_CD);
		uint8_t i_CF = (uint8_t)(vol_R38 / step_vol_CF);
		
		// 5. 临界值保护（当电压正好旋到3.3V满偏时，防止索引越界）
		if(i_CD >= parts_CD) i_CD = parts_CD - 1;
		if(i_CF >= parts_CF) i_CF = parts_CF - 1;
		
		// 6. 根据所在的阶梯份数，计算最终的 CD 和 CF
		CD = 10 + DS * i_CD;     // 比如 i=0 时 CD=10; i=1 时 CD=20
		CF = 1000 + FS * i_CF;
		
		// 7. 写入定时器寄存器更新 PWM
		TIM3->CCR2 = CD;
		TIM3->PSC = 80000000 / (CF * 100) - 1; // 预分频器通常需要 -1，看你定时器初始化的ARR是多少
	}
}


void lcd_recd(void)
{
	if(DF-CF>1000 ) 
	{
		PA15_PA17_1000=1;		//异常
		XF=DF-CF;
	}
	else if(CF-DF>1000)
	{
		PA15_PA17_1000=1;
		XF=CF-DF;
	}
	else
	{
		PA15_PA17_1000=0;		//正常
		RECD_lock=0;
	}
	
	if(PA15_PA17_1000 && RECD_lock==0)
	{
		RECD_lock=1;	//第一次进入异常程序进行记录
		XF_RECD=XF;
		CF_RECD=CF;
		CD_RECD=CD;
		DF_RECD=DF;
		sTime_RECD= sTime;
		sDate_RECD= sDate;
	}

}
void lcd_show(void)
{
	lcd_pwm1();
	lcd_recd();
	
	switch(lcd_ui)
	{
		case 0:
		{
			if(ST && PA15_PA17_1000) led_Disp(0x07);
			else if( ST) led_Disp(0x03);
			else if(PA15_PA17_1000) led_Disp(0x5);
			else	led_Disp(0x01);
			
			HAL_RTC_GetTime(&hrtc,&sTime,RTC_FORMAT_BIN);
			HAL_RTC_GetDate(&hrtc,&sDate,RTC_FORMAT_BIN);
			LCD_DisplayStringLine(Line1,(uint8_t *)"       PWM          ");
			sprintf((char *)lcd_text,"   CF=%1dHz           ",CF);
			LCD_DisplayStringLine(Line3,lcd_text);
			sprintf((char *)lcd_text,"   CD=%1d%%            ",CD);
			LCD_DisplayStringLine(Line4,lcd_text);
			sprintf((char *)lcd_text,"   DF=%1dHz           ",DF);
			LCD_DisplayStringLine(Line5,lcd_text);
			if(!ST)
				LCD_DisplayStringLine(Line6,(uint8_t *)"   ST=UNLOCK        ");
			else
				LCD_DisplayStringLine(Line6,(uint8_t *)"   ST=LOCK          ");
			sprintf((char *)lcd_text,"   %02dH%02dM%02dS        ",sTime.Hours,sTime.Minutes,sTime.Seconds);
			LCD_DisplayStringLine(Line7,lcd_text);
		}
		break;
		
		case 1:
		{
			if(ST && PA15_PA17_1000) led_Disp(0x06);
			else if(ST) led_Disp(0x02);
			else if(PA15_PA17_1000) led_Disp(0x4);
			else	led_Disp(0x00);
			
			HAL_RTC_GetTime(&hrtc,&sTime,RTC_FORMAT_BIN);
			HAL_RTC_GetDate(&hrtc,&sDate,RTC_FORMAT_BIN);
			LCD_DisplayStringLine(Line1,(uint8_t *)"       RECD         ");
			sprintf((char *)lcd_text,"   CF=%1dHz           ",CF_RECD);
			LCD_DisplayStringLine(Line3,lcd_text);
			sprintf((char *)lcd_text,"   CD=%1d%%           ",CD_RECD);
			LCD_DisplayStringLine(Line4,lcd_text);
			sprintf((char *)lcd_text,"   DF=%1dHz           ",DF_RECD);
			LCD_DisplayStringLine(Line5,lcd_text);
			sprintf((char *)lcd_text,"   XF=%1dHz           ",XF_RECD);
			LCD_DisplayStringLine(Line6,lcd_text);
			sprintf((char *)lcd_text,"   %02dH%02dM%02dS        ",sTime_RECD.Hours,sTime_RECD.Minutes,sTime_RECD.Seconds);
			LCD_DisplayStringLine(Line7,lcd_text);
		}
		break;
		
		default:
		{
			if(ST && PA15_PA17_1000) led_Disp(0x06);
			else if( ST) led_Disp(0x02);
			else if(PA15_PA17_1000) led_Disp(0x4);
			else	led_Disp(0x00);
		
			LCD_DisplayStringLine(Line1,(uint8_t *)"       PARA         ");
			sprintf((char *)lcd_text,"   DS=%1d%%            ",DS_t);					//重点关注此处初始化分值
			LCD_DisplayStringLine(Line3,lcd_text);
			sprintf((char *)lcd_text,"   DR=%1d%%            ",DR_t);				//
			LCD_DisplayStringLine(Line4,lcd_text);
			sprintf((char *)lcd_text,"   FS=%1dHz           ",FS_t);
			LCD_DisplayStringLine(Line5,lcd_text);
			sprintf((char *)lcd_text,"   FR=%1dHz           ",FR_t);
			LCD_DisplayStringLine(Line6,lcd_text);
			LCD_DisplayStringLine(Line7,(uint8_t *)"                    ");
		}
		break;
	}
}

void led_Disp(uint8_t ucLed)
{
	//**将所有灯熄灭
	HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15|GPIO_PIN_8
                          |GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12, GPIO_PIN_SET);//不是GPIO_PIN_ALL（控制高8位的LED）
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);	//根据ucLed的数值点亮相应的灯
	HAL_GPIO_WritePin(GPIOC,ucLed<<8,GPIO_PIN_RESET);//控制的LED处于高8位（8~18），所以需要将ucLed左移8位
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD,GPIO_PIN_2,GPIO_PIN_RESET);
}


double get_vol(ADC_HandleTypeDef *hadc)	//定义读取电压的函数(# ADC 模拟信号输入 R37 R38)
{
	HAL_ADC_Start(hadc);
	uint32_t adc_value = HAL_ADC_GetValue(hadc);	
	return 3.3 * adc_value /4096;
}
//使用方法： sprintf(text,"R37_VOL:%.2f",get_vol(&hadc2));		//读取电压、1是R38 2是R37


//输入捕获中断回调函数
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	if(htim -> Instance== TIM2)
	{
		//capture_value = HAL_TIM_ReadCapturedValue(htim,TIM_CHANNEL_1);
		capture_value = TIM2->CCR1;			//在捕获到上升沿时，会将CNT赋值给CCR
		TIM2->CNT =0;							//计数器清零
		fre = 80000000 / (80 * capture_value);	//输入捕获频率
		DF=fre;
		
		
	}
}
