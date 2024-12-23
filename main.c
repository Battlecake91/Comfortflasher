/*
 * Comfortblink_ATTiny13A.c
 * Version V2.4b
 * Author : Steven Tzschentke (aka Kampfkuchen)
 * Initiated in 626ge.nufag.de board for Mazda 626 GE
 * Thread: http://www.626ge.de/viewtopic.php?f=17&t=5663
 *
 * Change-log:
 * V1.2b:	fixed cycle-setup at normal blinking 
 *			adding time-out
 * V2.0b:	Both-detection now works with timer
 *			Brownout-Detection at 2.7V
 *			fix the forever-blinking-bug  
 *			some optimizing with counters
 *			(alternative cycle-timing)
 *V2.2b:	added Feedback-Timeout function
 *V2.3b:	added initial cycle-setup
 *V2.4b:	MIN_TIME from 50 to 100
 */ 

#define F_CPU 9600000UL

#define LEVER_MASK 0x0C
#define FEEDBACK_PIN 0x10

#define LEFT 0x01
#define RIGHT 0x02
#define BOTH 0x03
#define NO_LEVER 0x00

#define INACTIVE 0
#define ACTIVE 1
#define DISABLED 2
#define INITIAL_CYCLES 3


#define MIN_TIME 100
#define TIMEOUT_COMFORT 500
#define TIMEOUT_FEEDBACK 1000
#define TIME_BOTH 50
#define MAX_CYCLES 10
#define MIN_CYCLES 2

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <stdint.h>                            

volatile uint16_t counter = 0;
uint8_t lever = 0;
uint8_t feedback = 0;
uint8_t feedback_last = 0;
uint8_t feedback_count = 0;
uint8_t state = 0;
uint8_t comfort = 0;
uint8_t cycles = 0;
uint16_t counter_last_check = 0;
uint16_t both_counter = 0;
uint16_t time_last_feedback = 0;
uint16_t comfort_start_time = 0;
uint16_t time_first_detection = 0;
uint8_t first_detection = 0;
uint16_t hold_time = 0;

//Program the fuses to 9,6MHz, 14 Clks-0ms Startup, BOD to 2V7. 
FUSES =
{
	.low = 0x22,
	.high = 0xFB,
};

ISR (TIM0_COMPA_vect)
{
	counter++;
}

void setup_cycles(uint8_t value){

	if (value > MIN_CYCLES && value < MAX_CYCLES )
	{
		eeprom_update_byte((uint8_t*)16, value);
	}
	
}

void stop_program(){
	
	while(1){
		PORTB = NO_LEVER;
	}
}

void get_inputs(){
	
	//Writing input in variable
	//and shifting bits to the right
	uint8_t input = PINB;
	
	//mask out the lever-bits with fixed variables	
	lever = input & LEVER_MASK;
	lever = lever >> 2;
	
	
	if (!first_detection)
	{
		first_detection = lever;
		
		//If the first-detection is not both, set the state for the outputs from the first-detection
		if (first_detection != BOTH)
		{
			state = first_detection;	//Set the lever-state
			time_first_detection = counter;
		}else
		{
			comfort = DISABLED;			//disabling the comfort-mode 
		}
	}
	feedback = input & FEEDBACK_PIN;
	feedback = feedback >> 4;
}

int main(void)
{
 
    DDRB = 0x03;						//PB0 and PB1 as output
	
	TCCR0A = (1<<WGM01);				//Set Mode to CTC mode
	TCCR0B = (1<<CS01) | (1<<CS00);		//Set Prescaler to 64
	
	OCR0A  = 17;						//Set CTC value (17) for 1kHz rate, 1ms period time
	
	TIMSK0 = (1<<OCIE0A);				//Enable Interrupt at CTC value from OCR0A
	
	sei();
	
	//read flash-cycles from memory 
	cycles = eeprom_read_byte((uint8_t*)16);
	
	if(cycles >= MAX_CYCLES || cycles <= MIN_CYCLES){
		setup_cycles(INITIAL_CYCLES);
	}
	
	while (1) 
    {

		get_inputs();
		
		uint16_t time = counter;
		
		//Anti-bounce: only check after 2ms if feedback signal has changed
		if (counter - counter_last_check >= 2)
		{
			//check if the signal has a falling edge
			if (feedback != feedback_last && !feedback)
			{
				feedback_count++;				//increment the counter if a falling edge was detected
				time_last_feedback = time;		//save the time the last feedback was detected
			}
			
			//Timeout for Feedback. If no feedback is detected for the parametrized time stop the function
			else if (time - time_last_feedback >= TIMEOUT_FEEDBACK)
			{
				stop_program();
			}
			
			//disable the comfort-mode if there is no comfort-trigger within timeout detected 
			if (comfort == INACTIVE && time >= TIMEOUT_COMFORT)
			{
				comfort = DISABLED;				//disable comfort mode because the time for comfort was to high
			}
			
			if (comfort == ACTIVE)
			{
				//Increment the both_counter twice for 2ms each run through this function
				if (lever == BOTH)
				{
					 both_counter = both_counter + 2;
				}
				else
				{
					//reset the both_counter if there is only one or none lever
					both_counter = 0;
				}
				
				//if the both_counter or the cycles reach the limit/setup value
				if (both_counter >= TIME_BOTH || feedback_count >= cycles)
				{
					stop_program();
				}
				

				
			}else if (first_detection == BOTH)
			{
				//If both levers detected at the beginning and no comfort-mode, setup cycles
				setup_cycles(feedback_count);
			}
			
			//save the last counter-state for the last feedback-edge
			feedback_last = feedback;
			counter_last_check = time;
		}
		
		if (lever == NO_LEVER)					//no lever is set
		{			
			//Check if the first detection was both levers and setup-mode or just release of the lever
			if (!comfort && first_detection != BOTH)
			{
				//if no comfort-mode is set and the first detection was left or right, calculate the time the lever was hold for
				hold_time = time - time_first_detection;
			}
			
			//check if the lever was released in time and was not hold too short
			if (!comfort && hold_time >= MIN_TIME)
			{
				PORTB = state;					//Set output-state from the first-detection
				comfort = ACTIVE;				//member the comfort-state
				comfort_start_time = counter;	//save the comfort-start-time for feedback-timeout / mode without feedback
			}		
		}
	}
}

