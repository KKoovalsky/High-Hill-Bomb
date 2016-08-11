#include "common.h"
#include "admin_code.h"

volatile uint16_t to_boom_cnt;
volatile uint16_t half_to_boom_time;
volatile uint16_t to_disarm_cnt;

volatile uint16_t * ee_var_ptr;

volatile uint8_t sec_cnt = 0;

volatile char buffer[KEY_NUM + 1];


void disarming() {
	if(sec_cnt == TIMER1_INT_FREQ_INT - 1) to_disarm_cnt--;
	if(!to_disarm_cnt) { TIMER1_INT_DIS; wait_flag = true; int_exec = NULL; eeprom_write_byte(&sw_off_while_armed, 0); return; }
	if(RIGHT_DISARM_PIN_STATE || LEFT_DISARM_PIN_STATE) {
		int_exec = arm_countdown;
		main_exec = display_armed;
		delay_cnt_ms = 0;
		LED_OFF;
	}
}

void arm_countdown() {
	
	if(!(RIGHT_DISARM_PIN_STATE || LEFT_DISARM_PIN_STATE)) {
		sec_cnt = 0;
		int_exec = disarming;
		main_exec = display_unarming;
		
		// Set disarming time counter.
		to_disarm_cnt = eeprom_read_word(&changeable_vars[TO_DISARM_TIME]);
						
		LED_ON;
		BUZZER_CLR;
		return;
	}
	
	// Static variable used for count seconds.
	if(sec_cnt == TIMER1_INT_FREQ_INT - 1) to_boom_cnt--;
	
	if(!to_boom_cnt) { TIMER1_INT_DIS; wait_flag = true; main_exec = display_exploded; eeprom_write_byte(&sw_off_while_armed, 0); return; }
	
	// Turn on buzzer at 0.5 s interval with 0.25 ms duration (like always).
	if(to_boom_cnt <= half_to_boom_time >> 1) { BUZZER_SW; return; }
	
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



void get_root_code() {
	inc_buffer();
	if(keys_pressed_num == KEY_NUM) {
		buffer[keys_pressed_num] = '\0';
		if(pgm_read_word(&admin_code) == (uint16_t)atoi((char*)buffer)) {
			wait_flag = false;
			eeprom_write_byte(&sw_off_while_armed, 0);
		} else main_exec = key_display_bad_code;
		keys_pressed_num = 0;
	}
}

void what_to_change() {
	if(key_pressed >= '1' && key_pressed <= '3') {
		wait_flag = false;
		int_exec = change_ee_var;
		ee_var_ptr = &changeable_vars[key_pressed - 48 - 1];
	}
}

void update_ee() {
	buffer[keys_pressed_num] = '\0';
	eeprom_write_word((uint16_t *)ee_var_ptr, atoi((char*)buffer));
	wait_flag = false;
	keys_pressed_num = 0;
}

void change_ee_var() {
	if((!keys_pressed_num) && (key_pressed == '0' || key_pressed == '*' || key_pressed == '#')) return;
	if(key_pressed == '*' || key_pressed == '#') {
		if(ee_var_ptr != &changeable_vars[ARM_CODE]) {
			update_ee();
			main_exec = NULL;
			return;
		} else return;
	}
	
	inc_buffer();
	
	if(keys_pressed_num == KEY_NUM) {
		update_ee();
		main_exec = key_display;
	}
}

void get_arm_code() {
	inc_buffer();
	
	if(keys_pressed_num == KEY_NUM) {
		buffer[KEY_NUM] = '\0';
		if(eeprom_read_word(&changeable_vars[ARM_CODE]) == (uint16_t)atoi((char *)buffer)) {
			Timer0_stop();
							
			// Set countdown counter for "explosion".
			to_boom_cnt = eeprom_read_word(&changeable_vars[TO_BOOM_TIME]);
			
			// Set levels for enlarging frequency of buzzer's sound.
			half_to_boom_time = to_boom_cnt >> 1;
			
			// Set flag to avoid disarming of bomb by switching off the device.
			eeprom_write_byte(&sw_off_while_armed, TRUE);
			
			TIMER1_INT_EN;
			
			main_exec = display_armed;
			int_exec = arm_countdown;
			
			LED_OFF;
			
		} else {
			main_exec = key_display_bad_code;
		}
		
		keys_pressed_num = 0;
	}
}
