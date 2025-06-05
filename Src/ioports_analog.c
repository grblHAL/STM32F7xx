/*
  ioports_analog.c - driver code for STM32F4xx ARM processors

  Part of grblHAL

  Copyright (c) 2023-2025 Terje Io

  grblHAL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  grblHAL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public Licens
  along with grblHAL. If not, see <http://www.gnu.org/licenses/>.
*/

#include "driver.h"

#if AUX_ANALOG

#ifdef AUXOUTPUT0_PWM_PORT
#define PWM_OUT0 1
#else
#define PWM_OUT0 0
#endif

#ifdef AUXOUTPUT1_PWM_PORT
#define PWM_OUT1 1
#else
#define PWM_OUT1 0
#endif

#define AUX_ANALOG_OUT (PWM_OUT0 + PWM_OUT1)

#include "grbl/ioports.h"

typedef struct {
    GPIO_TypeDef *port;
    uint8_t pin;
    uint8_t alt;
    ADC_TypeDef *adc;
    uint32_t ch;
} adc_map_t;

static const adc_map_t adc_map[] = {
    { GPIOA,  0, 123, ADC1, ADC_CHANNEL_0 },
    { GPIOA,  1, 123, ADC1, ADC_CHANNEL_1 },
    { GPIOA,  2, 123, ADC1, ADC_CHANNEL_2 },
    { GPIOA,  3, 123, ADC1, ADC_CHANNEL_3 },
    { GPIOA,  4,  12, ADC2, ADC_CHANNEL_4 },
    { GPIOA,  5,  12, ADC2, ADC_CHANNEL_5 },
    { GPIOA,  6,  12, ADC2, ADC_CHANNEL_6 },
    { GPIOA,  7,  12, ADC2, ADC_CHANNEL_7 },
    { GPIOB,  0,  12, ADC2, ADC_CHANNEL_8 },
    { GPIOB,  1,  12, ADC2, ADC_CHANNEL_9 },
    { GPIOC,  0, 123, ADC1, ADC_CHANNEL_10 },
    { GPIOC,  1, 123, ADC1, ADC_CHANNEL_11 },
    { GPIOC,  2, 123, ADC1, ADC_CHANNEL_12 },
    { GPIOC,  3, 123, ADC1, ADC_CHANNEL_13 },
    { GPIOC,  4,  12, ADC2, ADC_CHANNEL_14 },
    { GPIOC,  5,  12, ADC2, ADC_CHANNEL_15 },
    { GPIOF,  3,   3, ADC3, ADC_CHANNEL_9 },
    { GPIOF,  4,   3, ADC3, ADC_CHANNEL_14 },
    { GPIOF,  5,   3, ADC3, ADC_CHANNEL_15 },
    { GPIOF,  6,   3, ADC3, ADC_CHANNEL_4 },
    { GPIOF,  7,   3, ADC3, ADC_CHANNEL_5 },
    { GPIOF,  8,   3, ADC3, ADC_CHANNEL_6 },
    { GPIOF,  9,   3, ADC3, ADC_CHANNEL_7 },
    { GPIOF, 10,   3, ADC3, ADC_CHANNEL_8 }
};

static io_ports_data_t analog;
static input_signal_t *aux_in_analog;
static output_signal_t *aux_out_analog;
static ADC_ChannelConfTypeDef adc_config = {
    .Rank = 1,
    .SamplingTime = ADC_SAMPLETIME_3CYCLES
};

#if AUX_ANALOG_OUT

static float pwm_get_value (xbar_t *output)
{
    return output->id < analog.out.n_ports ? aux_out_analog[output->id].pwm->value : -1.0f;
}

static void pwm_out (uint8_t port, float value)
{
    if(port < analog.out.n_ports && aux_out_analog[port].pwm) {

        uint_fast16_t pwm_value = ioports_compute_pwm_value(&aux_out_analog[port].pwm->data, value);
        const pwm_signal_t *pwm = aux_out_analog[port].pwm->port;

        aux_out_analog[port].pwm->value = value;

        if(pwm_value == aux_out_analog[port].pwm->data.off_value) {
            if(aux_out_analog[port].pwm->data.always_on) {
                *pwm->ccr = aux_out_analog[port].pwm->data.off_value;
                if(pwm->timer == TIM1 || pwm->timer == TIM8)
                    pwm->timer->BDTR |= TIM_BDTR_MOE;
                *pwm->ccr = 0;
            } else {
                if(pwm->timer == TIM1 || pwm->timer == TIM8)
                    pwm->timer->BDTR |= TIM_BDTR_MOE;
                *pwm->ccr = 0;
            }
        } else {
            *pwm->ccr = pwm_value;
            if(pwm->timer == TIM1 || pwm->timer == TIM8)
                pwm->timer->BDTR |= TIM_BDTR_MOE;
        }
    }
}

