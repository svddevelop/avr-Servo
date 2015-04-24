/*  
	The MIT License (MIT)

	Copyright (c) 2015 svddevelop

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/
#include <avr/io.h>
#include <avr/iom8.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#define _SERVO_CNT	8

/*
	Library tested with ATMEGA8 with internal oscilation to 8MHz.
	For each other frequence need to reclac and to rewrite the
	values in the eememSet.

	By eememSet items have values for OCR1A-register for 16-bit Timer1.
	An 8MHz one tick in OCR1A costs 1 microsecond.

   In the EEPROM saved information for each connected motor (from datasheet):
       timeimpulse for minimal ticks for minimal angle ;
       timeimpulse for maximal tichs for angle.   
*/

typedef struct {
	uint8_t received:1;
} opt_t;

//namespace servo_const {
  typedef struct {
	uint16_t cntmin;
	uint16_t cntmax;
  } servo_set_t;
  
//}



typedef struct {
	uint8_t idx;		//pin number maske
	uint16_t counter;	//timer counter
	int8_t	angle;		//values for recalculation of counter. from -90 to +90 only.
} servo_t;


static servo_set_t EEMEM eememSet[_SERVO_CNT]={
		 {780, 2200}
		,{780, 2200}
		,{780, 2200}
		,{780, 2200}
		,{780, 2200}
		,{780, 2200}
		,{780, 2200}
		,{780, 2200}
		};
 //for Patrics entity (HS-300BB) is 0.89mS -||- 2.2mS

static servo_set_t tmpSet;
static servo_t servos[_SERVO_CNT];
static opt_t opt;
static 	char ch;
static uint8_t idx = 0;
static uint8_t port = 0;


#define _START_TCI	TIMSK |= (1 << OCIE1A)
#define _STOP_TCI	TIMSK &= ~(1 << OCIE1A)

void uart_putc( char c )
{
  while( ( UCSRA & ( 1 << UDRE ) ) == 0  );
  UDR = c;
}

static uint16_t angle2counter(uint8_t idx, int8_t angle){
  
	uint16_t addr = &eememSet;
	addr += sizeof(servo_set_t)*idx;
	eeprom_read_block ((void *)&tmpSet, addr, sizeof(servo_set_t));
	return (tmpSet.cntmin + tmpSet.cntmax)/2 + angle*((tmpSet.cntmax - tmpSet.cntmin)/180);
}


void timerStart(){

	_STOP_TCI;
	port &= ~(servos[idx++].idx);
	if (idx >= _SERVO_CNT) idx = 0;
	port |= (servos[idx].idx);

	OCR1A = servos[idx].counter;
	
	TCNT1 = 0;	
	PORTB = port;
	_START_TCI;
}

int main(void) {


	//Init UART
	// USART initialization
	// Communication Parameters: 8 Data, 1 Stop, No Parity
	// USART Receiver: On
	// USART Transmitter: On
	// USART Mode: Asynchronous
	// USART Baud Rate: 9600
	UCSRA=0x00;
    UCSRC = ( 1 << URSEL ) | ( 1 << UCSZ1 ) | ( 1 << UCSZ0 );
  	UCSRB = ( 1 << TXEN ) | ( 1 <<RXEN );
	UBRRH=0x00;
	UBRRL=0x33;
	//*

	uart_putc('>');
	//Init timer
	//OCR1A = 0x0028;  //0x0028 - 40uS

	OCR1A = 0x7d0;  //0x030C - 780uS

	//Achtung! Diese reinfolge ist wichtigste!
	//OCR1AL = 0x20;
	//OCR1AH = 0x4e;

	//OCR1AL = 0x0A;
	//OCR1AH = 0x00;
	//***



	TCCR1A = 0;
	TCNT1 = 0;
	TCCR1B |= (1 << WGM12);
	TCCR1B |= (1<<CS11);

	idx = 0;



	//----------------------

	//INIT
	DDRB = 0xFF;
	DDRC = 0xFF;
	for(idx =0; idx < _SERVO_CNT; idx++){
		int16_t addr = &eememSet;
		addr += sizeof(servo_set_t)*idx;
		eeprom_read_block ((void *)&tmpSet, addr, sizeof(servo_set_t));
		servos[idx].idx = (1 << (idx));
		servos[idx].counter = tmpSet.cntmin;
		servos[idx].angle = -90;
	}
	idx = 0;

	timerStart();
  	sei();     
	opt.received = 0;
	uint8_t bidx;
	int8_t angle;

	while(1){

		if ( UCSRA &_BV(RXC)   ){
			ch = UDR;
			uart_putc(ch);
			opt.received = 1;
		}

		if (opt.received == 1){

			if ((ch >= '1') & (ch <= '8')){
				bidx = ch - '1';
				//uart_putc(ch);
			}
			if ((ch == '+')|(ch == '-')){

				if (ch == '+'){

					angle = servos[bidx].angle;
					angle += 15;
					if (angle > +90) angle = +90;
				}
				if (ch == '-'){

					angle = servos[bidx].angle;
					angle -= 15;
					if (angle < -90) angle = -90;
				}
				servos[bidx].counter = angle2counter(bidx, angle);
				servos[bidx].angle = angle;
				//uart_putc(ch);
				uart_putc(13);
				uart_putc(10);
				uart_putc('>');
			}
			opt.received = 0;
		}
	
	}
}



ISR(TIMER1_COMPA_vect)
{
	timerStart();
	PORTC = 1 & idx;
	PORTB = port;
}


