#include "common.h"
#include "ext_int.h"
#include "key.h"

volatile uint16_t to_boom_cnt;
volatile uint16_t half_to_boom_time;
volatile uint16_t quarter_to_boom_time;

volatile uint16_t to_unarm_cnt;

void Timer0_init() {
	// CTC mode set.
	TCCR0A |= (1<<WGM01);
	
	// Start Timer and set prescaling.
	TCCR0B |= (1<<CS02) | (1<<CS00);
	
	#define TIMER0_PRESCALER (1024.0)
	#define TIMER0_INT_FREQ (10.0)
	
	// Set ~25 ms interrupt
	OCR0A = F_CPU / TIMER0_PRESCALER / TIMER0_INT_FREQ - 0.5;
}

void Timer1_init() {
	// CTC mode set.
	TCCR1A |= (1<<WGM12);
	
	#define TIMER1_PRESCALER (8.0)
	#define TIMER1_INT_FREQ (4.0)
	#define TIMER1_INT_FREQ_INT ((uint8_t)TIMER1_INT_FREQ)
	
	// Start Timer and set prescaling.
	TCCR1B |= (1<<CS11);
	
	// Set ~250 ms interrupt
	OCR1A = F_CPU / TIMER1_INT_FREQ / TIMER1_PRESCALER - 0.5;
}

void Timer2_init() {
	// Set CTC mode.
	TCCR2A |= (1<<WGM21);
	
	// Set prescaling and start Timer.
	TCCR2B |= (1<<CS21);
	
	#define TIMER2_PRESCALER (8.0)
	#define TIMER2_INT_FREQ (1000.0)
	
	// Set ~1ms interrupt.
	OCR2A = F_CPU / TIMER2_PRESCALER / TIMER2_INT_FREQ - 0.5;
	
}

void ext_int_init() {
	
	// Falling edge of INT0 pin generates interrupt.
	EICRA |= (1<<ISC01);
	
	// Allow interrupt on INT0 pin.
	INT0_EN;
}


ISR(INT0_vect) {
	if(dev_state == BLOCKED) {
		dev_state = ROOT_KEY_ENTERING;
		delay_cnt_ms = 0;
	}
	
	if(dev_state == UNARMED) {
		dev_state = ARMING;
	}
	
	keys_pressed_tab_clr();
	
	// Enable Timer0 overflow interrupt.
	TIMER0_INT_EN;
	
	// Disable interrupt.
	INT0_DIS;
}
ISR(TIMER2_COMPA_vect) {
	if(!delay_cnt_ms) return;
	delay_cnt_ms--;
}

ISR(TIMER1_COMPA_vect) {
	
	static uint8_t sec_cnt = 0;
	sec_cnt = (sec_cnt + 1) % TIMER1_INT_FREQ_INT;
	
	if(dev_state == UNARMING) {
		if(sec_cnt == TIMER1_INT_FREQ_INT - 1) to_unarm_cnt--;
		if(!to_unarm_cnt) { TIMER1_INT_DIS; dev_state = NOT_EXPLODED; return; }
		if(RIGHT_UNARM_PIN_STATE || LEFT_UNARM_PIN_STATE) {
			to_unarm_cnt = eeprom_read_word(&to_unarm_time);
			dev_state = ARMED;
			delay_cnt_ms = 0;
		}
	}
	
	if(dev_state == ARMED) {
		
		if(!(RIGHT_UNARM_PIN_STATE || LEFT_UNARM_PIN_STATE)) {
			dev_state = UNARMING;
			return;
		}
		
		// Static variable used for count seconds.
		if(sec_cnt == TIMER1_INT_FREQ_INT - 1) to_boom_cnt--;
		
		if(!to_boom_cnt) { TIMER1_INT_DIS; BOOOOOOM; eeprom_write_byte(&sw_off_while_armed, 0); INT0_EN; return; }
		
		// Turn on buzzer at 0.5 s interval with 0.25 ms duration (like always).	
		if(to_boom_cnt <= quarter_to_boom_time) { BUZZER_SW; return; }
		
		// Turn on buzzer at 1 s interval with 0.25 ms duration (like always).		
		if(to_boom_cnt <= half_to_boom_time) {
			if(BUZZER_CHECK_STATE) BUZZER_CLR;
			if(!sec_cnt) BUZZER_SET;
			return;
		}
		
		// Turn on buzzer at 2 s interval with 0.25 ms duration (like always).	
		if(BUZZER_CHECK_STATE) BUZZER_CLR;
		if(to_boom_cnt % 2) BUZZER_SET;
	}
}

