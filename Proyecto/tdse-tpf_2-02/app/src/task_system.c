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
#include "mfrc522.h"
#include "ds3231.h"

/* Application & Tasks includes. */
#include "board.h"
#include "app.h"
#include "task_system_attribute.h"
#include "task_system_interface.h"
#include "task_actuator_attribute.h"
#include "task_actuator_interface.h"
#include "memory_handler.h"

/********************** macros and definitions *******************************/
#define G_TASK_SYS_CNT_INI			0ul
#define G_TASK_SYS_TICK_CNT_INI		0ul

#define DEL_SYS_XX_MIN				0ul
#define DEL_SYS_XX_MED				50ul
#define DEL_SYS_XX_MAX				500ul

#define DEL_SYS_INIT				1500ul
#define DEL_ADC_READ				5000ul
#define DEL_RFID_READ				400ul
#define DEL_RESET_STATE				10000ul
#define DEL_WRONG_PWD_WAIT			2000ul

#define ADC_INITIAL_CALIBRATION		1500

#define MAX_STORED_ENTRIES			5

#define MEMORY_CONNECTED			(1)
#define MEMORY_ACCESS				(1)

/********************** internal data declaration ****************************/
task_system_dta_t task_system_dta = {
    .tick = DEL_SYS_INIT,
	.adc_tick = 0,
    .reset_tick = 0,
	.rfid_tick = 0,
	.mem_tick = 0,
    .state = ST_SYS_INIT,
    .event = EV_SYS_XX_BTN_IDLE,
    .flag = false,
    .system_parameters = {
        .mem_status = {'\0'},
        .password = {'\0'},
        .system_status = true,
        .alarm_status = true,
        .ldr_mode = false,
        .ldr_adj = 5,
		.saved_entries = 0
    }
};

#define SYSTEM_DTA_QTY	(sizeof(task_system_dta)/sizeof(task_system_dta_t))

// Utility variables.
uint8_t wrong_tries = 0;

// Password buffer.
char pwd_buffer[6] = "xxxxx";
uint8_t buffer_idx = 0;

// Memory handler data.
MEM_WriteType_t MEM_WriteType = MEM_NO_WRITE;
char pwd_to_mem[6];

// Allowed UIDs array.
const char* allowed_uids[] = {
	"90245C21"
};

