/*
 * Comfortflasher V1.0b
 * Firmware for Comfortflasher Module M1
 * Created: 04.02.2017 01:18:23
 * Author : Steven Tzschentke (aka Kampfkuchen)
 * Initiated in 626ge.nufag.de board for Mazda 626 GE
 * Thread: http://www.626ge.de/viewtopic.php?f=17&t=5663
 */ 

#include <avr/interrupt.h>

// define CPU speed - actual speed is set using CLKPSR in main()
#define F_CPU 8000000UL

//Defines
#define no_lever 0x00
#define right_lever 0x01
#define left_lever 0x02
#define both_levers 0x03
#define set 0x04
#define activated 1
#define deactivated 0
#define reset 0

//ADC paramter list
#define left_lever_min_adc_value 60
#define left_lever_max_adc_value 100
#define right_lever_min_adc_value 120
#define right_lever_max_adc_value 150
#define both_lever_min_adc_value 180

//Time parameter list
#define time_in 10
#define time_out 50
#define blink_time 300

//Variables
volatile uint16_t counter = 0;
uint8_t lever = 0;
uint8_t state = 0;
uint8_t comfort = 0;

void setup(){
		// Set CPU Clock
		CCP = 0xD8;				//Set CCP with correct signature
		CLKMSR = 0x00;
		CLKPSR = 0;				//Set CPU Clock Prescaler: 0000 (8000000Hz / 1)
				
		DIDR0  = 0x04;			//Deactivate digital Memory on PB2 to save energy
		DDRB  = 0x03;			//PB0+1 as outputs
		
		//Set CTC value (1000)
		OCR0AH = 0x03;
		OCR0AL = 0xE8;
		
		//Configure Timer0
		TCCR0B = (1<<CS01) | (1<<WGM02); //Set prescaler to 8 and CTC mode
		TIMSK0 |= (1<<OCIE0A);	//Activate CTC mode

		ADCSRB = 0x00;			//Activate Freerun for ADC
		ADMUX  = 0x02;			//Set ADC Multiplexer to to PB2
		ADCSRA = 0xE6;			//Set ADC operation-mode and prescaler
		sei();					//activate global interrupts
		
}

void read_lever (){
	
	uint8_t AD_Result = ADCL;
	if(AD_Result > both_lever_min_adc_value){
		lever = both_levers;		
	}
	else if (AD_Result >= left_lever_min_adc_value && AD_Result <= left_lever_max_adc_value){
		lever = left_lever;
	}
	else if (AD_Result >= right_lever_min_adc_value && AD_Result <= right_lever_max_adc_value)
	{
		lever = right_lever;
	}
	else
	{
		lever = no_lever;
	}
}

ISR (TIM0_COMPA_vect)
{
	counter++;
}

int main(void)
{
	//Start setup first
	setup();
		
    while (1) 
    {
		
		//Read ADC values and set lever-state
		read_lever();
				
		//Switch to the case for the lever-state
		switch (lever) 
		{
			case no_lever:	//no lever is set
				
				//check if the lever was released in time and was not hold too long
				if (counter < time_out && counter > time_in && !comfort)	
				{
					PORTB = state;			//Set output for the last known lever state
					comfort = activated;	//member the comfort-state
					counter = reset;
				}
				break;
				
			case right_lever:
				state = right_lever;		//hold the lever-state
				break;
				
			case left_lever:
				state = left_lever;			//hold the lever-state
				break;
			
			case both_levers:
				//lock the microprocessor for everything and let him dying in the never ending loop until his energy is lost
				while(1){
					PORTB = no_lever;
				}
				break;		
			
		}
	
		//if comfort is activated check the time
		//and release the output after blink_time
		if (comfort)
		{
			if (counter >= blink_time)
			{
				//lock the microprocessor for everything and let him dying in the never ending loop until his energy is lost
				while(1){
					PORTB = no_lever;
					}
			}
		}
	}
		
}



