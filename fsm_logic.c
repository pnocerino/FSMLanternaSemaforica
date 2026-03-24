/*
 * fsm_logic.c
 *
 *  Created on: Apr 3, 2024
 *      Author: Gennaro
 */


#include "fsm_logic.h"

#define DEFAULT_BUTTON_DELAY		(3000)
#define TOGGLE_PERIOD				(1000)
#define TIMER_ACTIVE_RED_DURATION		(10)
#define TIMER_ACTIVE_YELLOW_DURATION	(15)
#define TIMER_ACTIVE_GREEN_DURATION    (10)
/**
 * Enumeration machine's states
 */
typedef enum fsm_state_enum{
	INACTIVE = 0,
	ACTIVE	= 1
} fsm_state_t;


typedef enum fsm_active_state_enum{
	ACTIVE_RED = 0,
	ACTIVE_YELLOW = 1,
	ACTIVE_GREEN = 2
}fsm_active_state_t;
/*
 * The FSM is a Moore machine that is updated at each step cycle
 * The input are read before evaluating state and output changes
 * therefore, we need to store the value from each input device at a given cycle
 *
 * This structure represent the state read from the input devices at each cycle
 * it buffers the state of each input ensuring that is stable for the overall cycle duration
 */
typedef struct FSM_input_s{
	button_state_t button;	 		//Stable value of the button at current step
	button_state_t button_last;		//Stable value of the button at previous step
	uint8_t timer_elapsed;
	uint8_t timer_elapsed_first;	//Used to avoid the first timer interrupt
}FSM_input_t;


/**
 * FSM Main Structure
 * It is composed by the input and outputs as well as
 * the current status of the machine and the current input reads
 */
typedef struct FSM_s
{
	// FSM Inputs devices
	button_t* button;
	timer_t* timer;

	// FSM Inputs state
	FSM_input_t in;

	// FSM Outputs devices
	led_t* red_led;
	led_t* yellow_led;
	led_t* green_led;

	// FSM current status
	fsm_state_t state;

	// FSM current active status
	fsm_active_state_t a_state;

} FSM_t;


/*
 * Private machine state
 */
static FSM_t fsm;


/*
 * Private function to read and buffers the inputs
 */
static int8_t FSM_read_inputs();


/*
 * Private function to update the current status and the output
 */
static int8_t FSM_update_state();
static int8_t FSM_inactive();
static int8_t FSM_active();



/*
 * Public init function
 */
int8_t FSM_init(led_t* red_led, led_t* yellow_led, led_t* green_led, button_t* button, timer_t* timer){
	uint8_t res = FSM_ERR;

	if(red_led && yellow_led && green_led && button)
	{
		fsm.state = INACTIVE;
		fsm.a_state = ACTIVE_RED;
		fsm.red_led = red_led;
		fsm.yellow_led = yellow_led;
		fsm.green_led = green_led;
		fsm.button = button;
		fsm.timer = timer;

		if(led_set_toggle_period(fsm.yellow_led,TOGGLE_PERIOD ) == LED_OK){
			res = FSM_OK;
		}else{
			res = FSM_ERR;
		}

		if(button_set_delay(fsm.button, DEFAULT_BUTTON_DELAY) != BUTTON_OK){
			res = FSM_ERR;
		}else{
			res = FSM_OK;
		}
	}

	return res;
}

/*
 * Public step function
 */
int8_t FSM_step(){
	int8_t res = FSM_ERR;

	if(FSM_read_inputs() == FSM_OK){
		res = FSM_OK;
	}

	if( (res == FSM_OK) && (FSM_update_state() != FSM_OK) ){
		res = FSM_ERR;
	}

	return res;
}

//********************************************************************************
//******	STATIC FUNCTIONS
//**************************************************************************

static int8_t FSM_read_inputs(){
	int8_t res = FSM_ERR;

	fsm.in.button_last = fsm.in.button;

	if(button_read(fsm.button, &fsm.in.button) == BUTTON_OK){
			res = FSM_OK;
	}
	return res;
}


