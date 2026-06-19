/*
 * rtos_main.c
 *
 *  Created on: Jun 19, 2026
 *      Author: hsuanjung
 */

#include "stm32f446xx.h"
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

#include "timers.h"

#include "uros_init.h"
int a = 0;

void StartDefaultTask(void *argument){
	a = 1;
	uros_init();
	a = 2;
	for(;;){
		uros_agent_status_check();
		osDelay(1000/FREQUENCY);
		a++;
	}
}
