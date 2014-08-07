// ATtiny25 ADC over I2C example project
// using the USITWISLAVE library by Erik Slagter
// 
// Copyright 2014 GPLv2 Mihaly Vadai vadaim@gmail.com

#define F_CPU 8000000L //fuses have to be set: -U lfuse:w:0xe2:m -U hfuse:w:0xdd:m -U efuse:w:0xff:m
#define DEVICE_ID 0x03
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "usitwislave_devices.h"
#include "usitwislave.h"
#include "usitwislave.c"

// pin setup is for TWI / I2C
uint8_t low;
uint8_t high;
uint8_t state = 0;

// _delay_ms can not take a parameter, only a constant
void blink300(int no){
	int i = 0;
	while( i < no ){
		PORTB |= (1 << PB1);
		_delay_ms(300);
		PORTB &= ~(1 << PB1);
		_delay_ms(300);
		i++;
	}
}

void blink20(int no){
	int i = 0;
	while( i < no ){
		PORTB |= (1 << PB1);
		_delay_ms(20);
		PORTB &= ~(1 << PB1);
		_delay_ms(20);
		i++;
	}
}

void ADC_input(uint8_t pin){
	ADMUX &= ~(1 << MUX3);
	ADMUX &= ~(1 << MUX2);
	ADMUX |= (1 << MUX1);
	if(pin == 3) ADMUX &= ~(1 << MUX0); // adc set to pb4
	if(pin == 2) ADMUX |= (1 << MUX0); // adc set to pb3
	_delay_us(120); // wait for 15 ADC cycles at 125kHz
}
void ADC_enable(uint8_t def_pin){
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (0 << ADPS0); // clock divided by 64 125 kHz
	ADCSRA |= (1 << ADATE); // setting auto trigger
	ADCSRB |= (0 << ADTS2) | (0 << ADTS1) | (0 << ADTS0); // free running mode - default
	ADMUX |= (0 << REFS0); // ref voltage is Vcc - default
	ADC_input(def_pin); // setting default pin
	ADMUX |= (0 << ADLAR); // using 10 bit resolution - default
	ADCSRA |= (1 << ADEN); // enable ADC

//	ADCSRA |= (1 << ADIE);  // Enable ADC Interrupt 
	ADCSRA |= (1 << ADSC);  // start sampling
}

void get_voltage(uint8_t pin){
	ADC_input(pin);
	low = ADCL;
	high = ADCH;
	state = 2;
}

static void twi_callback(uint8_t input_buffer_length, const uint8_t *input_buffer,
				uint8_t *output_buffer_length, uint8_t *output_buffer)
{
	if(input_buffer_length > 0){ //we're getting an instruction from master
		if(*input_buffer == 1){
			get_voltage(2);
		}
		if(*input_buffer == 2){
			get_voltage(3);
		}
		blink20(3); //invalid command
	}else{ // we're getting a write request from master
		*output_buffer_length = 8;
		switch(state)
		{
			case(2):{ // sending data high byte
				*output_buffer = high;
				state = 1;
				break;
			}
			case(1):{ // sending data low byte
				*output_buffer = low;
				state = 0;
				blink20(1);
				break;
			}
			case(0):{ // sending device id
				*output_buffer = DEVICE_ID;
				break;
			}
		}
	}
}

int main(void) {
	DDRB |= (1 << PB1); // setup pin1 for output
	PORTB |= (1 << PB1); // set pin1 value to 1
	ADC_enable(2);
	blink300(DEVICE_ID); // just to let you know my device ID
	usi_twi_slave(DEVICE_ID, 1, *twi_callback, 0);
	return 0;
}
