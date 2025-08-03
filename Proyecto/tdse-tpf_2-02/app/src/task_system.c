/*
 * Copyright (c) 2023 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @file   : task_system.c
 * @date   : Set 26, 2023
 * @author : Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>
 * @version	v1.0.0
 */

/********************** inclusions *******************************************/
/* Project includes. */
#include "main.h"

/* Demo includes. */
#include "logger.h"
#include "dwt.h"

/* External module includes. */
#include "i2c_lcd.h"
#include "keypad_4x4.h"

/* Application & Tasks includes. */
#include "board.h"
#include "app.h"
#include "task_system_attribute.h"
#include "task_system_interface.h"
#include "task_actuator_attribute.h"
#include "task_actuator_interface.h"
#include "ext_memory.h"

/********************** macros and definitions *******************************/
#define G_TASK_SYS_CNT_INI			0ul
#define G_TASK_SYS_TICK_CNT_INI		0ul

#define DEL_SYS_XX_MIN				0ul
#define DEL_SYS_XX_MED				50ul
#define DEL_SYS_XX_MAX				500ul

#define DEL_SYS_INIT				1500ul
#define DEL_ADC_READ				5000ul
#define DEL_RESET_STATE				5000ul

#define ADC_INITIAL_CALIBRATION		1500

/********************** internal data declaration ****************************/
task_system_dta_t task_system_dta =
	{DEL_SYS_INIT, ST_SYS_INIT, EV_SYS_XX_BTN_IDLE, false};

#define SYSTEM_DTA_QTY	(sizeof(task_system_dta)/sizeof(task_system_dta_t))

uint8_t wrong_tries = 0;
uint16_t reset_tick = 0;

char pwd_buffer[5] = "xxxxx";
uint8_t buffer_idx = 0;

memory_t ext_mem;

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
const char *p_task_system 		= "Task System (System Statechart)";
const char *p_task_system_ 		= "Non-Blocking & Update By Time Code";

/********************** external data declaration ****************************/
uint32_t g_task_system_cnt;
volatile uint32_t g_task_system_tick_cnt;

I2C_LCD_HandleTypeDef lcd1;

/********************** external functions definition ************************/
void task_system_init(void *parameters)
{
	task_system_dta_t 	*p_task_system_dta;
	task_system_st_t	state;
	task_system_ev_t	event;
	bool b_event;

	/* Print out: Task Initialized */
	LOGGER_LOG("  %s is running - %s\r\n", GET_NAME(task_system_init), p_task_system);
	LOGGER_LOG("  %s is a %s\r\n", GET_NAME(task_system), p_task_system_);

	g_task_system_cnt = G_TASK_SYS_CNT_INI;

	/* Print out: Task execution counter */
	LOGGER_LOG("   %s = %lu\r\n", GET_NAME(g_task_system_cnt), g_task_system_cnt);

	init_queue_event_task_system();

	/* Update Task Actuator Configuration & Data Pointer */
	p_task_system_dta = &task_system_dta;

	/* Print out: Task execution FSM */
	state = p_task_system_dta->state;
	LOGGER_LOG("   %s = %lu", GET_NAME(state), (uint32_t)state);

	event = p_task_system_dta->event;
	LOGGER_LOG("   %s = %lu", GET_NAME(event), (uint32_t)event);

	b_event = p_task_system_dta->flag;
	LOGGER_LOG("   %s = %s\r\n", GET_NAME(b_event), (b_event ? "true" : "false"));

	/* Init LCD Screen */
	lcd1.hi2c = &hi2c1;
	lcd1.address = 0x4E;
	lcd_init(&lcd1);
	lcd_clear(&lcd1);
	lcd_pos(&lcd1, 1, 5);
	lcd_puts(&lcd1, "BIENVENIDO");
	lcd_pos(&lcd1, 2, 1);
	lcd_puts(&lcd1, "Iniciando sistema");

	/* Init PWM */
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

	/* Init memory */
	#if MEMORY_CONNECTED
		// TODO: Read memory.
	#endif

	g_task_system_tick_cnt = G_TASK_SYS_TICK_CNT_INI;
}

