#ifndef COMMON_H_
#define COMMON_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <avr/cpufunc.h>
#include <stdbool.h>
#include <stdlib.h>

#include "LCD/lcd44780.h"
#include "int.h"
#include "display.h"
#include "int_executors.h"

#define KEY_NUM (4)

#define TIMER0_INT_EN (TIMSK0 |= (1<<OCIE0A))

#define TIMER1_INT_EN (TIMSK1 |= (1<<OCIE1A))
#define TIMER1_INT_DIS (TIMSK1 &= ~(1<<OCIE1A))

#define TIMER2_INT_EN (TIMSK2 |= (1<<OCIE2A))
#define TIMER2_INT_DIS (TIMSK2 &= ~(1<<OCIE2A))

#define CHECK_TIM0_RUN (TCCR0B & ((1<<CS02) | (1<<CS00)))

#define INT1_EN  if(EIFR & (1<<INTF1)) {EIFR |= (1<<INTF1);} EIMSK |= (1<<INT1)
#define INT1_DIS (EIMSK &= ~(1<<INT1))

#define BUZZER_PIN (1<<PD4)
#define BUZZER_SET (PORTD |= BUZZER_PIN)
#define BUZZER_CLR (PORTD &= ~BUZZER_PIN)
#define BUZZER_SW (PORTD ^= BUZZER_PIN)

#define LEFT_DISARM_PIN (1<<PC1)
#define LEFT_DISARM_PIN_STATE (PINC & LEFT_DISARM_PIN)

#define RIGHT_DISARM_PIN (1<<PD3)
#define RIGHT_DISARM_PIN_STATE (PIND & RIGHT_DISARM_PIN)

// Time after which LCD backlight switches off.
#define TIMEOUT (20)

#define LED_OFF (PORTC |= (1<<PC0))
#define LED_ON (PORTC &= ~(1<<PC0))

#define TRUE (1)
#define FALSE (0)

#define ARM_CODE (0)
#define TO_BOOM_TIME (1)
#define TO_DISARM_TIME (2)

typedef void( * f_ptr_t )( void );

extern uint16_t changeable_vars[3] EEMEM;

extern uint8_t sw_off_while_armed EEMEM;

extern volatile uint16_t delay_cnt_ms;

extern volatile uint8_t key_pressed;
extern volatile uint8_t keys_pressed_num;

extern volatile bool wait_flag;

extern volatile f_ptr_t main_exec;

void Timer0_start();
void Timer0_stop();

void Timer2_start();
void Timer2_stop();

void delay_ms_x (uint16_t ms_del);

inline bool check_admin_mode_call() {
	DDRD |=	(1<<PD1);
	
	//	Wait for steady pin state.
	_NOP(); _NOP();
	if(!(PINC & (1 << PC3))) {
		while(!(PINC & (1 << PC3)));
		DDRD &=	~(1<<PD1);
		delay_ms_x(100);
		return true;
	}
	
	DDRD &=	~(1<<PD1);
	return false;
	
}

void set_fptr(volatile f_ptr_t * dest, f_ptr_t src);
bool check_fptr(volatile f_ptr_t fptr);
bool comp_fptr(volatile f_ptr_t left, f_ptr_t right);

#endif /* COMON_H_ */