static bool analog_out (uint8_t port, float value)
{
    if(port < analog.out.n_ports)
        pwm_out(port, value);

    return port < analog.out.n_ports;
}

static bool init_pwm (xbar_t *output, pwm_config_t *config, bool persistent)
{
    bool ok;

    if(aux_out_analog[output->id].pwm == NULL) {

        pwm_out_t *pwm;

        if((pwm = calloc(sizeof(pwm_out_t), 1))) {
            if((pwm->port = pwm_claim((GPIO_TypeDef *)output->port, output->pin))) {
                pwm_enable(pwm->port);
                aux_out_analog[output->id].pwm = pwm;
            } else
                free(pwm);
        }
    }

    if((ok = !!aux_out_analog[output->id].pwm)) {

        uint32_t prescaler = 0, clock_hz = pwm_get_clock_hz(aux_out_analog[output->id].pwm->port);

        do {
            prescaler++;
            ok = ioports_precompute_pwm_values(config, &aux_out_analog[output->id].pwm->data, clock_hz / prescaler);
        } while(ok && aux_out_analog[output->id].pwm->data.period > 65530);

        if(ok) {

            pwm_config(aux_out_analog[output->id].pwm->port, prescaler, aux_out_analog[output->id].pwm->data.period, config->invert);

            aux_out_analog[output->id].mode.pwm = !config->servo_mode;
            aux_out_analog[output->id].mode.servo_pwm = config->servo_mode;

            pwm_out(output->id, config->min);
        }
    }

    if(!ok && !aux_out_analog[output->id].mode.claimed)
        hal.port.claim(Port_Analog, Port_Output, &output->id, "N/A");

    return ok;
}

#endif // AUX_ANALOG_OUT

static inline int32_t adc_read (ADC_HandleTypeDef *adc, uint32_t channel)
{
    int32_t value = -1;

    if(adc) {

        if(adc_config.Channel != channel) {
            adc_config.Channel = channel;
            HAL_ADC_ConfigChannel(adc, &adc_config);
        }

        if(HAL_ADC_Start(adc) == HAL_OK && HAL_ADC_PollForConversion(adc, 2) == HAL_OK)
            value = (int32_t)HAL_ADC_GetValue(adc);
    }

    return value;
}

static float analog_in_state (xbar_t *input)
{
    return input->id < analog.in.n_ports ? (float)adc_read(aux_in_analog[input->id].adc, aux_in_analog[input->id].channel) : -1.0f;
}

static int32_t wait_on_input (uint8_t port, wait_mode_t wait_mode, float timeout)
{
    return port < analog.in.n_ports ? adc_read(aux_in_analog[port].adc, aux_in_analog[port].channel) : -1;
}

static bool set_function (xbar_t *port, pin_function_t function)
{
    if(port->mode.input)
        aux_in_analog[port->id].id = function;
    else
        aux_out_analog[port->id].id = function;

    return true;
}

static xbar_t *get_pin_info (io_port_direction_t dir, uint8_t port)
{
    static xbar_t pin;

    xbar_t *info = NULL;

    memset(&pin, 0, sizeof(xbar_t));

    pin.set_function = set_function;

    switch(dir) {

        case Port_Input:
            if(port < analog.in.n_ports) {
                pin.id = port;
                pin.mode = aux_in_analog[pin.id].mode;
                pin.cap = aux_in_analog[port].cap;
                pin.function = aux_in_analog[port].id;
                pin.group = aux_in_analog[port].group;
                pin.pin = aux_in_analog[port].pin;
                pin.port = (void *)aux_in_analog[port].port;
                pin.description = aux_in_analog[port].description;
                pin.get_value = analog_in_state;
                info = &pin;
            }
            break;

        case Port_Output:
#if AUX_ANALOG_OUT
            if(port < analog.out.n_ports) {
                pin.id = port;
                pin.port = aux_out_analog[port].port;
                pin.mode = aux_out_analog[port].mode;
                pin.mode.pwm = !pin.mode.servo_pwm; //?? for easy filtering
                XBAR_SET_CAP(pin.cap, pin.mode);
                pin.function = aux_out_analog[port].id;
                pin.group = aux_out_analog[port].group;
                pin.pin = aux_out_analog[port].pin;
                pin.port = (void *)aux_out_analog[port].port;
                pin.description = aux_out_analog[port].description;
                pin.get_value = pwm_get_value;
                pin.config = init_pwm;
                info = &pin;
            }
#endif // AUX_ANALOG_OUT
            break;
    }

    return info;
}

