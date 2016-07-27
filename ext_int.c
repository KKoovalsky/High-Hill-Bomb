#include "common.h"
#include "ext_int.h"
#include "key.h"
#include <stdlib.h>

volatile uint16_t to_boom_cnt;
volatile uint16_t half_to_boom_time;
volatile uint16_t quarter_to_boom_time;

volatile uint16_t to_unarm_cnt;

void Timer0_init() {
	// CTC mode set.
	TCCR0A |= (1<<WGM01);
	
	#define TIMER0_PRESCALER (1024.0)
	#define TIMER0_INT_FREQ (10.0)
	
	// Set ~100 ms interrupt
	OCR0A = F_CPU / TIMER0_PRESCALER / TIMER0_INT_FREQ - 0.5;
	
	TIMER0_INT_EN;
}

void Timer1_init() {
	// CTC mode set.
	TCCR1B |= (1<<WGM12);
	
	#define TIMER1_PRESCALER (8.0)
	#define TIMER1_INT_FREQ (4.0)
	#define TIMER1_INT_FREQ_INT ((uint8_t)TIMER1_INT_FREQ)
	
	// Start Timer and set prescaling.
	TCCR1B |= (1<<CS11);
	
	// Set ~250 ms interrupt
	OCR1A = F_CPU / TIMER1_PRESCALER / TIMER1_INT_FREQ  - 0.5;
}

void Timer2_init() {
	// Set CTC mode.
	TCCR2A |= (1<<WGM21);
	
	#define TIMER2_PRESCALER (8.0)
	#define TIMER2_INT_FREQ (1000.0)
	
	// Set ~1ms interrupt.
	OCR2A = F_CPU / TIMER2_PRESCALER / TIMER2_INT_FREQ - 1;
	
	TIMER2_INT_EN;
}

void ext_int_init() {
	
	// Falling edge of INT1 pin generates interrupt.
	EICRA |= (1<<ISC11);
	
	// Allow interrupt on INT1 pin.
	INT1_EN;
}


ISR(INT1_vect) {
	if(dev_state == BLOCKED) {
		dev_state = ROOT_KEY_ENTERING;
		delay_cnt_ms = 0;
	}
	
	if(dev_state == UNARMED) {
		dev_state = ARMING;
	}
	
	keys_pressed_tab_clr();
	
	// Enable Timer0 overflow interrupt.
	Timer0_start();
	
	// Disable interrupt.
	INT1_DIS;

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
		if(!to_unarm_cnt) { TIMER1_INT_DIS; dev_state = NOT_EXPLODED; eeprom_write_byte(&sw_off_while_armed, 0); return; }
		if(RIGHT_UNARM_PIN_STATE || LEFT_UNARM_PIN_STATE) {
			to_unarm_cnt = eeprom_read_word(&to_unarm_time);
			dev_state = ARMED;
			delay_cnt_ms = 0;
		}
	}
	
	if(dev_state == ARMED) {
		
		if(!(RIGHT_UNARM_PIN_STATE || LEFT_UNARM_PIN_STATE)) {
			sec_cnt = 0;
			dev_state = UNARMING;
			BUZZER_CLR;
			return;
		}
		
		// Static variable used for count seconds.
		if(sec_cnt == TIMER1_INT_FREQ_INT - 1) to_boom_cnt--;
		
		if(!to_boom_cnt) { TIMER1_INT_DIS; BOOOOOOM; eeprom_write_byte(&sw_off_while_armed, 0); INT1_EN; return; }
		
		// Turn on buzzer at 0.5 s interval with 0.25 ms duration (like always).	
		if(to_boom_cnt <= quarter_to_boom_time) { BUZZER_SW; return; }
		
		// Turn on buzzer at 1 s interval with 0.25 ms duration (like always).		
		if(to_boom_cnt <= half_to_boom_time) {
			BUZZER_CLR;
			if(!sec_cnt) BUZZER_SET;
			return;
		}
		
		// Turn on buzzer at 2 s interval with 0.25 ms duration (like always).	
		BUZZER_CLR;
		if((to_boom_cnt % 2) && (!sec_cnt)) BUZZER_SET;
	}
}

