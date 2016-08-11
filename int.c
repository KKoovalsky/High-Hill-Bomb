#include "common.h"
#include "int.h"

#define NONE (0xFE)

volatile f_ptr_t int_exec;

void ext_int_init() {
	
	// Falling edge of INT1 pin generates interrupt.
	EICRA |= (1<<ISC11);
	
	// Allow interrupt on INT1 pin.
	INT1_EN;
}


ISR(INT1_vect) {
	delay_cnt_ms = 0;
	wait_flag = false;
	
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
	sec_cnt = (sec_cnt + 1) % TIMER1_INT_FREQ_INT;
	
	if(int_exec) int_exec();
}

uint8_t get_key() {
	// Statements used to help handling with long key pressing.
	#define LAST_STATE_IGNORE (0xFF)
	static uint8_t last_state_row = LAST_STATE_IGNORE;
	static uint8_t last_state_col = LAST_STATE_IGNORE;
		
	if(last_state_col != LAST_STATE_IGNORE) {
		DDRD |= (1 << (last_state_col));
		
		// No operation to get into steady state (without it long keypad pressing will spoil the code entered).
		_NOP(); _NOP();
		
		if(!(PINC & (1 << (last_state_row + 2)))) {
			// Key still pressed.
			DDRD &= ~((1<<PD0) | (1<<PD1) | (1<<PD2));
			return NONE;
		}
		// Key not pressed. Continue checking.
		last_state_row = last_state_col = LAST_STATE_IGNORE;
	}
	
	for(uint8_t col = 0; col < 3; col++) {
		DDRD &= ~((1<<PD0) | (1<<PD1) | (1<<PD2));
		DDRD |= (1 << col);
		for (uint8_t row = 0; row < 4; row ++) {
			if(!(PINC & (1 << (row + 2)))) {
				DDRD &= ~((1<<PD0) | (1<<PD1) | (1<<PD2));
			
				last_state_col = col;
				last_state_row = row;
				
				switch(row) {
					case 3:
						switch(col) {
							case 0: return '*';
							case 1: return '0';
							case 2: return '#';
							default: break;
						}
						break;
					default: return (row * 3 + col + 1) + 48;
				}		
			}
		}
	}
	DDRD &= ~((1<<PD0) | (1<<PD1) | (1<<PD2));
	return NONE;	
}

void inc_buffer() {
	buffer[keys_pressed_num] = key_pressed;
	
	if(!keys_pressed_num) main_exec = key_display_clear_bad_code;
	else main_exec = key_display;
	
	keys_pressed_num ++;	
}

ISR(TIMER0_COMPA_vect) {
	// Statements used to bring avr in sleep mode if keys wouldn't be pressed for TIMEOUT seconds.
	#define KEY_PRESS_IGNORED (TIMEOUT * TIMER0_INT_FREQ)
	static uint16_t key_not_pressed_cnt = 0;
	
	uint8_t temp = get_key();
	
	if(temp == NONE) {
		// After TIMEOUT seconds with no key pressed go back to state before INT interrupt.
		key_not_pressed_cnt ++;
		if(key_not_pressed_cnt == KEY_PRESS_IGNORED) {
			if(int_exec == get_arm_code) {
				wait_flag = true;
				main_exec = display_steady;
			}
			else wait_flag = false;
			keys_pressed_num = 0;
			Timer0_stop();
			INT1_EN;
		}
		return;
	}
	
	key_pressed = temp;
	
	key_not_pressed_cnt = 0;
									
	if(int_exec) int_exec();
}
