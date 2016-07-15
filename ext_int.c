#include "common.h"
#include "ext_int.h"
#include "key.h"

#define SET_TIM1_FREQ(X) (OCR1A = F_CPU / X / TIMER1_PRESCALER - 0.5)

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
	#define TIMER0_INT_FREQ (40.0)
	
	// Set ~25 ms interrupt
	OCR0A = F_CPU / TIMER0_PRESCALER / TIMER0_INT_FREQ - 0.5;
}

void Timer1_init() {
	// CTC mode set.
	TCCR1A |= (1<<WGM12);
	
	#define TIMER1_PRESCALER (8.0)
	
	// Start Timer and set prescaling.
	TCCR1B |= (1<<CS11);
	
	// Set ~1 ms interrupt
	SET_TIM1_FREQ(1000.0);
	
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
	
	// Disable interrupt.
	INT0_DIS;
}

ISR(TIMER1_COMPA_vect) {
	#define FREQ_FOR_GAME (4)
	
	static uint8_t sec_cnt = 0;
	sec_cnt = (sec_cnt + 1) % FREQ_FOR_GAME;
	
	if(dev_state == UNARMING) {
		if(sec_cnt == FREQ_FOR_GAME - 1) to_unarm_cnt--;
		if(!to_unarm_cnt) { TIMER1_INT_DIS; dev_state = NOT_EXPLODED; SET_TIM1_FREQ(1000.0); return; }
		if(RIGHT_UNARM_PIN_STATE || LEFT_UNARM_PIN_STATE) {
			to_unarm_cnt = eeprom_read_word(&to_unarm_time);
			dev_state == ARMED;
		}
	}
	
	if(dev_state == ARMED) {
		
		if(!(RIGHT_UNARM_PIN_STATE || LEFT_UNARM_PIN_STATE)) {
			dev_state = UNARMING;
			return;
		}
		
		// Static variable used for count seconds.
		if(sec_cnt == FREQ_FOR_GAME - 1) to_boom_cnt--;
		
		if(!to_boom_cnt) { TIMER1_INT_DIS; BOOOOOOM; SET_TIM1_FREQ(1000.0); return; }
			
		if(to_boom_cnt <= quarter_to_boom_time) { BUZZER_SW; return; }
			
		if(to_boom_cnt <= half_to_boom_time) {
			if(BUZZER_CHECK_STATE) BUZZER_CLR;
			if(!sec_cnt) BUZZER_SET;
			return;
		}
		
		if(BUZZER_CHECK_STATE) BUZZER_CLR;
		if(to_boom_cnt % 2) BUZZER_SET;
		return;
	}
	
	if(!delay_cnt_ms) return;
	delay_cnt_ms--;
}

ISR(TIMER0_COMPA_vect) {
	static uint8_t last_state_row = 0xFF;
	static uint8_t last_state_col = 0xFF;
	
	if(dev_state == BLOCKED || dev_state == UNARMED) {	// Probably don't need this condition.
		
		//TODO: Za³¹czenie podœwietlenia
	
		if(last_state_col != 0xFF) {
			PORTD |= (1 << (last_state_col + 4));
			if(PINB & (1 << last_state_row)) {
				// Key still pressed.
				return;
			}
			// Key not pressed. Continue checking.
			last_state_row = last_state_col = 0xFF;
		}
	
		for(uint8_t col = 0; col < 3; col++) {
			//	Set PD5, PD6 or PD7.
			PORTD |= (1 << (col + 4));
			for (uint8_t row = 0; row < 4; row ++) {
				if(PINB & (1 << row)) {
					PORTD &= ~(1 << (col + 4));
					switch(row) {
						case 0:
						case 1:
						case 2:
							keys_pressed[keys_pressed_num] = (row * 3 + col + 1) + 48;
							break;
						default: 
							switch(col) {
								case 0:
									keys_pressed[keys_pressed_num] = '*';
									break;
								case 1:
									keys_pressed[keys_pressed_num] = '0';
									break;
								case 2:
									keys_pressed[keys_pressed_num] = '#';
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
							if(keys_pressed[i] != (dev_state == BLOCKED ? pgm_read_byte(&key[i]) : eeprom_read_byte(&arm_code[i]))) {
								keys_pressed_tab_clr();
								keys_pressed_num = 0;
								return;
							}
						}
						
						keys_pressed_num = 0;
						last_state_row = last_state_col = 0xFF;
						TIMER0_INT_DIS;
						
						if(dev_state == BLOCKED) {
							dev_state = UNARMED;
						} else {
							dev_state = ARMED;
							to_unarm_cnt = eeprom_read_word(&to_unarm_time);
							to_boom_cnt = eeprom_read_word(&to_boom_time);
							half_to_boom_time = to_boom_cnt << 1;
							quarter_to_boom_time = half_to_boom_time << 1;
							eeprom_write_byte(&sw_off_while_armed, 1);
							SET_TIM1_FREQ(4.0);
							TIMER1_INT_EN;
						}
					
						return;
					}
				}	
			} 
		}
	}
}
