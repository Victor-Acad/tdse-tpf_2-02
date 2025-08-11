/*
 *
 * @file   : memory_handler.c
 * @date   : Aug 10, 2025
 *
 */

/********************** inclusions *******************************************/
#include "main.h"
#include "logger.h"
#include "memory_handler.h"

/********************** macros and definitions *******************************/

/********************** external functions definition ************************/
void handle_memory(uint32_t tick, MEM_WriteType_t type, char pwd[], uint8_t idx, char time_str[])
{
	switch (type)
	{
		case MEM_NO_WRITE:

			break;

		case MEM_WRITE_TIME:

			if (tick == 200)
			{
				HAL_I2C_Mem_Write(&hi2c2, 0xA0, 0x000F + 64*(idx - 1), I2C_MEMADD_SIZE_16BIT, (uint8_t*)time_str, strlen(time_str) + 1, HAL_MAX_DELAY);
			}
			else if (tick == 100)
			{
				HAL_I2C_Mem_Write(&hi2c2, 0xA0, 0x000E, I2C_MEMADD_SIZE_16BIT, &idx, 1, HAL_MAX_DELAY);
			}

			break;

		case MEM_WRITE_PWD:

			if (tick == 300)
			{
				HAL_I2C_Mem_Write(&hi2c2, 0xA0, 0x0000, I2C_MEMADD_SIZE_16BIT, (uint8_t*)"written", 8, HAL_MAX_DELAY);
			}
			else if (tick == 200)
			{
				HAL_I2C_Mem_Write(&hi2c2, 0xA0, 0x0008, I2C_MEMADD_SIZE_16BIT, (uint8_t*)pwd, 6, HAL_MAX_DELAY);
			}
			else if (tick == 100)
			{
				HAL_I2C_Mem_Write(&hi2c2, 0xA0, 0x000E, I2C_MEMADD_SIZE_16BIT, 0x00, 1, HAL_MAX_DELAY);
			}

			break;

		case MEM_CLEAR:

			if (tick == 300)
			{
				HAL_I2C_Mem_Write(&hi2c2, 0xA0, 0x0000, I2C_MEMADD_SIZE_16BIT, (uint8_t*)"notinit", 8, HAL_MAX_DELAY);
			}
			else if (tick == 200)
			{
				HAL_I2C_Mem_Write(&hi2c2, 0xA0, 0x0008, I2C_MEMADD_SIZE_16BIT, (uint8_t*)"xxxxx", 6, HAL_MAX_DELAY);
			}
			else if (tick == 100)
			{
				HAL_I2C_Mem_Write(&hi2c2, 0xA0, 0x000E, I2C_MEMADD_SIZE_16BIT, 0x00, 1, HAL_MAX_DELAY);
			}

			break;

		default:

			break;
	}
}

/********************** end of file ******************************************/
