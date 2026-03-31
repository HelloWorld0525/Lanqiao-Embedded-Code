#ifndef __FUN_H
#define __FUN_H

#include "stm32g4xx.h"
#include "stdio.h"
#include "string.h"
#include "stdint.h"

#include "main.h"
#include "gpio.h"
#include "tim.h"
#include "adc.h"
//#include "usart.h"
#include "rtc.h"

#include "fun.h"
#include "lcd.h"
//#include "i2c.h"

void key_scan(void);
void lcd_show(void);
void led_Disp(uint8_t ucLed);
double get_vol(ADC_HandleTypeDef *hadc);
void lcd_pwm(void);

#endif
