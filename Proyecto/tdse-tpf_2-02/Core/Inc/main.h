/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern TIM_HandleTypeDef htim1;
extern ADC_HandleTypeDef hadc1;
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED2_Pin GPIO_PIN_13
#define LED2_GPIO_Port GPIOC
#define BTN_Pin GPIO_PIN_0
#define BTN_GPIO_Port GPIOC
#define BUZ_Pin GPIO_PIN_3
#define BUZ_GPIO_Port GPIOC
#define R1_Pin GPIO_PIN_5
#define R1_GPIO_Port GPIOA
#define R2_Pin GPIO_PIN_6
#define R2_GPIO_Port GPIOA
#define R3_Pin GPIO_PIN_7
#define R3_GPIO_Port GPIOA
#define C4_Pin GPIO_PIN_4
#define C4_GPIO_Port GPIOC
#define MFRC522_MISO_Pin GPIO_PIN_1
#define MFRC522_MISO_GPIO_Port GPIOB
#define MFRC522_RST_Pin GPIO_PIN_2
#define MFRC522_RST_GPIO_Port GPIOB
#define MFRC522_CS_Pin GPIO_PIN_13
#define MFRC522_CS_GPIO_Port GPIOB
#define MFRC522_SCK_Pin GPIO_PIN_14
#define MFRC522_SCK_GPIO_Port GPIOB
#define MFRC522_MOSI_Pin GPIO_PIN_15
#define MFRC522_MOSI_GPIO_Port GPIOB
#define C3_Pin GPIO_PIN_10
#define C3_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define LED1_Pin GPIO_PIN_15
#define LED1_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define C1_Pin GPIO_PIN_4
#define C1_GPIO_Port GPIOB
#define C2_Pin GPIO_PIN_5
#define C2_GPIO_Port GPIOB
#define R4_Pin GPIO_PIN_6
#define R4_GPIO_Port GPIOB
#define LED3_Pin GPIO_PIN_7
#define LED3_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
