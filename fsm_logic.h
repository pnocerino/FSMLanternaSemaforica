/*
 * fsm_logic.h
 *
 *  Created on: Apr 3, 2024
 *      Author: Gennaro
 */

#ifndef INC_FSM_LOGIC_H_
#define INC_FSM_LOGIC_H_

#include "led.h"
#include "button.h"
#include "timer.h"


#define FSM_ERR		(-1)
#define FSM_OK		(0)

/**
 * Public function to initialize the FMS
 */
int8_t FSM_init(led_t* red_led, led_t* yellow_led, led_t* green_led, button_t* button, timer_t* timer);

/**
 * Public function to be called to evolve the FSM Status
 * This function allows to read the inputs and update the current status and the output
 * The function must be called iteratively after the machine has been initialized
 */
int8_t FSM_step();


#endif /* INC_FSM_LOGIC_H_ */
