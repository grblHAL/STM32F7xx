/*

  driver.h - driver code for STM32F7xx ARM processors

  Part of grblHAL

  Copyright (c) 2021-2025 Terje Io

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

//
// NOTE: do NOT change configuration here - edit my_machine.h instead!
//

#ifndef __DRIVER_H__
#define __DRIVER_H__

#include <stdbool.h>
#include <stdint.h>

#ifndef OVERRIDE_MY_MACHINE
#include "my_machine.h"
#endif

#include "main.h"
#include "pwm.h"

#if defined(MCP3221_ENABLE)
#define I2C_ENABLE 1
#define MCP3221_ENABLE_NEW MCP3221_ENABLE
#endif

#define OPTS_POSTPROCESSING

#include "grbl/driver_opts.h"

#include "timers.h"

#define DIGITAL_OUT(port, bit, on) { port->BSRR = (on) ? (bit) : ((bit) << 16); }
#define DIGITAL_IN(port, bit) (!!(port->IDR & (bit)))

#define timer(t) timerN(t)
#define timerN(t) TIM ## t
#define timerBase(t) timerbase(t)
#define timerbase(t) TIM ## t ## _BASE
#define timerINT(t) timerint(t)
#define timerint(t) TIM ## t ## _IRQn
#define timerHANDLER(t) timerhandler(t)
#define timerhandler(t) TIM ## t ## _IRQHandler
#define timerCLKEN(t) timerclken(t)
#define timerclken(t) __HAL_RCC_TIM ## t ## _CLK_ENABLE
#define timerCCEN(c, n) timerccen(c, n)
#define timerccen(c, n) TIM_CCER_CC ## c ## n ## E
#define timerCCMR(p, c) timerccmr(p, c)
#define timerccmr(p, c) TIM ## p->CCMR ## c
#define timerOCM(p, c) timerocm(p, c)
#define timerocm(p, c) TIM_CCMR ## p ##_OC ## c ## M_1|TIM_CCMR ## p ##_OC ## c ## M_2
#define timerOCMC(p, c) timerocmc(p, c)
#define timerocmc(p, c) (TIM_CCMR ## p ##_OC ## c ## M|TIM_CCMR ## p ##_CC ## c ## S)
#define timerCCR(t, c) timerccr(t, c)
#define timerccr(t, c) TIM ## t->CCR ## c
#define timerCCP(c, n) timerccp(c, n)
#define timerccp(c, n) TIM_CCER_CC ## c ## n ## P
#define timerCR2OIS(c, n) timercr2ois(c, n)
#define timercr2ois(c, n) TIM_CR2_OIS ## c ## n
#define timerAF(t, f) timeraf(t, f)
#define timeraf(t, f) GPIO_AF ## f ## _TIM ## t
#define usart(t) usartN(t)
#define usartN(t) USART ## t
#define usartINT(t) usartint(t)
#define usartint(t) USART ## t ## _IRQn
#define usartHANDLER(t) usarthandler(t)
#define usarthandler(t) USART ## t ## _IRQHandler
#define usartCLKEN(t) usartclken(t)
#define usartclken(t) __HAL_RCC_USART ## t ## _CLK_ENABLE

#define TIMER_CLOCK_MUL(d) (d == RCC_HCLK_DIV1 ? 1 : (d == RCC_HCLK_DIV2 ? 2 : (d == RCC_HCLK_DIV4 ? 4 : 8)))

// Define GPIO output mode options

#define GPIO_SHIFT0   0
#define GPIO_SHIFT1   1
#define GPIO_SHIFT2   2
#define GPIO_SHIFT3   3
#define GPIO_SHIFT4   4
#define GPIO_SHIFT5   5
#define GPIO_SHIFT6   6
#define GPIO_SHIFT7   7
#define GPIO_SHIFT8   8
#define GPIO_SHIFT9   9
#define GPIO_SHIFT10 10
#define GPIO_SHIFT11 11
#define GPIO_SHIFT12 12
#define GPIO_SHIFT13 13
#define GPIO_MAP     14
#define GPIO_SINGLE  15

#ifndef IS_NUCLEO_DEVKIT
#if defined(NUCLEO_F756)
#define IS_NUCLEO_DEVKIT 1
#else
#define IS_NUCLEO_DEVKIT 0
#endif
#endif

#ifndef CONTROL_ENABLE
#define CONTROL_ENABLE (CONTROL_HALT|CONTROL_FEED_HOLD|CONTROL_CYCLE_START)
#endif

#if defined(BOARD_PROTONEER_3XX)
  #include "boards/protoneer_3.xx_map.h"
#elif defined(BOARD_GENERIC_UNO)
  #include "boards/uno_map.h"
#elif defined(BOARD_REFERENCE)
  #include "boards/reference_map.h"
#elif defined(BOARD_MY_MACHINE)
  #include "boards/my_machine_map.h"
#else // default board
  #include "boards/generic_map.h"
#endif

#if DRIVER_SPINDLE_ENABLE && !defined(SPINDLE_ENABLE_PIN)
#warning "Selected spindle is not supported!"
#undef DRIVER_SPINDLE_ENABLE
#define DRIVER_SPINDLE_ENABLE 0
#endif

#if DRIVER_SPINDLE_DIR_ENABLE && !defined(SPINDLE_DIRECTION_PIN)
#warning "Selected spindle is not fully supported - no direction output!"
#undef DRIVER_SPINDLE_DIR_ENABLE
#define DRIVER_SPINDLE_DIR_ENABLE 0
#endif

#if DRIVER_SPINDLE_PWM_ENABLE && (!DRIVER_SPINDLE_ENABLE || !defined(SPINDLE_PWM_PIN))
#warning "Selected spindle is not supported!"
#undef DRIVER_SPINDLE_PWM_ENABLE
#define DRIVER_SPINDLE_PWM_ENABLE 0
#ifdef SPINDLE_PWM_PORT_BASE
#undef SPINDLE_PWM_PORT_BASE
#endif
#endif

// Define timer allocations.

#define STEPPER_TIMER_N             5
#define STEPPER_TIMER_BASE          timerBase(STEPPER_TIMER_N)
#define STEPPER_TIMER               timer(STEPPER_TIMER_N)
#define STEPPER_TIMER_CLKEN         timerCLKEN(STEPPER_TIMER_N)
#define STEPPER_TIMER_IRQn          timerINT(STEPPER_TIMER_N)
#define STEPPER_TIMER_IRQHandler    timerHANDLER(STEPPER_TIMER_N)

#if ETHERNET_ENABLE

    /**ETH GPIO Configuration
    PC1     ------> ETH_MDC
    PA1     ------> ETH_REF_CLK
    PA2     ------> ETH_MDIO
    PA7     ------> ETH_CRS_DV
    PC4     ------> ETH_RXD0
    PC5     ------> ETH_RXD1
    PB11     ------> ETH_TX_EN
    PB12     ------> ETH_TXD0
    PB13     ------> ETH_TXD1
    */

