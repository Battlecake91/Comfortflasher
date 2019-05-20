/*/*
 * Comfortblink_ATTiny13A.c
 * Version V1.2b
 * Author : Steven Tzschentke (aka Kampfkuchen)
 * Initiated in 626ge.nufag.de board for Mazda 626 GE
 * Thread: http://www.626ge.de/viewtopic.php?f=17&t=5663
 * Changes:
 * V1.2b: fixed cycle-setup at normal blinking 
 * adding time-out
 */ 

#define F_CPU 9600000UL

#define lever_mask 0x0C
#define feedback_pin 0x10

#define left 0x01
#define right 0x02
#define both 0x03
#define no_lever 0x00

#define active 1

#define time_in 40
#define time_out 300
#define time_out_Feedback 1000
#define time_out_both 700

#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <stdint.h>                            

volatile uint16_t counter = 0;
uint8_t detect_count = 0;
uint8_t lever = 0;
uint8_t Feedback = 0;
uint8_t Feedback_last = 0;
uint8_t Feedback_count = 0;
uint8_t Feedback_count_last = 0;
uint8_t state = 0;
uint8_t comfort = 0;
uint8_t cycles = 0;
uint16_t counter_last_check = 0;
uint16_t both_counter = 0;
uint16_t time_last_Feedback = 0;
uint8_t first_detection = 0;


ISR (TIM0_COMPA_vect)
{
	counter++;

}

void setup_cycles(){
	
	if (Feedback_count > 2 )
	{
		eeprom_update_byte((uint8_t*)16, Feedback_count);
	}
}

void stop_program(){
	
	while(1){
		PORTB = no_lever;
	}

}

void get_inputs(){
	
	//Writing input in variable
	//and shifting bits to the right
	
	uint8_t input = PINB;
		
	lever = input & lever_mask;
	lever = lever >> 2;
	
	if (!first_detection)
	{
		first_detection = lever;
	}
	
	Feedback = input & feedback_pin;
	Feedback = Feedback >> 4;
	detect_count++;
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
	
		
	while (1) 
    {

		get_inputs();
		
		//Anti-bounce: only check after 2ms if feedback signal has changed
		if(counter - counter_last_check > 2){
			//check if the signal has a falling edge
			if (Feedback != Feedback_last && !Feedback)
			{
				//increment the counter if a falling edge was detected
				Feedback_count++;
				//save the last counter-state for the last feedback-edge
				time_last_Feedback = counter;
				
			//Check if the last feedback was too long ago
			}else if(counter - time_last_Feedback > time_out_Feedback){
				stop_program();
			}
			//stop the program if nothing happened until time-out except both 
			if (!comfort && counter >= time_out && lever != both)
			{
				stop_program();
			}
			
			Feedback_last = Feedback;
			counter_last_check = counter;
		}
		switch (lever)
		{
			case no_lever:				//no lever is set
				
				if(!comfort && counter >= time_in){
					//check if the lever was released in time and was not hold too short
					PORTB = state;			//Set output from the last known lever state
					comfort = active;		//member the comfort-state
					counter = 0;			//reset the counter for the blink-cycle
				}
				break;
			
			case right:
				
				//Only the first known state changes the state value
				if (detect_count == 1)
				{
					state = right;		//hold the lever-state	
					both_counter = 0;
				}
				
				break;
					
			case left:
					
				//Only the first known state changes the state value
				if (detect_count == 1)
				{
					state = left;			//hold the lever-state
					both_counter = 0;
				}
				
				break;
					
			case both:
				
				if (comfort)
				{
					//Stop the program if both lever are detected for specific times
					both_counter++;
					if (both_counter > time_out_both)
					{
						stop_program();
					}
					
				}else if (first_detection == both)
				{
					//If both levers detected at the beginning and no comfort-mode, setup cycles
					setup_cycles();
				}
				break;
				
			}
		
		if (Feedback_count >= cycles && comfort)
		{
			//stop after saved times of feedbacks
			stop_program();
		}
	}
}

