#include <avr/io.h>
#include <avr/iom8.h>
#include <avr/interrupt.h>

#define _SERVO_CNT	2

//typedef unsigned char uint8_t;
//#define uint8_t unsigned char


/*
Minimal angle is 0°. They need to send one impulse an 0.388 ms.
Maximal angle is 180°. Impulse must to be 2.140 ms.
It is means that timer must to be call every(2.140-0.388)/180=0,009733 ms.
or better to set for every 4°: 0,009733 * 4 = 0.0389 ms= 25707Hz

At every 20 ms can to call 20/0.0389=514

Prescaler PSC=F_CPU/25707 = 311 

*/

#define _T_CALLER	(F_CPU/256)

#define _RESET_TICK	(514-1)

volatile uint16_t counter = 0; //ticks counter. every _RESET_TICK must to be to 0 reseted. 

typedef struct {
	uint8_t received:1;
} opt_t;

typedef struct {
	uint8_t idx;		//pin number
	uint8_t angle; 		//from 0 <-> 180
	int8_t process; 	//how meny tacts to do (one decrement from angle)
} servo_t;

#define _SET_PROC(x)	x.process=x.angle

static servo_t servos[_SERVO_CNT];
static opt_t opt;
static 	char ch;


void uart_putc( char c )
{
  while( ( UCSRA & ( 1 << UDRE ) ) == 0  );
  UDR = c;
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
	//http://mainloop.ru/avr-atmega/avr-timer-counter.html
  	TCCR1B = (0<<CS12)|(0<<CS11)|(1<<CS10); // 
	//#define _T_OFFSET	(_T_CALLER/25707)
	#define _T_OFFSET  0x0;
  	TIMSK |= (1<<TOIE1); // 
  	TCNT1 = _T_OFFSET;        // offset of counter
	
	//*

	//INIT
	DDRB = 0xFF;
	DDRC = 0xFF;
	servos[0].idx = (1 << PB0);
	servos[0].angle = 9;
	servos[0].process = 9;

	servos[1].idx = (1 << PB1);
	servos[1].angle = 12;
	servos[1].process = 12;
	//*



  	sei();                // ?????????? ??? ?????? ?????????? ??????????



	opt.received = 0;

	while(1){

		if ( UCSRA &_BV(RXC)   ){
			ch = UDR;
			uart_putc(ch);
			opt.received = 1;
		}

		if (opt.received == 1){

			if (ch == '+')
				servos[0].angle += 10;
			if (ch == '-')
				servos[0].angle -= 10;
			uart_putc(ch);
			uart_putc(13);
			uart_putc(10);
			opt.received = 0;
		}
	
	}
}

static uint8_t port = 0;

ISR( TIMER1_OVF_vect )
{
	PORTC = 1 & counter;
	TCNT1 = _T_OFFSET;// offset for correctiong of ours frequence
	
	counter++;
	uint8_t i;//, port = 0;
	/*if (counter >= _RESET_TICK){

		//Reinnitialisation
		for(i=0; i<_SERVO_CNT; i++){
			_SET_PROC(servos[i]);
		}
		counter = 0;
		//*

	}
	else {
		port = 0;
		for(i=0; i<_SERVO_CNT; i++){
			if (servos[i].process > 0){

				servos[i].process--;
				port |= servos[i].idx;
			}
		}
	}*/

	// Variant 2:
	port = 0;
	for(i=0; i<_SERVO_CNT; i++){
		if (servos[i].process > -servos[i].angle){

			servos[i].process--;
			if (servos[i].process >= 0)
				port |= servos[i].idx;
		}else{

			servos[i].process = servos[i].angle;
			//port |= servos[i].idx;
		}
	}
	//


	PORTB = port;
	PORTC = 1 & counter;
}
