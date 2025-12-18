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

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
void USB_CDC_RxHandler(uint8_t*, uint32_t);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define BOARD_LED_Pin GPIO_PIN_13
#define BOARD_LED_GPIO_Port GPIOC

/* USER CODE BEGIN Private defines */
#define RTK_I2C_SCK_PORT	0
#define RTK_I2C_SCK_PIN		1
#define RTK_I2C_SDA_PORT	2
#define RTK_I2C_SDA_PIN		3

#define RTK_I2C_SCK_PORT_1		GPIOB
#define RTK_I2C_SCK_PIN_1		GPIO_PIN_8 /* GPIO used for SMI Clock Generation */
#define RTK_I2C_SDA_PORT_1		GPIOB
#define RTK_I2C_SDA_PIN_1		GPIO_PIN_9 /* GPIO used for SMI Data signal */

#define RTK_I2C_SCK_PORT_2		GPIOB
#define RTK_I2C_SCK_PIN_2		GPIO_PIN_6 /* GPIO used for SMI Clock Generation */
#define RTK_I2C_SDA_PORT_2		GPIOB
#define RTK_I2C_SDA_PIN_2		GPIO_PIN_7 /* GPIO used for SMI Data signal */


#define RTK_I2C_DELAY			5
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
