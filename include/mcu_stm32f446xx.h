/*
 * mcu_stm32f446xx.h
 *
 *  Created on: Jun 29, 2017
 *      Author: tgil
 */

#ifndef MCU_STM32F446XX_H_
#define MCU_STM32F446XX_H_


#include <string.h>
#include <mcu/types.h>


#include "cmsis/stm32f4xx.h"
#include "cmsis/stm32f446xx.h"


#define MCU_NO_HARD_FAULT 1

#define MCU_ADC_API 0
#define MCU_ADC_PORTS 3
#define MCU_ADC_REGS { ADC1, ADC2, ADC3 }
#define MCU_ADC_IRQS { ADC_IRQn, ADC_IRQn, ADC_IRQn }

#define MCU_ENET_PORTS 1
#define MCU_FLASH_PORTS 1
#define MCU_MEM_PORTS 1

#define MCU_I2C_API 0
#define MCU_I2C_PORTS 3
#define MCU_I2C_REGS { I2C1, I2C2, I2C3 }
#define MCU_I2C_IRQS { I2C1_EV_IRQn, I2C2_EV_IRQn, I2C3_EV_IRQn }
#define MCU_I2C_ER_IRQS { I2C1_ER_IRQn, I2C2_ER_IRQn, I2C3_ER_IRQn }

#define MCU_CORE_PORTS 1
#define MCU_EEPROM_PORTS 0
#define MCU_SPI_API 0
#define MCU_SPI_PORTS 4
#define MCU_SPI_REGS { SPI1, SPI2, SPI3, SPI4 }
#define MCU_SPI_IRQS { SPI1_IRQn, SPI2_IRQn, SPI3_IRQn, SPI4_IRQn }

#define MCU_SAI_API 0
#define MCU_SAI_PORTS 2
#define MCU_SAI_REGS { SAI1, SAI2 }
#define MCU_SAI_IRQS { SAI1_IRQn, SAI2_IRQn }

#define MCU_SDIO_API 0
#define MCU_SDIO_PORTS 1
#define MCU_SDIO_REGS { SDIO }
#define MCU_SDIO_IRQS { SDIO_IRQn }

#define MCU_TMR_PORTS 14
#define MCU_TMR_REGS { TIM1, TIM2, TIM3, TIM4, TIM5, TIM6, TIM7, TIM8, TIM9, TIM10, TIM11, TIM12, TIM13, TIM14 }
#define MCU_TMR_IRQS { -1, TIM2_IRQn, TIM3_IRQn, TIM4_IRQn, TIM5_IRQn, TIM6_DAC_IRQn, TIM7_IRQn, -1, TIM1_BRK_TIM9_IRQn, TIM1_UP_TIM10_IRQn, TIM1_TRG_COM_TIM11_IRQn, \
	TIM8_BRK_TIM12_IRQn, TIM8_UP_TIM13_IRQn, TIM8_TRG_COM_TIM14_IRQn }

#define MCU_PIO_PORTS 8
#define MCU_PIO_REGS { GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH }
#define MCU_PIO_IRQS { 0 }

#define MCU_UART_PORTS 6
#define MCU_UART_REGS { USART1, USART2, USART3, UART4, UART5, USART6 }
#define MCU_UART_IRQS { USART1_IRQn, USART2_IRQn, USART3_IRQn, UART4_IRQn, UART5_IRQn, USART6_IRQn }

#define DEV_LAST_IRQ FMPI2C1_ER_IRQn
#define DEV_MIDDLE_PRIORITY 16


#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif


#endif /* MCU_STM32F446XX_H_ */