static void set_pin_description (io_port_direction_t dir, uint8_t port, const char *description)
{
    if(dir == Port_Input && port < analog.in.n_ports)
        aux_in_analog[port].description = description;
    else if(port < analog.out.n_ports)
        aux_out_analog[port].description = description;
}

void ioports_init_analog (pin_group_pins_t *aux_inputs, pin_group_pins_t *aux_outputs)
{
    io_analog_t ports = {
        .ports = &analog,
#if AUX_ANALOG_OUT
        .analog_out = analog_out,
#endif
        .get_pin_info = get_pin_info,
        .wait_on_input = wait_on_input,
        .set_pin_description = set_pin_description
    };

    aux_in_analog = aux_inputs->pins.inputs;
    aux_out_analog = aux_outputs->pins.outputs;

    analog.in.n_ports = aux_inputs->n_pins;
    analog.out.n_ports = aux_outputs->n_pins;

    if(ioports_add_analog(&ports)) {

        if(analog.in.n_ports) {

            GPIO_InitTypeDef gpio_init = {
                .Mode = GPIO_MODE_ANALOG,
                .Pull = GPIO_NOPULL
            };

            uint_fast8_t i;

            for(i = 0; i < analog.in.n_ports; i++) {

                uint_fast8_t j = sizeof(adc_map) / sizeof(adc_map_t);

                do {
                    j--;
                    if(adc_map[j].port == aux_inputs->pins.inputs[i].port && adc_map[j].pin == aux_inputs->pins.inputs[i].pin) {

                        ADC_HandleTypeDef *adc;

                        if((adc = calloc(sizeof(ADC_HandleTypeDef), 1))) {

                            if(adc_map[j].alt == 3)
                                __HAL_RCC_ADC3_CLK_ENABLE();
                            else if(adc_map[j].alt == 12)
                                __HAL_RCC_ADC2_CLK_ENABLE();
                            else
                                __HAL_RCC_ADC1_CLK_ENABLE();

                            gpio_init.Pin = aux_inputs->pins.inputs[i].bit;
                            HAL_GPIO_Init(aux_inputs->pins.inputs[i].port, &gpio_init);

                            adc_config.Channel = aux_inputs->pins.inputs[i].channel = adc_map[j].ch;

                            adc->Instance = adc_map[j].adc;
                            adc->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
                            adc->Init.Resolution = ADC_RESOLUTION_12B;
                            adc->Init.ScanConvMode = DISABLE;
                            adc->Init.ContinuousConvMode = DISABLE;
                            adc->Init.DiscontinuousConvMode = DISABLE;
                            adc->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
                            adc->Init.ExternalTrigConv = ADC_SOFTWARE_START;
                            adc->Init.DataAlign = ADC_DATAALIGN_RIGHT;
                            adc->Init.NbrOfConversion = 1;
                            adc->Init.DMAContinuousRequests = DISABLE;
                            adc->Init.EOCSelection = ADC_EOC_SINGLE_CONV;
                            if(HAL_ADC_Init(adc) == HAL_OK && HAL_ADC_ConfigChannel(adc, &adc_config) == HAL_OK)
                                aux_inputs->pins.inputs[i].adc = adc;
                            else
                                analog.in.n_ports--;
                        }
                        break;
                    }
                } while(j);
            }
        }

#if AUX_ANALOG_OUT

        if(analog.out.n_ports) {

            xbar_t *pin;
            uint_fast8_t i;
            pwm_config_t config = {
                .freq_hz = 5000.0f,
                .min = 0.0f,
                .max = 100.0f,
                .off_value = 0.0f,
                .min_value = 0.0f,
                .max_value = 100.0f,
                .invert = Off
            };

            hal.port.analog_out = analog_out;

            for(i = 0; i < analog.out.n_ports; i++) {
                if((pin = get_pin_info(Port_Output, i)))
                    pin->config(pin, &config, false);
            }
        }

#endif // AUX_ANALOG_OUT
    }
}

#endif