#define ALLOWED_UIDS_QTY (sizeof(allowed_uids)/sizeof(allowed_uids[0]))


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
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 1000);

	/* Init RFID module */
	MFRC522_Init();

	/* Read memory */
	#if MEMORY_CONNECTED
		HAL_I2C_Mem_Read(&hi2c2, 0xA0, 0x0000, I2C_MEMADD_SIZE_16BIT, (uint8_t*)p_task_system_dta->system_parameters.mem_status, 8, HAL_MAX_DELAY);
		HAL_I2C_Mem_Read(&hi2c2, 0xA0, 0x0008, I2C_MEMADD_SIZE_16BIT, (uint8_t*)p_task_system_dta->system_parameters.password, 6, HAL_MAX_DELAY);
		HAL_I2C_Mem_Read(&hi2c2, 0xA0, 0x000E, I2C_MEMADD_SIZE_16BIT, &p_task_system_dta->system_parameters.saved_entries, 1, HAL_MAX_DELAY);

	#if MEMORY_ACCESS
		LOGGER_LOG("Se inició el sistema en modo de acceso a la memoria.\n\n");
		LOGGER_LOG("Estado: %s\nContraseña: %s\n\n", p_task_system_dta->system_parameters.mem_status, p_task_system_dta->system_parameters.password);
		LOGGER_LOG("En total hay %d entradas guardadas.\n\n", p_task_system_dta->system_parameters.saved_entries);

		char time_str[33];

		for (uint8_t i = 0; i < p_task_system_dta->system_parameters.saved_entries; i++)
		{
			HAL_I2C_Mem_Read(&hi2c2, 0xA0, 0x000F + 64*i, I2C_MEMADD_SIZE_16BIT, (uint8_t*)time_str, sizeof(time_str), HAL_MAX_DELAY);

			LOGGER_LOG("%s\n", time_str);
		}

		LOGGER_LOG("\n");
	#endif

	#endif

	/* Turn off actuators */
		put_event_task_actuator(EV_ACT_XX_OFF, ID_LED_1);
		put_event_task_actuator(EV_ACT_XX_OFF, ID_LED_2);
		put_event_task_actuator(EV_ACT_XX_OFF, ID_LED_3);
		put_event_task_actuator(EV_ACT_XX_OFF, ID_BUZ);


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

		// Handle ADC data and LDR controlled system.
		if (p_task_system_dta->adc_tick == 0)
		{
			p_task_system_dta->adc_tick = DEL_ADC_READ;

			HAL_ADC_Start(&hadc1);
			HAL_ADC_PollForConversion(&hadc1, 0);

			uint16_t adc_val = HAL_ADC_GetValue(&hadc1);

			if (adc_val > (ADC_INITIAL_CALIBRATION + 200 * p_task_system_dta->system_parameters.ldr_adj))
			{
				put_event_task_actuator(EV_ACT_XX_ON, ID_LED_3);
			}
			else
			{
				put_event_task_actuator(EV_ACT_XX_OFF, ID_LED_3);
			}

			if (p_task_system_dta->system_parameters.system_status && p_task_system_dta->system_parameters.ldr_mode)
			{
				if (adc_val > (ADC_INITIAL_CALIBRATION + 200 * p_task_system_dta->system_parameters.ldr_adj))
				{
					p_task_system_dta->system_parameters.alarm_status = true;

					put_event_task_actuator(EV_ACT_XX_ON, ID_LED_2);
					put_event_task_actuator(EV_ACT_XX_OFF, ID_LED_1);
				}
				else
				{
					p_task_system_dta->system_parameters.alarm_status = false;

					put_event_task_actuator(EV_ACT_XX_ON, ID_LED_1);
					put_event_task_actuator(EV_ACT_XX_OFF, ID_LED_2);
				}
			}
		}
		else
		{
			p_task_system_dta->adc_tick--;
		}

		/* Local variables declaration */

		char status_str[21];
		char key;

		uint8_t UID[8];
		uint8_t TagType;

		uint8_t day, mth, year, dow, hr, min, sec;
		char time_str[33];

		switch (p_task_system_dta->state)
		{

			case ST_SYS_INIT:

				if (p_task_system_dta->tick > 0) {
					p_task_system_dta->tick--;
				}
				else
				{
					#if MEMORY_CONNECTED
						if (strcmp(p_task_system_dta->system_parameters.mem_status, "written") == 0)
						{
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

							#if MEMORY_CONNECTED
								strcpy(pwd_to_mem, pwd_buffer);
								MEM_WriteType = MEM_WRITE_PWD;
								p_task_system_dta->mem_tick = 300;
							#endif

							memcpy(p_task_system_dta->system_parameters.password, pwd_buffer, 6);

							buffer_reset(pwd_buffer, &buffer_idx);

							put_event_task_actuator(EV_ACT_XX_ON, ID_LED_2);
							put_event_task_actuator(EV_ACT_XX_OFF, ID_LED_1);
						}
					}
				}

				break;

			case ST_SYS_AWAIT_PWD:

				if (p_task_system_dta->system_parameters.alarm_status == false)
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
					p_task_system_dta->reset_tick = 0;

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
					else if ((key == 'D') && (buffer_idx > 0))
					{
						if (strcmp(p_task_system_dta->system_parameters.password, pwd_buffer) == 0)
						{
							p_task_system_dta->state = ST_SYS_OPEN_DOOR;
							__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 2000);
							buffer_reset(pwd_buffer, &buffer_idx);

							// Prepare LCD.
							lcd_clear(&lcd1);
							lcd_pos(&lcd1, 0, 0);
							lcd_puts(&lcd1, "Clave correcta.");
							lcd_pos(&lcd1, 2, 0);
							lcd_puts(&lcd1, "Puerta abierta.");

							wrong_tries = 0;
							put_event_task_actuator(EV_ACT_XX_FAST_BLINK, ID_BUZ);
						}
						else
						{
							lcd_clear(&lcd1);
							lcd_pos(&lcd1, 1, 0);
							lcd_puts(&lcd1, "  CLAVE INCORRECTA");
							buffer_reset(pwd_buffer, &buffer_idx);

							p_task_system_dta->tick = DEL_WRONG_PWD_WAIT;
							p_task_system_dta->state = ST_SYS_WAIT;

							if (wrong_tries < 2)
							{
								wrong_tries++;
							}
							else if (wrong_tries == 2)
							{
								wrong_tries++;
								put_event_task_actuator(EV_ACT_XX_BLINK, ID_BUZ);
							}
						}
					}
				}

				if (p_task_system_dta->rfid_tick == 0)
				{
					p_task_system_dta->rfid_tick = DEL_RFID_READ;

					if (MFRC522_IsCard(&TagType))
					{
						if (MFRC522_ReadCardSerial((uint8_t*)&UID))
						{
							if (verify_uid(allowed_uids, ALLOWED_UIDS_QTY, UID))
							{
								p_task_system_dta->state = ST_SYS_OPEN_DOOR;
								__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 2000);

								// Prepare LCD.
								lcd_clear(&lcd1);
								lcd_pos(&lcd1, 0, 0);
								lcd_puts(&lcd1, "Tarjeta introducida.");
								lcd_pos(&lcd1, 2, 0);
								lcd_puts(&lcd1, "Puerta abierta.");

								buffer_reset(pwd_buffer, &buffer_idx);

								wrong_tries = 0;
								put_event_task_actuator(EV_ACT_XX_FAST_BLINK, ID_BUZ);

								#if MEMORY_CONNECTED
									if (p_task_system_dta->system_parameters.saved_entries >= MAX_STORED_ENTRIES)
									{
										p_task_system_dta->system_parameters.saved_entries = 0;
									}

									DS3231_Get_Date(&day, &mth, &year, &dow);
									DS3231_Get_Time(&hr, &min, &sec);
									snprintf(time_str, sizeof(time_str), "%02u/%02u/20%02u | %02u:%02u:%02u | %02X%02X%02X%02X", day, mth, year, hr, min, sec, UID[0], UID[1], UID[2], UID[3]);

									p_task_system_dta->system_parameters.saved_entries++;

									MEM_WriteType = MEM_WRITE_TIME;
									p_task_system_dta->mem_tick = 200;
								#endif
							}
							else
							{
								lcd_clear(&lcd1);
								lcd_pos(&lcd1, 1, 0);
								lcd_puts(&lcd1, "TARJETA NO ACEPTADA");
								buffer_reset(pwd_buffer, &buffer_idx);

								p_task_system_dta->tick = DEL_WRONG_PWD_WAIT;
								p_task_system_dta->state = ST_SYS_WAIT;

								if (wrong_tries < 2)
								{
									wrong_tries++;
								}
								else if (wrong_tries == 2)
								{
									wrong_tries++;
									put_event_task_actuator(EV_ACT_XX_BLINK, ID_BUZ);
								}
							}

						}

						MFRC522_Halt();
					}
				}
				else
				{
					p_task_system_dta->rfid_tick--;
				}

				p_task_system_dta->reset_tick++;

				if (p_task_system_dta->reset_tick >= DEL_RESET_STATE)
				{
					p_task_system_dta->reset_tick = 0;

					buffer_reset(pwd_buffer, &buffer_idx);
					lcd_pos(&lcd1, 1, 0);
					buffer_to_lcd(&lcd1, pwd_buffer);
				}

				break;

			case ST_SYS_OFF_MODE:

				if (p_task_system_dta->system_parameters.alarm_status == true)
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
						__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 2000);

						// Prepare LCD.
						lcd_clear(&lcd1);
						lcd_pos(&lcd1, 2, 0);
						lcd_puts(&lcd1, "Puerta abierta.");

						put_event_task_actuator(EV_ACT_XX_FAST_BLINK, ID_BUZ);
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

				if (p_task_system_dta->rfid_tick == 0)
				{
					p_task_system_dta->rfid_tick = DEL_RFID_READ;

					if (MFRC522_IsCard(&TagType))
					{
						if (MFRC522_ReadCardSerial((uint8_t*)&UID))
						{
							if (verify_uid(allowed_uids, ALLOWED_UIDS_QTY, UID))
							{
								p_task_system_dta->state = ST_SYS_OPEN_DOOR;
								__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 2000);

								// Prepare LCD.
								lcd_clear(&lcd1);
								lcd_pos(&lcd1, 0, 0);
								lcd_puts(&lcd1, "Tarjeta introducida.");
								lcd_pos(&lcd1, 2, 0);
								lcd_puts(&lcd1, "Puerta abierta.");

								buffer_reset(pwd_buffer, &buffer_idx);

								wrong_tries = 0;
								put_event_task_actuator(EV_ACT_XX_FAST_BLINK, ID_BUZ);

								#if MEMORY_CONNECTED
									if (p_task_system_dta->system_parameters.saved_entries >= MAX_STORED_ENTRIES)
									{
										p_task_system_dta->system_parameters.saved_entries = 0;
									}

									DS3231_Get_Date(&day, &mth, &year, &dow);
									DS3231_Get_Time(&hr, &min, &sec);
									snprintf(time_str, sizeof(time_str), "%02u/%02u/20%02u | %02u:%02u:%02u | %02X%02X%02X%02X", day, mth, year, hr, min, sec, UID[0], UID[1], UID[2], UID[3]);

									p_task_system_dta->system_parameters.saved_entries++;

									MEM_WriteType = MEM_WRITE_TIME;
									p_task_system_dta->mem_tick = 200;
								#endif
							}
						}

						MFRC522_Halt();
					}
				}
				else
				{
					p_task_system_dta->rfid_tick--;
				}

				break;

			case ST_SYS_OPT_PWD:

				key = keypad_get_char();

				if (key != 0)
				{
					p_task_system_dta->reset_tick = 0;

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

						if (p_task_system_dta->system_parameters.alarm_status)
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
					else if ((key == 'D') && (buffer_idx > 0))
					{
						if (strcmp(p_task_system_dta->system_parameters.password, pwd_buffer) == 0)
						{
							p_task_system_dta->state = ST_SYS_OPT_MENU;
							buffer_reset(pwd_buffer, &buffer_idx);

							// Prepare LCD.
							lcd_clear(&lcd1);

							lcd_pos(&lcd1, 0, 0);

							snprintf(status_str, sizeof(status_str), "A-Sistema %s", (p_task_system_dta->system_parameters.system_status == true ? "ON " : "OFF"));
							lcd_puts(&lcd1, status_str);

							lcd_pos(&lcd1, 1, 0);

							snprintf(status_str, sizeof(status_str), "B-Modo LDR %s", (p_task_system_dta->system_parameters.ldr_mode == true ? "ON " : "OFF"));
							lcd_puts(&lcd1, status_str);

							lcd_pos(&lcd1, 2, 0);

							snprintf(status_str, sizeof(status_str), "C-Ajuste LDR %d ", p_task_system_dta->system_parameters.ldr_adj);
							lcd_puts(&lcd1, status_str);

							lcd_pos(&lcd1, 3, 0);
							lcd_puts(&lcd1, "D-Volver     *-Reset");

							wrong_tries = 0;
							put_event_task_actuator(EV_ACT_XX_OFF, ID_BUZ);
						}

						#if MEMORY_CONNECTED

						else if (strcmp("65535", pwd_buffer) == 0)
						{
							MEM_WriteType = MEM_CLEAR;
							p_task_system_dta->mem_tick = 300;

							buffer_reset(pwd_buffer, &buffer_idx);
							lcd_pos(&lcd1, 1, 0);
							buffer_to_lcd(&lcd1, pwd_buffer);
						}

						#endif

						else
						{
							buffer_reset(pwd_buffer, &buffer_idx);
							lcd_pos(&lcd1, 1, 0);
							buffer_to_lcd(&lcd1, pwd_buffer);

							if (wrong_tries < 2)
							{
								wrong_tries++;
							}
							else if (wrong_tries == 2)
							{
								wrong_tries++;
								put_event_task_actuator(EV_ACT_XX_BLINK, ID_BUZ);
							}
						}
					}
				}

				p_task_system_dta->reset_tick++;

				if (p_task_system_dta->reset_tick >= DEL_RESET_STATE)
				{
					p_task_system_dta->reset_tick = 0;

					if (p_task_system_dta->system_parameters.alarm_status == true)
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
					p_task_system_dta->reset_tick = 0;

					if (key == 'A')
					{
						p_task_system_dta->system_parameters.system_status = !p_task_system_dta->system_parameters.system_status;

						lcd_pos(&lcd1, 0, 0);

						snprintf(status_str, sizeof(status_str), "A-Sistema %s", (p_task_system_dta->system_parameters.system_status == true ? "ON " : "OFF"));
						lcd_puts(&lcd1, status_str);

						if (p_task_system_dta->system_parameters.system_status == true)
						{
							put_event_task_actuator(EV_ACT_XX_ON, ID_LED_2);
							put_event_task_actuator(EV_ACT_XX_OFF, ID_LED_1);

							p_task_system_dta->system_parameters.alarm_status = true;
						}
						else
						{
							put_event_task_actuator(EV_ACT_XX_ON, ID_LED_1);
							put_event_task_actuator(EV_ACT_XX_OFF, ID_LED_2);

							p_task_system_dta->system_parameters.alarm_status = false;
						}
					}
					else if (key == 'B')
					{
						p_task_system_dta->system_parameters.ldr_mode = !p_task_system_dta->system_parameters.ldr_mode;

						lcd_pos(&lcd1, 1, 0);

						snprintf(status_str, sizeof(status_str), "B-Modo LDR %s", (p_task_system_dta->system_parameters.ldr_mode == true ? "ON " : "OFF"));
						lcd_puts(&lcd1, status_str);

						if (p_task_system_dta->system_parameters.system_status == true && p_task_system_dta->system_parameters.ldr_mode == false)
						{
							p_task_system_dta->system_parameters.alarm_status = true;

							put_event_task_actuator(EV_ACT_XX_ON, ID_LED_2);
							put_event_task_actuator(EV_ACT_XX_OFF, ID_LED_1);
						}
					}
					else if (key == 'C')
					{
						p_task_system_dta->system_parameters.ldr_adj = (p_task_system_dta->system_parameters.ldr_adj % 9) + 1;

						lcd_pos(&lcd1, 2, 0);

						snprintf(status_str, sizeof(status_str), "C-Ajuste LDR %d ", p_task_system_dta->system_parameters.ldr_adj);
						lcd_puts(&lcd1, status_str);
					}
					else if (key == 'D')
					{
						if (p_task_system_dta->system_parameters.alarm_status)
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

				p_task_system_dta->reset_tick++;

				if (p_task_system_dta->reset_tick >= DEL_RESET_STATE)
				{
					p_task_system_dta->reset_tick = 0;

					if (p_task_system_dta->system_parameters.alarm_status == true)
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
					__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 1000);
					put_event_task_actuator(EV_ACT_XX_OFF, ID_BUZ);

					if (p_task_system_dta->system_parameters.alarm_status)
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

			case ST_SYS_WAIT:

				if (p_task_system_dta->tick == 0)
				{
					if (p_task_system_dta->system_parameters.alarm_status)
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
				else
				{
					p_task_system_dta->tick--;
				}

				break;

			default:

				break;
		}

		// Handle memory.
		if (p_task_system_dta->mem_tick > 0)
		{
			handle_memory(p_task_system_dta->mem_tick, MEM_WriteType, pwd_to_mem, p_task_system_dta->system_parameters.saved_entries, time_str);
			p_task_system_dta->mem_tick--;
		}
		else
		{
			MEM_WriteType = MEM_NO_WRITE;
		}
	}
}

/********************** end of file ******************************************/
