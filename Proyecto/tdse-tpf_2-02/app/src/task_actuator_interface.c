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
 * @file   : task_actuator_interface.c
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

/* Application & Tasks includes. */
#include "board.h"
#include "app.h"
#include "task_actuator_attribute.h"

/********************** macros and definitions *******************************/

/********************** internal data declaration ****************************/

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/

/********************** external data declaration ****************************/

/********************** external functions definition ************************/
void put_event_task_actuator(task_actuator_ev_t event, task_actuator_id_t identifier)
{
	task_actuator_dta_t *p_task_actuator_dta;

	p_task_actuator_dta = &task_actuator_dta_list[identifier];

	p_task_actuator_dta->event = event;
	p_task_actuator_dta->flag = true;
}

// Pushes a char to the buffer.
void buffer_push_char(char buffer[], uint8_t* idx, char c)
{
	if (*idx < 5)
	{
		buffer[*idx] = c;
		(*idx)++;
	}
}

// "Pulls" a char from the buffer.
void buffer_pull_char(char buffer[], uint8_t* idx)
{
	if (*idx > 0)
	{
		(*idx)--;
		buffer[*idx] = 'x';
	}
}

// Resets the buffer state.
void buffer_reset(char buffer[], uint8_t* idx)
{
	memset(buffer, 'x', 5);
	*idx = 0;
}

// Prints the buffer content into the LCD screen.
void buffer_to_lcd(I2C_LCD_HandleTypeDef *lcd, char buffer[])
{
	for(int i=0; i < 5; i++)
	{
		if (buffer[i] != 'x')
		{
			lcd_putchar(lcd, buffer[i]);
		}
		else lcd_putchar(lcd, ' ');
	}
}

/********************** end of file ******************************************/