void task_system_update(void *parameters)
{
	task_system_dta_t *p_task_system_dta;

	bool b_time_update_required = false;

	/* Update Task System Counter */
	g_task_system_cnt++;

	/* Protect shared resource (g_task_system_tick) */
	__asm("CPSID i");	/* disable interrupts*/
    if (G_TASK_SYS_TICK_CNT_INI < g_task_system_tick_cnt)
    {
    	g_task_system_tick_cnt--;
    	b_time_update_required = true;
    }
    __asm("CPSIE i");	/* enable interrupts*/

    while (b_time_update_required)
    {
		/* Protect shared resource (g_task_system_tick) */
		__asm("CPSID i");	/* disable interrupts*/
		if (G_TASK_SYS_TICK_CNT_INI < g_task_system_tick_cnt)
		{
			g_task_system_tick_cnt--;
			b_time_update_required = true;
		}
		else
		{
			b_time_update_required = false;
		}
		__asm("CPSIE i");	/* enable interrupts*/

    	/* Update Task System Data Pointer */
		p_task_system_dta = &task_system_dta;

		if (true == any_event_task_system())
		{
			p_task_system_dta->flag = true;
			p_task_system_dta->event = get_event_task_system();
		}

		// Handle LDR controlled system.
		if (ext_mem.system_status)
		{
			if (ext_mem.ldr_mode)
			{
				if (p_task_system_dta->tick == 0)
				{
					p_task_system_dta->tick = DEL_ADC_READ;

					HAL_ADC_Start(&hadc1);
					HAL_ADC_PollForConversion(&hadc1, 0);

					uint16_t adc_val = HAL_ADC_GetValue(&hadc1);

					if (adc_val > (ADC_INITIAL_CALIBRATION + 200 * ext_mem.ldr_adj))
					{
						ext_mem.alarm_status = true;

						put_event_task_actuator(EV_ACT_XX_ON, ID_LED_2);
						put_event_task_actuator(EV_ACT_XX_OFF, ID_LED_1);
					}
					else
					{
						ext_mem.alarm_status = false;

						put_event_task_actuator(EV_ACT_XX_ON, ID_LED_1);
						put_event_task_actuator(EV_ACT_XX_OFF, ID_LED_2);
					}
				}
				else
				{
					p_task_system_dta->tick--;
				}
			}
		}

		// Handle manual opening.
		if (p_task_system_dta->state != ST_SYS_OPEN_DOOR && p_task_system_dta->state != ST_SYS_INIT && p_task_system_dta->state != ST_SYS_REQ_PWD)
		{
			if ((true == p_task_system_dta->flag) && (EV_SYS_XX_BTN_ACTIVE == p_task_system_dta->event))
			{
				p_task_system_dta->flag = false;
				p_task_system_dta->state = ST_SYS_OPEN_DOOR;

				buffer_reset(pwd_buffer, &buffer_idx);

				// Prepare LCD.
				lcd_clear(&lcd1);
				lcd_pos(&lcd1, 2, 0);
				lcd_puts(&lcd1, "Puerta abierta.");
			}
		}

		// Handle buzzer.
		if (wrong_tries == 3)
		{
			wrong_tries++;
			put_event_task_actuator(EV_ACT_XX_BLINK, ID_BUZ);
		}

		char status_str[20];
		char key;

		switch (p_task_system_dta->state)
		{

			case ST_SYS_INIT:

				if (p_task_system_dta->tick > 0) {
					p_task_system_dta->tick--;
				}
				else
				{
					#if MEMORY_CONNECTED
						if (strcmp(ext_mem.mem_status, "written") == 0)
						{
							if (ext_mem.system_status) {
								// Prepare LCD.
								lcd_clear(&lcd1);
								lcd_pos(&lcd1, 0, 0);
								lcd_puts(&lcd1, "Ingrese clave:");
								lcd_pos(&lcd1, 2, 0);
								lcd_puts(&lcd1, "A-Borrar D-Confirmar");
								lcd_pos(&lcd1, 3, 0);
								lcd_puts(&lcd1, "C-Opciones");
								put_event_task_actuator(EV_ACT_XX_ON, ID_LED_2);
								put_event_task_actuator(EV_ACT_XX_OFF, ID_LED_1);

								p_task_system_dta->state = ST_SYS_AWAIT_PWD;
							}
							else
							{
								// Prepare LCD.
								lcd_clear(&lcd1);
								lcd_pos(&lcd1, 0, 0);
								lcd_puts(&lcd1, "Presione cualquier");
								lcd_pos(&lcd1, 1, 0);
								lcd_puts(&lcd1, "numero para entrar.");
								lcd_pos(&lcd1, 3, 0);
								lcd_puts(&lcd1, "C-Opciones");
								put_event_task_actuator(EV_ACT_XX_ON, ID_LED_1);
								put_event_task_actuator(EV_ACT_XX_OFF, ID_LED_2);

								p_task_system_dta->state = ST_SYS_OFF_MODE;
							}
						}
						else
						{
							lcd_clear(&lcd1);
							lcd_pos(&lcd1, 0, 0);
							lcd_puts(&lcd1, "Nueva clave:");
							lcd_pos(&lcd1, 3, 0);
							lcd_puts(&lcd1, "A-Borrar D-Confirmar");
							p_task_system_dta->state = ST_SYS_REQ_PWD;
						}
					#else
						lcd_clear(&lcd1);
						lcd_pos(&lcd1, 0, 0);
						lcd_puts(&lcd1, "Nueva clave:");
						lcd_pos(&lcd1, 3, 0);
						lcd_puts(&lcd1, "A-Borrar D-Confirmar");
						p_task_system_dta->state = ST_SYS_REQ_PWD;
					#endif
				}

				break;

			case ST_SYS_REQ_PWD:

				key = keypad_get_char();

				if (key != 0)
				{
					if (key >= '0' && key <= '9')
					{
						buffer_push_char(pwd_buffer, &buffer_idx, key);
						lcd_pos(&lcd1, 1, 0);
						buffer_to_lcd(&lcd1, pwd_buffer);
					}
					else if (key == 'A')
					{
						buffer_pull_char(pwd_buffer, &buffer_idx);
						lcd_pos(&lcd1, 1, 0);
						buffer_to_lcd(&lcd1, pwd_buffer);
					}
					else if (key == 'D')
					{
						if (buffer_idx == 5)
						{
							p_task_system_dta->state = ST_SYS_AWAIT_PWD;

							// Prepare LCD.
							lcd_clear(&lcd1);
							lcd_pos(&lcd1, 0, 0);
							lcd_puts(&lcd1, "Ingrese clave:");
							lcd_pos(&lcd1, 2, 0);
							lcd_puts(&lcd1, "A-Borrar D-Confirmar");
							lcd_pos(&lcd1, 3, 0);
							lcd_puts(&lcd1, "C-Opciones");

							// Init memory data.
							strcpy(ext_mem.mem_status, "written"); // Mark that the memory has been written.
							ext_mem.system_status = true;
							ext_mem.alarm_status = true;
							strcpy(ext_mem.password, pwd_buffer);
							ext_mem.password[5] = '\0';
							ext_mem.ldr_mode = false;
							ext_mem.ldr_adj = 5;
							// TODO: Write to actual memory.

							buffer_reset(pwd_buffer, &buffer_idx);

							put_event_task_actuator(EV_ACT_XX_ON, ID_LED_2);
							put_event_task_actuator(EV_ACT_XX_OFF, ID_LED_1);
						}
					}
				}

				break;

			case ST_SYS_AWAIT_PWD:

				if (ext_mem.alarm_status == false)
				{
					p_task_system_dta->state = ST_SYS_OFF_MODE;

					// Prepare LCD.
					lcd_clear(&lcd1);
					lcd_pos(&lcd1, 0, 0);
					lcd_puts(&lcd1, "Presione cualquier");
					lcd_pos(&lcd1, 1, 0);
					lcd_puts(&lcd1, "numero para entrar.");
					lcd_pos(&lcd1, 3, 0);
					lcd_puts(&lcd1, "C-Opciones");

					buffer_reset(pwd_buffer, &buffer_idx);

					break;
				}

				key = keypad_get_char();

				if (key != 0)
				{
					reset_tick = 0;

					if (key >= '0' && key <= '9')
					{
						buffer_push_char(pwd_buffer, &buffer_idx, key);
						lcd_pos(&lcd1, 1, 0);
						buffer_to_lcd(&lcd1, pwd_buffer);
					}
					else if (key == 'A')
					{
						buffer_pull_char(pwd_buffer, &buffer_idx);
						lcd_pos(&lcd1, 1, 0);
						buffer_to_lcd(&lcd1, pwd_buffer);
					}
					else if (key == 'C')
					{
						p_task_system_dta->state = ST_SYS_OPT_PWD;
						buffer_reset(pwd_buffer, &buffer_idx);

						// Prepare LCD.
						lcd_clear(&lcd1);
						lcd_pos(&lcd1, 0, 0);
						lcd_puts(&lcd1, "Ingrese clave (OPC):");
						lcd_pos(&lcd1, 2, 0);
						lcd_puts(&lcd1, "A-Borrar D-Confirmar");
						lcd_pos(&lcd1, 3, 0);
						lcd_puts(&lcd1, "C-Volver");
					}
					else if (key == 'D')
					{
						if (strcmp(ext_mem.password, pwd_buffer) == 0)
						{
							p_task_system_dta->state = ST_SYS_OPEN_DOOR;
							buffer_reset(pwd_buffer, &buffer_idx);

							// Prepare LCD.
							lcd_clear(&lcd1);
							lcd_pos(&lcd1, 0, 0);
							lcd_puts(&lcd1, "Clave correcta.");
							lcd_pos(&lcd1, 2, 0);
							lcd_puts(&lcd1, "Puerta abierta.");

							wrong_tries = 0;
							put_event_task_actuator(EV_ACT_XX_OFF, ID_BUZ);
						}
						else
						{
							buffer_reset(pwd_buffer, &buffer_idx);
							lcd_pos(&lcd1, 1, 0);
							buffer_to_lcd(&lcd1, pwd_buffer);

							if (wrong_tries < 3)
							{
								wrong_tries++;
							}
						}
					}
				}

				reset_tick++;

				if (reset_tick >= DEL_RESET_STATE)
				{
					reset_tick = 0;

					buffer_reset(pwd_buffer, &buffer_idx);
					lcd_pos(&lcd1, 1, 0);
					buffer_to_lcd(&lcd1, pwd_buffer);
				}

				break;

			case ST_SYS_OFF_MODE:

				if (ext_mem.alarm_status == true)
				{
					p_task_system_dta->state = ST_SYS_AWAIT_PWD;

					// Prepare LCD.
					lcd_clear(&lcd1);
					lcd_pos(&lcd1, 0, 0);
					lcd_puts(&lcd1, "Ingrese clave:");
					lcd_pos(&lcd1, 2, 0);
					lcd_puts(&lcd1, "A-Borrar D-Confirmar");
					lcd_pos(&lcd1, 3, 0);
					lcd_puts(&lcd1, "C-Opciones");

					buffer_reset(pwd_buffer, &buffer_idx);

					break;
				}

				key = keypad_get_char();

				if (key != 0)
				{
					if (key >= '0' && key <= '9')
					{
						p_task_system_dta->state = ST_SYS_OPEN_DOOR;

						// Prepare LCD.
						lcd_clear(&lcd1);
						lcd_pos(&lcd1, 2, 0);
						lcd_puts(&lcd1, "Puerta abierta.");
					}
					else if (key == 'C')
					{
						p_task_system_dta->state = ST_SYS_OPT_PWD;
						buffer_reset(pwd_buffer, &buffer_idx);

						// Prepare LCD.
						lcd_clear(&lcd1);
						lcd_pos(&lcd1, 0, 0);
						lcd_puts(&lcd1, "Ingrese clave (OPC):");
						lcd_pos(&lcd1, 2, 0);
						lcd_puts(&lcd1, "A-Borrar D-Confirmar");
						lcd_pos(&lcd1, 3, 0);
						lcd_puts(&lcd1, "C-Volver");
					}
				}

				break;

			case ST_SYS_OPT_PWD:

				key = keypad_get_char();

				if (key != 0)
				{
					reset_tick = 0;

					if (key >= '0' && key <= '9')
					{
						buffer_push_char(pwd_buffer, &buffer_idx, key);
						lcd_pos(&lcd1, 1, 0);
						buffer_to_lcd(&lcd1, pwd_buffer);
					}
					else if (key == 'A')
					{
						buffer_pull_char(pwd_buffer, &buffer_idx);
						lcd_pos(&lcd1, 1, 0);
						buffer_to_lcd(&lcd1, pwd_buffer);
					}
					else if (key == 'C')
					{
						buffer_reset(pwd_buffer, &buffer_idx);

						if (ext_mem.alarm_status)
						{
							p_task_system_dta->state = ST_SYS_AWAIT_PWD;

							// Prepare LCD.
							lcd_clear(&lcd1);
							lcd_pos(&lcd1, 0, 0);
							lcd_puts(&lcd1, "Ingrese clave:");
							lcd_pos(&lcd1, 2, 0);
							lcd_puts(&lcd1, "A-Borrar D-Confirmar");
							lcd_pos(&lcd1, 3, 0);
							lcd_puts(&lcd1, "C-Opciones");
						}
						else
						{
							p_task_system_dta->state = ST_SYS_OFF_MODE;

							// Prepare LCD.
							lcd_clear(&lcd1);
							lcd_pos(&lcd1, 0, 0);
							lcd_puts(&lcd1, "Presione cualquier");
							lcd_pos(&lcd1, 1, 0);
							lcd_puts(&lcd1, "numero para entrar.");
							lcd_pos(&lcd1, 3, 0);
							lcd_puts(&lcd1, "C-Opciones");
						}
					}
					else if (key == 'D')
					{
						if (strcmp(ext_mem.password, pwd_buffer) == 0)
						{
							p_task_system_dta->state = ST_SYS_OPT_MENU;
							buffer_reset(pwd_buffer, &buffer_idx);

							// Prepare LCD.
							lcd_clear(&lcd1);

							lcd_pos(&lcd1, 0, 0);

							snprintf(status_str, sizeof(status_str), "A-Sistema %s", (ext_mem.system_status == true ? "ON " : "OFF"));
							lcd_puts(&lcd1, status_str);

							lcd_pos(&lcd1, 1, 0);

							snprintf(status_str, sizeof(status_str), "B-Modo LDR %s", (ext_mem.ldr_mode == true ? "ON " : "OFF"));
							lcd_puts(&lcd1, status_str);

							lcd_pos(&lcd1, 2, 0);

							snprintf(status_str, sizeof(status_str), "C-Ajuste LDR %d ", ext_mem.ldr_adj);
							lcd_puts(&lcd1, status_str);

							lcd_pos(&lcd1, 3, 0);
							lcd_puts(&lcd1, "D-Volver     *-Reset");

							wrong_tries = 0;
							put_event_task_actuator(EV_ACT_XX_OFF, ID_BUZ);
						}
						else
						{
							buffer_reset(pwd_buffer, &buffer_idx);
							lcd_pos(&lcd1, 1, 0);
							buffer_to_lcd(&lcd1, pwd_buffer);

							if (wrong_tries < 3)
							{
								wrong_tries++;
							}
						}
					}
				}

				reset_tick++;

				if (reset_tick >= DEL_RESET_STATE)
				{
					reset_tick = 0;

					if (ext_mem.alarm_status == true)
					{
						p_task_system_dta->state = ST_SYS_AWAIT_PWD;

						// Prepare LCD.
						lcd_clear(&lcd1);
						lcd_pos(&lcd1, 0, 0);
						lcd_puts(&lcd1, "Ingrese clave:");
						lcd_pos(&lcd1, 2, 0);
						lcd_puts(&lcd1, "A-Borrar D-Confirmar");
						lcd_pos(&lcd1, 3, 0);
						lcd_puts(&lcd1, "C-Opciones");
					}
					else
					{
						p_task_system_dta->state = ST_SYS_OFF_MODE;

						// Prepare LCD.
						lcd_clear(&lcd1);
						lcd_pos(&lcd1, 0, 0);
						lcd_puts(&lcd1, "Presione cualquier");
						lcd_pos(&lcd1, 1, 0);
						lcd_puts(&lcd1, "numero para entrar.");
						lcd_pos(&lcd1, 3, 0);
						lcd_puts(&lcd1, "C-Opciones");
					}
				}

				break;

			case ST_SYS_OPT_MENU:

				key = keypad_get_char();

				if (key != 0)
				{
					reset_tick = 0;

					if (key == 'A')
					{
						ext_mem.system_status = !ext_mem.system_status;
						// TODO: Write to memory.

						lcd_pos(&lcd1, 0, 0);

						snprintf(status_str, sizeof(status_str), "A-Sistema %s", (ext_mem.system_status == true ? "ON " : "OFF"));
						lcd_puts(&lcd1, status_str);

						if (ext_mem.system_status == true)
						{
							put_event_task_actuator(EV_ACT_XX_ON, ID_LED_2);
							put_event_task_actuator(EV_ACT_XX_OFF, ID_LED_1);

							ext_mem.alarm_status = true;
						}
						else
						{
							put_event_task_actuator(EV_ACT_XX_ON, ID_LED_1);
							put_event_task_actuator(EV_ACT_XX_OFF, ID_LED_2);

							ext_mem.alarm_status = false;
						}
					}
					else if (key == 'B')
					{
						ext_mem.ldr_mode = !ext_mem.ldr_mode;
						// TODO: Write to memory.

						lcd_pos(&lcd1, 1, 0);

						snprintf(status_str, sizeof(status_str), "B-Modo LDR %s", (ext_mem.ldr_mode == true ? "ON " : "OFF"));
						lcd_puts(&lcd1, status_str);

						if (ext_mem.system_status == true && ext_mem.ldr_mode == false)
						{
							ext_mem.alarm_status = true;

							put_event_task_actuator(EV_ACT_XX_ON, ID_LED_2);
							put_event_task_actuator(EV_ACT_XX_OFF, ID_LED_1);
						}
					}
					else if (key == 'C')
					{
						ext_mem.ldr_adj = (ext_mem.ldr_adj % 9) + 1;
						// TODO: Write to memory.

						lcd_pos(&lcd1, 2, 0);

						snprintf(status_str, sizeof(status_str), "C-Ajuste LDR %d ", ext_mem.ldr_adj);
						lcd_puts(&lcd1, status_str);
					}
					else if (key == 'D')
					{
						if (ext_mem.alarm_status)
						{
							p_task_system_dta->state = ST_SYS_AWAIT_PWD;

							// Prepare LCD.
							lcd_clear(&lcd1);
							lcd_pos(&lcd1, 0, 0);
							lcd_puts(&lcd1, "Ingrese clave:");
							lcd_pos(&lcd1, 2, 0);
							lcd_puts(&lcd1, "A-Borrar D-Confirmar");
							lcd_pos(&lcd1, 3, 0);
							lcd_puts(&lcd1, "C-Opciones");
						}
						else
						{
							p_task_system_dta->state = ST_SYS_OFF_MODE;

							// Prepare LCD.
							lcd_clear(&lcd1);
							lcd_pos(&lcd1, 0, 0);
							lcd_puts(&lcd1, "Presione cualquier");
							lcd_pos(&lcd1, 1, 0);
							lcd_puts(&lcd1, "numero para entrar.");
							lcd_pos(&lcd1, 3, 0);
							lcd_puts(&lcd1, "C-Opciones");
						}
					}
					else if (key == '*')
					{
						lcd_clear(&lcd1);
						lcd_pos(&lcd1, 0, 0);
						lcd_puts(&lcd1, "Nueva clave:");
						lcd_pos(&lcd1, 3, 0);
						lcd_puts(&lcd1, "A-Borrar D-Confirmar");
						p_task_system_dta->state = ST_SYS_REQ_PWD;

						put_event_task_actuator(EV_ACT_XX_OFF, ID_LED_1);
						put_event_task_actuator(EV_ACT_XX_OFF, ID_LED_2);
					}
				}

				reset_tick++;

				if (reset_tick >= DEL_RESET_STATE)
				{
					reset_tick = 0;

					if (ext_mem.alarm_status == true)
					{
						p_task_system_dta->state = ST_SYS_AWAIT_PWD;

						// Prepare LCD.
						lcd_clear(&lcd1);
						lcd_pos(&lcd1, 0, 0);
						lcd_puts(&lcd1, "Ingrese clave:");
						lcd_pos(&lcd1, 2, 0);
						lcd_puts(&lcd1, "A-Borrar D-Confirmar");
						lcd_pos(&lcd1, 3, 0);
						lcd_puts(&lcd1, "C-Opciones");
					}
					else
					{
						p_task_system_dta->state = ST_SYS_OFF_MODE;

						// Prepare LCD.
						lcd_clear(&lcd1);
						lcd_pos(&lcd1, 0, 0);
						lcd_puts(&lcd1, "Presione cualquier");
						lcd_pos(&lcd1, 1, 0);
						lcd_puts(&lcd1, "numero para entrar.");
						lcd_pos(&lcd1, 3, 0);
						lcd_puts(&lcd1, "C-Opciones");
					}
				}

				break;

			case ST_SYS_OPEN_DOOR:

				if ((true == p_task_system_dta->flag) && (EV_SYS_XX_BTN_ACTIVE == p_task_system_dta->event))
				{
					p_task_system_dta->flag = false;

					if (ext_mem.alarm_status)
					{
						p_task_system_dta->state = ST_SYS_AWAIT_PWD;

						// Prepare LCD.
						lcd_clear(&lcd1);
						lcd_pos(&lcd1, 0, 0);
						lcd_puts(&lcd1, "Ingrese clave:");
						lcd_pos(&lcd1, 2, 0);
						lcd_puts(&lcd1, "A-Borrar D-Confirmar");
						lcd_pos(&lcd1, 3, 0);
						lcd_puts(&lcd1, "C-Opciones");
					}
					else
					{
						p_task_system_dta->state = ST_SYS_OFF_MODE;

						// Prepare LCD.
						lcd_clear(&lcd1);
						lcd_pos(&lcd1, 0, 0);
						lcd_puts(&lcd1, "Presione cualquier");
						lcd_pos(&lcd1, 1, 0);
						lcd_puts(&lcd1, "numero para entrar.");
						lcd_pos(&lcd1, 3, 0);
						lcd_puts(&lcd1, "C-Opciones");
					}
				}

				break;

			default:

				break;
		}
	}
}

/********************** end of file ******************************************/
