#ifndef COMMON_H_
#define COMMON_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>

#include "LCD/lcd44780.h"
#include "ext_int.h"

#define KEY_NUM (4)

#define TIMER0_INT_EN (TIMSK0 |= (1<<OCIE0A))
#define TIMER0_INT_DIS (TIMSK0 &= ~(1<<OCIE0A))

#define TIMER1_INT_EN (TIMSK1 |= (1<<OCIE1A))
#define TIMER1_INT_DIS (TIMSK1 &= ~(1<<OCIE1A))

#define TIMER2_INT_EN (TIMSK2 |= (1<<OCIE2A))
#define TIMER2_INT_DIS (TIMSK2 &= ~(1<<OCIE2A))

#define INT0_EN  (EIMSK |= (1<<INT0))
#define INT0_DIS (EIMSK &= ~(1<<INT0))

#define BUZZER_PIN (1<<PB5)
#define BUZZER_SET (PORTB |= BUZZER_PIN)
#define BUZZER_CLR (PORTB &= ~BUZZER_PIN)
#define BUZZER_SW (PORTB ^= BUZZER_PIN)
#define BUZZER_CHECK_STATE (PINB & BUZZER_PIN)
#define BOOOOOOM (dev_state = EXPLODED)

#define LEFT_UNARM_PIN (1<<PB6)
#define LEFT_UNARM_PIN_STATE (PINB & LEFT_UNARM_PIN)

#define RIGHT_UNARM_PIN (1<<PB7)
#define RIGHT_UNARM_PIN_STATE (PINB & RIGHT_UNARM_PIN)

// Time after which LCD backlight switches off.
#define TIMEOUT (10)

typedef enum dev_state_e {
	BLOCKED, ROOT_KEY_ENTERING, ROOT_KEY_ENTERED, UNARMED, ARMING, ARMED, EXPLODED, UNARMING, NOT_EXPLODED
} dev_state_t;

extern volatile dev_state_t dev_state;

extern uint8_t arm_code [KEY_NUM] EEMEM;
extern uint16_t to_boom_time EEMEM;
extern uint16_t to_unarm_time EEMEM;

extern uint8_t sw_off_while_armed EEMEM;

extern volatile uint16_t delay_cnt_ms;

extern volatile uint8_t keys_pressed[];
extern volatile uint8_t keys_pressed_num;

extern volatile uint16_t arm_bar_dur;

inline void delay_ms_x (uint16_t ms_del) {
	TIMER2_INT_EN;
	delay_cnt_ms = ms_del;
	while(delay_cnt_ms);
	TIMER2_INT_EN;
}

inline void keys_pressed_tab_clr() {
	keys_pressed[0] = keys_pressed[1] = keys_pressed[2] = keys_pressed[3] = ' ';
	keys_pressed[4] = '\0';
}

#endif /* COMON_H_ */