ISR(TIMER0_COMPA_vect) {
	#define LAST_STATE_IGNORE (0xFF)
	static uint8_t last_state_row = LAST_STATE_IGNORE;
	static uint8_t last_state_col = LAST_STATE_IGNORE;
	
	
	if(last_state_col != LAST_STATE_IGNORE) {
		PORTD |= (1 << (last_state_col + 4));
		if(PINB & (1 << last_state_row)) {
			// Key still pressed.
			return;
		}
		// Key not pressed. Continue checking.
		last_state_row = last_state_col = LAST_STATE_IGNORE;
	}
	
	for(uint8_t col = 0; col < 3; col++) {
		//	Set PD5, PD6 or PD7.
		PORTD |= (1 << (col + 4));
		for (uint8_t row = 0; row < 4; row ++) {
			if(PINB & (1 << row)) {
				PORTD &= ~(1 << (col + 4));
				switch(row) {
					case 3:
						switch(col) {
							case 0: keys_pressed[keys_pressed_num] = '*'; break;
							case 1: keys_pressed[keys_pressed_num] = '0'; break;
							case 2: keys_pressed[keys_pressed_num] = '#'; break;
							default: break;
						}
						break;
					default: keys_pressed[keys_pressed_num] = (row * 3 + col + 1) + 48; break;
				}
					
				last_state_col = col;
				last_state_row = row;
					
				keys_pressed_num ++;
					
				if(keys_pressed_num == KEY_NUM) {
					for(uint8_t i = 0; i < KEY_NUM; i++) {
						//	Check if correct key entered.
						if(keys_pressed[i] != (dev_state == ROOT_KEY_ENTERING ? pgm_read_byte(&key[i]) : eeprom_read_byte(&arm_code[i]))) {
							keys_pressed_tab_clr();
							keys_pressed_num = 0;
							return;
						}
					}
						
					keys_pressed_num = 0;
					last_state_row = last_state_col = LAST_STATE_IGNORE;
					TIMER0_INT_DIS;
						
					if(dev_state == ROOT_KEY_ENTERING) {
						dev_state = UNARMED;
						eeprom_write_byte(&sw_off_while_armed, 0);
					} else {
						dev_state = ARMED;
							
						to_unarm_cnt = eeprom_read_word(&to_unarm_time);
						arm_bar_dur = to_unarm_cnt / (double)LCD_COLS * 1000.0;
							
						to_boom_cnt = eeprom_read_word(&to_boom_time);
						half_to_boom_time = to_boom_cnt << 1;
						quarter_to_boom_time = half_to_boom_time << 1;
						eeprom_write_byte(&sw_off_while_armed, 1);
						TIMER1_INT_EN;
					}
					
					return;
				}
			}
		} 
	}
		
	#define KEY_PRESS_IGNORED (TIMEOUT * TIMER0_INT_FREQ)
	static uint16_t key_not_pressed_cnt = 0;
	key_not_pressed_cnt ++;
	if(key_not_pressed_cnt == KEY_PRESS_IGNORED) {
		if(dev_state == ROOT_KEY_ENTERING) dev_state = BLOCKED;
		else dev_state = UNARMED;
		keys_pressed_tab_clr();
		keys_pressed_num = 0;
		TIMER0_INT_DIS;
		INT0_EN;
	}
}