#endif

#if defined(AUXOUTPUT0_PWM_PORT) || defined(AUXOUTPUT1_PWM_PORT) ||\
     defined(AUXOUTPUT0_ANALOG_PORT) || defined( AUXOUTPUT1_ANALOG_PORT) ||\
      defined(MCP3221_ENABLE)
#define AUX_ANALOG 1
#else
#define AUX_ANALOG 0
#endif

#if SPINDLE_ENCODER_ENABLE

#define RPM_COUNTER_N               3
#define RPM_COUNTER_BASE            timerBase(RPM_COUNTER_N)
#define RPM_COUNTER                 timer(RPM_COUNTER_N)
#define RPM_COUNTER_CLKEN           timerCLKEN(RPM_COUNTER_N)
#define RPM_COUNTER_IRQn            timerINT(RPM_COUNTER_N)
#define RPM_COUNTER_IRQHandler      timerHANDLER(RPM_COUNTER_N)

#define RPM_TIMER_N                 2
#define RPM_TIMER_BASE              timerBase(RPM_TIMER_N)
#define RPM_TIMER                   timer(RPM_TIMER_N)
#define RPM_TIMER_CLKEN             timerCLKEN(RPM_TIMER_N)
#define RPM_TIMER_IRQn              timerINT(RPM_TIMER_N)
#define RPM_TIMER_IRQHandler        timerHANDLER(RPM_TIMER_N)

#endif

