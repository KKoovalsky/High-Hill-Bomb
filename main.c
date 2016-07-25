#include "common.h"

uint8_t arm_code [KEY_NUM] EEMEM;
uint16_t to_boom_time EEMEM;
uint16_t to_unarm_time EEMEM;

// If bomb were switched off while armed.
uint8_t sw_off_while_armed EEMEM;

volatile dev_state_t dev_state;

volatile uint16_t delay_cnt_ms;

volatile uint8_t keys_pressed[KEY_NUM + 1];
volatile uint8_t keys_pressed_num;

volatile uint16_t arm_bar_dur;

const char def_arm_key [] PROGMEM = "1993";

inline void init_eeprom_if_default() {
	if(eeprom_read_word(&to_boom_time) == 0xFFFF) eeprom_write_word(&to_boom_time, 40);
	
	if(eeprom_read_word(&to_unarm_time) == 0xFFFF) eeprom_write_word(&to_unarm_time, 5);
	
	for(uint8_t i = 0; i < KEY_NUM ; i ++) {
		if(eeprom_read_byte(&arm_code[i]) == 0xFF) eeprom_write_byte(&arm_code[i], pgm_read_byte(&def_arm_key[i]));
	}
	
	if(eeprom_read_byte(&sw_off_while_armed) == 0xFF) eeprom_write_byte(&sw_off_while_armed, 0);
}


int main(void) {	
	
	// Buzzer pin.
	DDRB |= (1<<PB5);
	
	// Debug LED pin.
	DDRC |= (1<<PC0) | (1<<PC1);

	// Unarming pins.
	DDRD &= ~((1<<PD3) | (1<<PD4));
	
	// Keyboard row input pins.
	DDRB &= ~((1<<PB0) | (1<<PB1) | (1<<PB2) | (1<<PB3));
	// Pullups.
	PORTB |= (1<<PB0) | (1<<PB1) | (1<<PB2) | (1<<PB3);
	// Keyboard column outputs - set 0 state (HIGH-Z, when DDR bits set to zero)
	PORTD &= ~((1<<PD5) | (1<<PD6) | (1<<PD7));
	
	init_eeprom_if_default();
	
	keys_pressed_tab_clr();
	
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	
	Timer0_init();
	Timer1_init();
	Timer2_init();
	ext_int_init();
	lcd_init();
	
	sei();

	
	// When device were switched off while armed.
	if(eeprom_read_byte(&sw_off_while_armed)) {
		
		
		//TODO: Za³¹czenie podœwietlenia
		
		dev_state = BLOCKED;
		
		while(dev_state == BLOCKED) {
			lcd_str_P(PSTR("Wyl. w trakcie"));
			lcd_locate(1, 0);
			lcd_str_P(PSTR("odliczania"));
		
			if(dev_state != BLOCKED) break;
			delay_ms_x(2000);
			lcd_cls();
			if(dev_state != BLOCKED) break;
			
			lcd_str_P(PSTR("Dajcie Oskarowi"));
			lcd_locate(1, 0);
			lcd_str_P(PSTR("luj odblokuje"));
			
			if(dev_state != BLOCKED) break;
			delay_ms_x(2000);
			lcd_cls();
			// Will probably need lcd_locate(0, 0)
		}
		
		lcd_cls();
		//lcd_locate(0, 0);
		uint8_t local_keys_pressed_num = 0;
		while(dev_state == ROOT_KEY_ENTERING) {
			
			// When key entered update LCD content:
			if(local_keys_pressed_num != keys_pressed_num) {
				LED1_TOG;
				lcd_cls();
				lcd_str((char*)keys_pressed);
				
				// When wrong code entered.
				if(local_keys_pressed_num > keys_pressed_num) {
					lcd_locate(1, 0);
					lcd_str_P(PSTR("Zly kod"));
				}
				local_keys_pressed_num = keys_pressed_num;
			}
			// TODO: Turn off backlight.
		}
		
		lcd_cls();
		lcd_str_P(PSTR("Odblokowano"));
		delay_ms_x(5000);
		lcd_cls();
	}
	
	dev_state = UNARMED;
	
	lcd_str_P(PSTR("WITAJ"));
    
    while (1) {
		
		if(dev_state == EXPLODED) {
			BUZZER_SET;
			lcd_cls();
			lcd_str_P(PSTR("Terrorysci"));
			lcd_locate(1, 0);
			lcd_str_P(PSTR("wygrali!"));
			delay_ms_x(5000);
			BUZZER_CLR;
			dev_state = UNARMED;
			INT1_EN;
		}
		
		if(dev_state == UNARMING) {
			// TODO: Turn on backlight.
			lcd_cls();
			lcd_str_P(PSTR("Rozbrajanie..."));
			for(uint8_t i = 0 ; i < LCD_COLS && dev_state == UNARMING; i ++) {
				lcd_locate(1, i);
				lcd_char('-');
				delay_ms_x(arm_bar_dur);
			}
			lcd_cls();
			// TODO: Turn off backlight.
		}
		
		if(dev_state == NOT_EXPLODED) {
			// TODO: Turn on backlight.
			lcd_cls();
			lcd_str_P(PSTR("Bomba rozbrojona"));
			delay_ms_x(2000);
			// TODO: Turn off backlight.
			dev_state = UNARMED;
			INT1_EN;
		}
		
		uint8_t local_keys_pressed_num = 0;
		while(dev_state == ARMING) {
			
			// When key entered update LCD content:
			if(local_keys_pressed_num != keys_pressed_num) {

				lcd_cls();
				lcd_str((char*)keys_pressed);
				
				// When wrong code entered.
				if(local_keys_pressed_num > keys_pressed_num) {
					lcd_locate(1, 0);
					lcd_str_P(PSTR("Zly kod"));
				}
				local_keys_pressed_num = keys_pressed_num;
			}
			// TODO: Turn off backlight.
		}
		
		if(dev_state == ARMED) {
			lcd_cls();
			lcd_str((char *)keys_pressed);
			lcd_locate(1, 0);
			lcd_str_P(PSTR("UZBROJONO!"));
			delay_ms_x(2000);
			while(dev_state == ARMED);
		}
		
		//Sleep and switch off backlight.
		if(dev_state == UNARMED) sleep_mode();	
    }
}

