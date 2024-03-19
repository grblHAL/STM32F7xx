/*
  main.c - driver code for STM32F7xx ARM processors

  Part of grblHAL

  Copyright (c) 2021-2024 Terje Io
  Some parts (C) COPYRIGHT STMicroelectronics - code created by IDE

  grblHAL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  grblHAL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with grblHAL. If not, see <http://www.gnu.org/licenses/>.
*/

#include "main.h"
#include "grbl/grbllib.h"

void SystemClock_Config(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    if(!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->LAR = 0xC5ACCE55;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
        DWT->LAR = 0;
    }

    grbl_enter();
}

void SystemClock_Config(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();

#if RTC_ENABLE
    /** Configure LSE Drive Capability */
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);
#endif

#ifdef STM32F765xx

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitTypeDef RCC_OscInitStruct = {
        .OscillatorType = RCC_OSCILLATORTYPE_HSE,
        .HSEState = RCC_HSE_ON,
        .PLL.PLLState = RCC_PLL_ON,
        .PLL.PLLSource = RCC_PLLSOURCE_HSE,
        .PLL.PLLM = 25,
        .PLL.PLLN = 432,
        .PLL.PLLP = RCC_PLLP_DIV2,
        .PLL.PLLQ = 6,
        .PLL.PLLR = 2
    };

    #define APB1CLKDIV RCC_HCLK_DIV4
    #define APB2CLKDIV RCC_HCLK_DIV2

    #define FLASH_LATENCY FLASH_LATENCY_7

#else // F756

    /** Configure the main internal regulator output voltage  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitTypeDef RCC_OscInitStruct = {
#if RTC_ENABLE
        .OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE,
        .LSEState       = RCC_LSE_ON,
#else
        .OscillatorType = RCC_OSCILLATORTYPE_HSE,
#endif
        .HSEState       = RCC_HSE_ON,
        .PLL.PLLState   = RCC_PLL_ON,
        .PLL.PLLSource  = RCC_PLLSOURCE_HSE,
        .PLL.PLLM       = 4,
        .PLL.PLLN       = 216,
        .PLL.PLLP       = RCC_PLLP_DIV2,
        .PLL.PLLQ       = 9
    };

    #define APB1CLKDIV RCC_HCLK_DIV4
    #define APB2CLKDIV RCC_HCLK_DIV2

    #define FLASH_LATENCY FLASH_LATENCY_7

#endif

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
      Error_Handler();
    }

    /** Activate the Over-Drive mode */
    if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
      Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks */
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {
        .ClockType      = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2,
        .SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK,
        .AHBCLKDivider  = RCC_SYSCLK_DIV1,
        .APB1CLKDivider = APB1CLKDIV,
        .APB2CLKDivider = APB2CLKDIV
    };
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY) != HAL_OK) {
      Error_Handler();
    }

    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {
#if RTC_ENABLE
        .PeriphClockSelection = RCC_PERIPHCLK_USART3|RCC_PERIPHCLK_I2C1|RCC_PERIPHCLK_CLK48|RCC_PERIPHCLK_RTC,
        .RTCClockSelection    = RCC_RTCCLKSOURCE_LSE,
#else
        .PeriphClockSelection = RCC_PERIPHCLK_USART3|RCC_PERIPHCLK_I2C1|RCC_PERIPHCLK_CLK48,
#endif
        .Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1,
        .I2c1ClockSelection   = RCC_I2C1CLKSOURCE_PCLK1,
        .Clk48ClockSelection  = RCC_CLK48SOURCE_PLL
    };
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
      Error_Handler();
    }

#if RTC_ENABLE
    __HAL_RCC_RTC_ENABLE();
#endif
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1);
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */

#include <stdio.h>
#include "grbl/hal.h"

void assert_failed(uint8_t *file, uint32_t line)
{
    char buf[100];

    if(hal.stream.write) {
        sprintf(buf, "Wrong parameters value: file %s on line %lu\r\n", file, line);
        hal.stream.write(buf);
    }
}
#endif