ISR(TIMER0_COMPA_vect) {
	
	// Statements used to help handling with long key pressing.
	#define LAST_STATE_IGNORE (0xFF)
	static uint8_t last_state_row = LAST_STATE_IGNORE;
	static uint8_t last_state_col = LAST_STATE_IGNORE;
	
	static uint16_t * ee_time_val_ptr;
	
	// Statements used to bring avr in sleep mode if keys wouldn't be pressed for TIMEOUT seconds.
	#define KEY_PRESS_IGNORED (TIMEOUT * TIMER0_INT_FREQ)
	static uint16_t key_not_pressed_cnt = 0;
	
	if(last_state_col != LAST_STATE_IGNORE) {
		DDRD |= (1 << (last_state_col + 5));
		
		// No operation to get into steady state (without it long keypad pressing will spoil the code entered).
		_NOP(); _NOP();
		
		if(!(PINB & (1 << last_state_row))) {
			// Key still pressed.
			DDRD &= ~((1<<PD5) | (1<<PD6) | (1<<PD7));
			return;
		}
		// Key not pressed. Continue checking.
		last_state_row = last_state_col = LAST_STATE_IGNORE;
	}
	
	DDRD &= ~((1<<PD5) | (1<<PD6) | (1<<PD7));
	for(uint8_t col = 0; col < 3; col++) {
		DDRD |= (1 << (col + 5));
		for (uint8_t row = 0; row < 4; row ++) {
			if(!(PINB & (1 << row))) {
				
				if(!keys_pressed_num)
					keys_pressed_tab_clr();
				
				key_not_pressed_cnt = 0;
				
				DDRD &= ~((1<<PD5) | (1<<PD6) | (1<<PD7));
				
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
				
				if(dev_state == ADMIN_MOD_AUTH) {
					switch(keys_pressed[0]) {
						case '1': {
							dev_state = ADMIN_MOD_CODE_CHANGE;
							break;
						}
						case '2': {
							dev_state = ADMIN_MOD_TIME_CHANGE;
							ee_time_val_ptr = &to_boom_time;
							break;
						}
						case '3': {
							dev_state = ADMIN_MOD_TIME_CHANGE;
							ee_time_val_ptr = &to_unarm_time;
							break;
						}
						default: break;
					}
					return;
				}
				
				if(dev_state == ADMIN_MOD_TIME_CHANGE) {
					if((!keys_pressed_num) && ((keys_pressed[0] <= 48) || keys_pressed[0] > 57)) return;
					if(keys_pressed[keys_pressed_num] == '*' || keys_pressed[keys_pressed_num] == '#') {
						keys_pressed[keys_pressed_num] = '\0';
						eeprom_write_word(ee_time_val_ptr, atoi((char*)keys_pressed));
					}
				}
					
				keys_pressed_num ++;
					
				if(keys_pressed_num >= KEY_NUM) {
								
					if(dev_state != ADMIN_MOD_TIME_CHANGE) {
						for(uint8_t i = 0; i < KEY_NUM; i++) {
							//	Change code if needed
							if(dev_state == ADMIN_MOD_CODE_CHANGE) { 
								eeprom_write_byte(&arm_code[i], keys_pressed[i]);
								continue;
							}
						
							//	Check if correct key entered.
							if(keys_pressed[i] != ((dev_state == ROOT_KEY_ENTERING || dev_state == ADMIN_MOD_UNAUTH) ? 
									pgm_read_byte(&key[i]) : eeprom_read_byte(&arm_code[i]))) {
								keys_pressed_num = 0;
								return;
							}
						}
					}
						
					keys_pressed_num = 0;
					last_state_row = last_state_col = LAST_STATE_IGNORE;
					Timer0_stop();
					
					switch(dev_state) {
						case ADMIN_MOD_UNAUTH:
							dev_state = ADMIN_MOD_AUTH;
							Timer0_start();
							break;
						case ADMIN_MOD_CODE_CHANGE:
							dev_state = UNARMED;
							INT1_EN;
							break;
						case ADMIN_MOD_TIME_CHANGE: {
							keys_pressed[KEY_NUM] = '\0';
							eeprom_write_word(ee_time_val_ptr, atoi((char*)keys_pressed));
							dev_state = UNARMED;
							INT1_EN;
							break;				
						}
						case ROOT_KEY_ENTERING:
							dev_state = UNARMED;
							eeprom_write_byte(&sw_off_while_armed, 0);
							INT1_EN;
							break;
						default: {
							dev_state = ARMED;
							
							to_unarm_cnt = eeprom_read_word(&to_unarm_time);
							arm_bar_dur = to_unarm_cnt / (double)LCD_COLS * 1000.0;
							
							to_boom_cnt = eeprom_read_word(&to_boom_time);
							half_to_boom_time = to_boom_cnt >> 1;
							quarter_to_boom_time = half_to_boom_time >> 1;
							eeprom_write_byte(&sw_off_while_armed, 1);
							TIMER1_INT_EN;
							break;
						}
					}
				}					
				return;
			}
		}
		DDRD &= ~((1<<PD5) | (1<<PD6) | (1<<PD7));
	}
		
	key_not_pressed_cnt ++;
	if(key_not_pressed_cnt == KEY_PRESS_IGNORED) {
		if(dev_state == ROOT_KEY_ENTERING) dev_state = BLOCKED;
		else dev_state = UNARMED;
		keys_pressed_num = 0;
		Timer0_stop();
		INT1_EN;
	}
}
