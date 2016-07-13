#include "common.h"
#include "ext_int.h"
#include "key.h"

void Timer0_init() {
	// CTC mode set.
	TCCR0A |= (1<<WGM01);
	
	// Start Timer and set prescaling.
	TCCR0B |= (1<<CS02) | (1<<CS00));
	
	#define TIMER0_PRESCALER (1024)
	#define TIMER0_INT_FREQ (40)
	
	// Set ~25 ms interrupt
	OCR0A = F_CPU / TIMER0_PRESCALER / TIMER0_INT_FREQ;
}

void start_timeout() {
	
}

void ext_int_init() {
	
	// Falling edge of INT0 pin generates interrupt.
	EICRA |= (1<<ISC01);
	
	// Allow interrupt on INT0 pin.
	EIMSK |= (1<<INT0);
}


ISR(INT0_vect) {
	if(!root_code_entered) {
		get_root_key();	
	} else {
		get_arm_key();
	}
	
	start_timeout();
	
	// Disable interrupt.
	EIMSK &= ~(1<<INT0);
}

ISR(TIMER0_COMPA_vect) {
	static uint8_t last_state_row = 0xFF;
	static uint8_t last_state_col = 0xFF;
	
	static uint8_t keys_pressed_num;
	static uint8_t keys_pressed[KEY_NUM];
	
	if(last_state_col != 0xFF) {
		PORTD |= (1 << (last_state_col + 4));
		if(PINB & (1 << last_state_row)) {
			// Key still pressed.
			return;
		}
		// Key not pressed. Continue checking.
		last_state_row = last_state_col = 0xFF;
	}
	
	if(root_key_check) {
		for(uint8_t col = 0; col < 3; col++) {
			//	Set PD5, PD6 or PD7.
			PORTD |= (1 << (col + 4));
			for (uint8_t row = 0; row < 4; row ++) {
				if(PINB & (1 << row)) {
					switch(row) {
						case 0:
						case 1:
						case 2:
							keys_pressed[keys_pressed_num] = LCD_buffer[LCD_buf_ind] = (row * 3 + col + 1) + 48;
							break;
						default: 
							switch(col) {
								case 0:
									keys_pressed[keys_pressed_num] = LCD_buffer[LCD_buf_ind] = '*';
									break;
								case 1:
									keys_pressed[keys_pressed_num] = LCD_buffer[LCD_buf_ind] = '0';
									break;
								case 2:
									keys_pressed[keys_pressed_num] = LCD_buffer[LCD_buf_ind] = '#';
									break;
								default:
									break;
							}
							break;
					}
					
					last_state_col = col;
					last_state_row = row;
					
					keys_pressed_num ++;
					
					if(keys_pressed_num == KEY_NUM) {
						for(uint8_t i = 0; i < KEY_NUM; i++) {
							if(keys_pressed[i] != pgm_read_byte(&key[i])) {
								LCD_buff_add_P(PSTR("Zly klucz"));
								keys_pressed_num = 0;
								return;
							}
						}
						
						keys_pressed_num = 0;
						last_state_row = last_state_col = 0xFF;
						
						TIMER0_INT_DIS;
						
						root_key_check = false;
						root_code_entered = true;
						
						LCD_buff_add_P(PSTR("Odblokowano"));
					} 
					
					return;
				}
			}	
		} 
		
	}
}