static int8_t FSM_update_state(){
	int8_t res = FSM_ERR;

	switch(fsm.state)
	{
		case INACTIVE:
			if(FSM_inactive() == FSM_OK){
				res = FSM_OK;
			}
			break;
		case ACTIVE:
			if(FSM_active() == FSM_OK){
				res = FSM_OK;
			}
			break;
		default:
			res = FSM_ERR;
			break;
	}
	return res;
}


static int8_t FSM_inactive(){
	int8_t res = FSM_ERR;

	if( (fsm.in.button == GPIO_PIN_SET) && (fsm.in.button_last != fsm.in.button) ){
		if(led_off(fsm.yellow_led) == LED_OK){
			fsm.state = ACTIVE;
			res = FSM_OK;
		}else{
			res = FSM_ERR;
		}

	}else{

		if(led_toggle(fsm.yellow_led) != LED_OK){
			res = FSM_ERR;
		}else{
			res = FSM_OK;
		}
	}
	return res;
}

static int8_t FSM_active(){
	int8_t res = FSM_ERR;

	if((fsm.in.button == GPIO_PIN_SET) && (fsm.in.button_last != fsm.in.button) ){
		switch(fsm.a_state){
			case ACTIVE_RED:
				if(led_off(fsm.red_led) != LED_OK){
					res = FSM_ERR;
				}else{
					res = FSM_OK;
				}
				break;
			case ACTIVE_YELLOW:
				if(led_off(fsm.yellow_led) != LED_OK){
					res = FSM_ERR;
				}else{
					res = FSM_OK;
				}
				break;
			case ACTIVE_GREEN:
				if(led_off(fsm.green_led) != LED_OK){
					res = FSM_ERR;
				}else{
					res = FSM_OK;
				}
				break;
			default:
				res = FSM_ERR;
				break;
		}
		if(timer_stop(fsm.timer)!= TIMER_OK){
			res = FSM_ERR;
		}else{
			res =FSM_OK;
		}
		fsm.state = INACTIVE;
		fsm.a_state = ACTIVE_RED;

	}else{
		switch(fsm.a_state){
			case ACTIVE_RED:
				if(timer_is_running(fsm.timer) == 0){
					if(timer_set_period(fsm.timer, TIMER_ACTIVE_RED_DURATION) == TIMER_OK
						&& timer_start(fsm.timer) == TIMER_OK
						&& led_on(fsm.red_led) == LED_OK){
						res = FSM_OK;
					}else{
						res = FSM_ERR;
					}
				 }else{
					 res = FSM_OK;
				 }
				 break;
			case ACTIVE_YELLOW:
				if(timer_is_running(fsm.timer) == 0){
					if(timer_set_period(fsm.timer, TIMER_ACTIVE_YELLOW_DURATION) == TIMER_OK
						&& timer_start(fsm.timer) == TIMER_OK
						&& led_on(fsm.yellow_led) == LED_OK){
						res = FSM_OK;
					}else{
						res = FSM_ERR;
					}
				 }else{
					 res = FSM_OK;
				 }
				 break;
			case ACTIVE_GREEN:
				if(timer_is_running(fsm.timer) == 0){
					if(timer_set_period(fsm.timer, TIMER_ACTIVE_GREEN_DURATION) == TIMER_OK
						&& timer_start(fsm.timer) == TIMER_OK
						&& led_on(fsm.green_led) == LED_OK){
						res = FSM_OK;
					}else{
						res = FSM_ERR;
					}
				 }else{
					 res = FSM_OK;
				 }
				 break;

			default:
				res = FSM_ERR;
				break;
		}

	}
	return res;
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* handler) {

	if(!fsm.in.timer_elapsed_first) {
		fsm.in.timer_elapsed_first = 1;
	}
	else {

		timer_period_elapsed(fsm.timer, handler);

		switch(fsm.a_state){
			case ACTIVE_RED:
				led_off(fsm.red_led);
				fsm.a_state = ACTIVE_YELLOW;

				break;

			case ACTIVE_YELLOW:
				led_off(fsm.yellow_led);
				fsm.a_state = ACTIVE_GREEN;
				break;

			case ACTIVE_GREEN:
				led_off(fsm.green_led);
				fsm.a_state = ACTIVE_RED;
				break;

			default:
				break;
		}
	}
}


