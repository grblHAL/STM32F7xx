/*

  encoders.c - driver code for STM32F7xx ARM processors

  *** WIP ***

  Part of grblHAL

  Copyright (c) 2026 Terje Io

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

#include "driver.h"

#if QEI_ENABLE && defined(QEI_PORT)

#include "grbl/task.h"
#include "grbl/encoders.h"

typedef struct {
    uint8_t pin_a;
    uint8_t pin_b;
    uint8_t af;
    GPIO_TypeDef *port_a;
    GPIO_TypeDef *port_b;
    TIM_TypeDef *timer;
} stm32_encoder_hw_t;

typedef struct {
    encoder_t encoder;
    encoder_data_t data;
    encoder_cfg_t settings;
    encoder_event_t event;
    void *context;
    int32_t count;
    int16_t count_h;
    int32_t vel_count;
    volatile uint32_t vel_timeout;
    uint32_t vel_timestamp;
    const stm32_encoder_hw_t *st_encoder;
    encoder_on_event_ptr on_event;
} stm32_encoder_t;

static const stm32_encoder_hw_t encoders[] = {
    { .port_a = GPIOB, .pin_a = 4, .port_b = GPIOB, .pin_b = 5, .af = GPIO_AF2_TIM3, .timer = timer(3) }
};

static stm32_encoder_t qei = {0};

static encoder_data_t *encoder_get_data (encoder_t *encoder)
{
    return &qei.data;
}

static bool qei_configure (encoder_t *encoder, encoder_cfg_t *settings)
{
    if(qei.vel_timeout != settings->vel_timeout)
        qei.vel_timestamp = hal.get_elapsed_ticks();

    memcpy(&qei.settings, settings, sizeof(encoder_cfg_t));

    return true;
}

static void encoder_reset (encoder_t *encoder)
{
    qei.vel_timeout = 0;
    qei.data.position = qei.count = qei.vel_count = 0;
    qei.vel_timestamp = uwTick;
//    qei.vel_timeout = qei.encoder.axis != 0xFF ? QEI_VELOCITY_TIMEOUT : 0;
    qei.st_encoder->timer->CNT = 0; //stop/start timer?
}

static void encoder_poll (void *data)
{
    if(qei.on_event && qei.count != qei.st_encoder->timer->CNT) {

        qei.data.position = (qei.count_h << 16) | qei.st_encoder->timer->CNT;

        qei.event.position_changed = On;

        // encoder->timer->CR1 & TIM_CR1_DIR -> 0 = up, !=0 down

        qei.on_event(&qei.encoder, &qei.event, qei.context);

        if(qei.vel_timeout && !(--qei.vel_timeout)) {
            qei.data.velocity = abs(qei.count - qei.vel_count) * 1000 / (uwTick - qei.vel_timestamp);
            qei.vel_timestamp = uwTick;
            qei.vel_timeout = qei.settings.vel_timeout;
            if((qei.event.position_changed = qei.data.velocity == 0))
                qei.on_event(&qei.encoder, &qei.event, qei.context);
            qei.vel_count = qei.count;
        }
    }
}

static void encoder_overflow (void *context)
{
    stm32_encoder_t *qei = (stm32_encoder_t *)context;

    if(qei->st_encoder->timer->CR1 & TIM_CR1_DIR)
        qei->count_h++;
    else
        qei->count_h--;
}

static bool encoder_claim (encoder_on_event_ptr event_handler, void *context)
{
    if(event_handler == NULL || qei.on_event)
        return false;

    qei.context = context;
    qei.on_event = event_handler;
    qei.encoder.reset = encoder_reset;
    qei.encoder.get_data = encoder_get_data;
    qei.encoder.configure = qei_configure;
    qei.st_encoder->timer->DIER |= TIM_DIER_UIE;
    qei.st_encoder->timer->CR1 = TIM_CR1_CEN|TIM_CR1_URS;

    task_add_systick(encoder_poll, NULL);

    return true;
}

void driver_encoders_init (void)
{
    hal_timer_t timer;

    const stm32_encoder_hw_t *encoder = &encoders[0];

    if((timer = timer_claim(encoder->timer))) {

        timer_clk_enable(encoder->timer);

        GPIO_InitTypeDef GPIO_Init = {
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Mode = GPIO_MODE_AF_PP,
            .Pull = GPIO_NOPULL
        };

        timer_cfg_t cfg = {
            .context = &qei,
            .timeout_callback = encoder_overflow
        };

        GPIO_Init.Pin = (1 << encoder->pin_a);
        GPIO_Init.Alternate = encoder->af;
        HAL_GPIO_Init(encoder->port_a, &GPIO_Init);

        GPIO_Init.Pin = (1 << encoder->pin_b);
        GPIO_Init.Alternate = encoder->af;
        HAL_GPIO_Init(encoder->port_b, &GPIO_Init);

        encoder->timer->SMCR = TIM_SMCR_SMS_0|TIM_SMCR_SMS_1|TIM_SMCR_ETF_0|TIM_SMCR_ETF_1;
        encoder->timer->ARR = 0xFFFF;
        encoder->timer->CCMR1 = TIM_CCMR1_CC1S_0|TIM_CCMR1_CC2S_0;

        qei.st_encoder = encoder;
        qei.encoder.claim = encoder_claim;

        timerCfg(timer, &cfg);

        encoder_register(&qei.encoder);
    }
}

#endif