#define IS_TIMER_CLAIMED(INSTANCE) (((INSTANCE) == STEPPER_TIMER_BASE) || \
                                    ((INSTANCE) == RPM_TIMER_BASE) || \
                                    ((INSTANCE) == RPM_COUNTER_BASE))

// Adjust these values to get more accurate step pulse timings when required, e.g if using high step rates.
// The default values below are calibrated for 5 microsecond pulses on a F446 @ 180 MHz.
// NOTE: step output mode, number of axes and compiler optimization setting may all affect these values.

// Minimum pulse off time.
#ifndef STEP_PULSE_TOFF_MIN
#define STEP_PULSE_TOFF_MIN 2.0f
#endif
// Time from main stepper interrupt to pulse output, must be less than STEP_PULSE_TOFF_MIN.
// Adjust for correct pulse off time after configuring and running at a step rate > max possible.
#ifndef STEP_PULSE_TON_LATENCY
#if SPINDLE_SYNC_ENABLE
#define STEP_PULSE_TON_LATENCY 1.35f
#else
#define STEP_PULSE_TON_LATENCY 0.65f
#endif
#endif
// Time from step out to step reset.
// Adjust for correct step pulse time
#ifndef STEP_PULSE_TOFF_LATENCY
#define STEP_PULSE_TOFF_LATENCY 0.65f
#endif
// Only used when step injection is enabled (stepper spindle and plasma THC)
#ifndef STEP_PULSE_LATENCY
#define STEP_PULSE_LATENCY 1.0f // microseconds
#endif

// End configuration

#include "grbl/driver_opts2.h"

#ifndef I2C_PORT
#define I2C_PORT 2 // GPIOB, SCL_PIN = 10, SDA_PIN = 11
#endif

#ifndef SPI_PORT
#define SPI_PORT 1
#endif

#if KEYPAD_ENABLE == 1 && !defined(I2C_STROBE_PORT)
#error Keypad plugin not supported!
#elif I2C_STROBE_ENABLE && !defined(I2C_STROBE_PORT)
#error I2C strobe not supported!
#endif

#if SDCARD_ENABLE
#ifndef SDCARD_SDIO
#define SDCARD_SDIO 0
#endif
#if !SDCARD_SDIO && !defined(SD_CS_PORT)
#error SD card plugin not supported!
#endif
#endif

#ifndef STEP_PINMODE
#define STEP_PINMODE PINMODE_OUTPUT
#endif

#ifndef DIRECTION_PINMODE
#define DIRECTION_PINMODE PINMODE_OUTPUT
#endif

#ifndef STEPPERS_ENABLE_PINMODE
#define STEPPERS_ENABLE_PINMODE PINMODE_OUTPUT
#endif

typedef struct {
    pin_function_t id;
    pin_cap_t cap;
    pin_mode_t mode;
    uint8_t pin;
    uint32_t bit;
    GPIO_TypeDef *port;
    pin_group_t group;
    uint8_t user_port;
    volatile bool active;
    ioport_interrupt_callback_ptr interrupt_callback;
    ADC_HandleTypeDef *adc;
    uint32_t channel; // ADC channel;
    const char *description;
} input_signal_t;

typedef struct {
    float value;
    ioports_pwm_t data;
    const pwm_signal_t *port;
} pwm_out_t;

typedef struct {
    pin_function_t id;
    GPIO_TypeDef *port;
    uint8_t pin;
    uint32_t bit;
    pin_group_t group;
    pin_mode_t mode;
    const char *description;
    pwm_out_t *pwm;
} output_signal_t;

typedef struct {
    uint8_t n_pins;
    union {
        input_signal_t *inputs;
        output_signal_t *outputs;
    } pins;
} pin_group_pins_t;

bool driver_init (void);
void Driver_IncTick (void);
void gpio_irq_enable (const input_signal_t *input, pin_irq_mode_t irq_mode);
void ioports_init(pin_group_pins_t *aux_inputs, pin_group_pins_t *aux_outputs);
#if AUX_ANALOG
void ioports_init_analog (pin_group_pins_t *aux_inputs, pin_group_pins_t *aux_outputs);
#endif
void ioports_event (input_signal_t *input);
const pwm_signal_t *get_pwm_timer (GPIO_TypeDef *port, uint8_t pin);

#endif // __DRIVER_H__
