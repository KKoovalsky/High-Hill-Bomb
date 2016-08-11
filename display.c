#include "common.h"

void two_msg(const char * str11, const char * str12, const char * str21, const char * str22) {
	
	wait_flag = true;
	while(wait_flag) {
		lcd_cls();
		lcd_str_P(str11);
		lcd_locate(1, 0);
		lcd_str_P(str12);
			
		if(!wait_flag) return;
		delay_ms_x(2000);
		lcd_cls();
		if(!wait_flag) return;
		
		lcd_str_P(str21);
		lcd_locate(1, 0);
		lcd_str_P(str22);
		
		if(!wait_flag) return;
		delay_ms_x(2000);
	}
	
}

void keys_entering(const char * str) {
	
	// No reason to call this function if Timer0 is off.
	if(!CHECK_TIM0_RUN) return;	
	
	lcd_cls();
	lcd_str_P(str);
	lcd_locate(1, 0);
	
	wait_flag = true;
	while(wait_flag) {

		// When key entered update LCD content:
		if(main_exec && wait_flag) {
			main_exec();
			main_exec = NULL;
		}
	}
	
}

void key_display() {
	lcd_char(key_pressed);
}

void key_display_bad_code() {
	lcd_char(key_pressed);
	lcd_str_P(PSTR(" Zly kod"));
}

void key_display_clear_bad_code() {
	lcd_locate(1, 0);
	lcd_char(key_pressed);
	lcd_str_P(PSTR("               "));
	lcd_locate(1, 1);
}

void display_armed() {
	lcd_cls();
	lcd_str_P(PSTR("Uzbrojono!   "));
}

void display_unarming() {
	lcd_cls();
	lcd_str_P(PSTR("Rozbrajanie..."));
	lcd_locate(1, 0);
	while(int_exec == disarming) {
		lcd_char(0xFF);
		delay_ms_x(eeprom_read_word(&changeable_vars[TO_DISARM_TIME])/(double)LCD_COLS * 1000.0);
	}
	if(int_exec == arm_countdown) display_armed();
	else display_unarmed();
}

void display_unarmed() {
	lcd_cls();
	lcd_str_P(PSTR("Bomba"));
	lcd_locate(1, 0);
	lcd_str_P(PSTR("rozbrojona!"));
}

void display_exploded() {
	lcd_cls();
	BUZZER_SET;
	lcd_str_P(PSTR("BOOOOOOOOOOOOM!"));
}

void display_steady() {
	lcd_cls();
	lcd_str_P(PSTR("Rozbrojona..."));
}