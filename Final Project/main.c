/*
 * Final Project.c
 *
 * Created: 5/12/2026 9:01:59 PM
 * Author : Omar Desoky
 */ 

#include <avr/io.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <stdio.h>

/*------------------ Macros---------------*/
#define setbit(port,bit) (port) |= (1<<(bit))
#define clearbit(port,bit) (port) &= ~(1<<(bit))
#define bitisset(reg,bit) ((reg)&(1<<(bit)))
#define bitisclear(reg,bit) !((reg)&(1<<(bit)))

#define PPR 12 //Obtained from Motor Data Sheet
#define SAMPL_MS 100 // RPM Sampling window

volatile int PWMduty =0 ; // -255 TO +255 , sign = direction
volatile int32_t encoderTicks = 0 ; // Controlled by encoder ISR
volatile float RPM = 0.0 ; // Updated every (SAMPLE_MS) by Timer1 ISR

void PWM_init()
{
	TCCR0 = (1<<WGM00) | (1<<WGM01) | (1<<COM01) |(1<<CS01); /* Change the prescaler to make the intution of changing the frequency*/
	setbit(DDRB,PB3) ;
}
void Timer1_init(void)
{
	TCCR1B = (1<<WGM12) | (1<<CS12); // CTC Mode with 256 prescaler
	OCR1A = 6249 ; // (Sample_ms) Compare Value
	TIMSK |= (1<<OCIE1A);
}

int main(void)
{   
	PWM_init();
	
	// H Bridge control pins Config.
	DDRB = 0XFF ; // The port that has pins for motor direction control
	
	clearbit(DDRD,PD2); //set PD2 as input
	setbit(PORTD,PD3); //Enables PD2 PULL UP
	clearbit(DDRD,PD3); //set PD3 as input
	setbit(PORTD,PD2); //Enables PD2 PULL UP
	
		
	/*--------Enable Interrupts-----*/
	MCUCR = 0x0A ; // Make INT0 & INT1 Falling Edge triggered
	setbit(GICR,INT0); // Enables INT0 External Interrupt (PD2 : Speed Up switch)
	setbit(GICR,INT1); // Enables INT1 External Interrupt (PD3 : Speed Down Switch)
		
	sei(); // Enables the global interrupt
	
    while (1) 
    {   /* Atomic Read of RPM */
	    cli();
	    float currentRPM = RPM ;
	    sei();
	    /* H Bridge Logic */
		/* H Bridge Logic */
		if (PWMduty>0)
		{
			setbit(PORTB,PB0);
			clearbit(PORTB,PB1); //Motor Clockwise
			OCR0 = PWMduty;
		}
		if (PWMduty<0)
		{
			setbit(PORTB,PB1);
			clearbit(PORTB,PB0); //Motor AntiClockwise
			OCR0 = abs(PWMduty);
		}
		if (PWMduty==0)
		{
			clearbit(PORTB,PB1);
			clearbit(PORTB,PB0); //Motor stops
			OCR0 = 0;
		}
		(void) currentRPM ; 
		
    }
}

ISR(INT0_vect)
{
	PWMduty = PWMduty + 25 ;
	if (PWMduty>255) PWMduty = 255 ;
	while(bitisclear(PIND,PD2));
}


ISR(INT1_vect)
{
	PWMduty = PWMduty - 25 ;
	if (PWMduty < -255) PWMduty = -255 ;
	while(bitisclear(PIND,PD3)) ;
}

ISR (INT2_vect)
{
	encoderTicks++;
}

ISR(TIMER1_COMPA_vect)
{
	RPM = ((float)encoderTicks /PPR)*(60000.0f/SAMPL_MS);
	encoderTicks = 0 ; //Reset for next sampling window
}
