#include "common.h"
#include <util/atomic.h>

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

static inline void init_eeprom_if_default() {
	if(eeprom_read_word(&to_boom_time) == 0xFFFF) eeprom_write_word(&to_boom_time, 40);
	
	if(eeprom_read_word(&to_unarm_time) == 0xFFFF) eeprom_write_word(&to_unarm_time, 5);
	
	for(uint8_t i = 0; i < KEY_NUM ; i ++) {
		if(eeprom_read_byte(&arm_code[i]) == 0xFF) eeprom_write_byte(&arm_code[i], pgm_read_byte(&def_arm_key[i]));
	}
	
	if(eeprom_read_byte(&sw_off_while_armed) == 0xFF) eeprom_write_byte(&sw_off_while_armed, 0);
}

void delay_ms_x (uint16_t ms_del) {
	uint8_t exit = FALSE;
	delay_cnt_ms = ms_del;
	
	Timer2_start();
	
	while(1) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			if(!delay_cnt_ms) exit = TRUE;
		}
		if(exit) break;
	}	
	
	Timer2_stop();
}

void code_entering(dev_state_t context) {
	
	uint8_t local_keys_pressed_num = 0;
	lcd_cls();
	lcd_str_P(PSTR("Wprowadz kod:"));
		
	while(dev_state == context) {

		// When key entered update LCD content:
		if(local_keys_pressed_num != keys_pressed_num) {
				
			lcd_locate(1, 0);
			lcd_str((char*)keys_pressed);
				
			// Clear message about wrong code.
			if(keys_pressed_num == 1) {
				lcd_locate(1, 5);
				lcd_str_P(PSTR("       "));
			}
				
			// When wrong code entered.
			if(local_keys_pressed_num > keys_pressed_num) {
				lcd_locate(1, 5);
				lcd_str_P(PSTR("Zly kod"));
			}
			local_keys_pressed_num = keys_pressed_num;
		}
	}
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

	// Check if the admin mode is called ('5' key pressed at startup).
	DDRD |=	(1<<PD6);
	_NOP(); _NOP();
	if(!(PINB & (1 << PB1))) {
		DDRD &=	~(1<<PD6);
		
		LED_ON;
	
		dev_state = ADMIN_MOD_UNAUTH;
		code_entering(ADMIN_MOD_UNAUTH);
		
		// Ask user what does he want to change.
		while(dev_state == ADMIN_MOD_AUTH) {
			lcd_str_P(PSTR("Co zmienic?"));
			lcd_locate(1, 0);
			lcd_str_P(PSTR("1.Kod uzbrojenia"));
		
			if(dev_state != ADMIN_MOD_AUTH) break;
			delay_ms_x(2000);
			lcd_cls();
			if(dev_state != ADMIN_MOD_AUTH) break;
			
			lcd_str_P(PSTR("2.Czas do bum"));
			lcd_locate(1, 0);
			lcd_str_P(PSTR("3.Czas anty-bum"));
			
			if(dev_state != ADMIN_MOD_AUTH) break;
			delay_ms_x(2000);
			lcd_cls();
		}
		
		lcd_cls();
		
		if(dev_state == ADMIN_MOD_CODE_CHANGE) {
			code_entering(ADMIN_MOD_CODE_CHANGE);
			
			// After changes inform about the changes.
			lcd_cls();
			lcd_str_P(PSTR("Zmieniono kod na"));
			lcd_locate(1, 0);
			lcd_str((char*)keys_pressed);
			delay_ms_x(4000);
			lcd_cls();
		}
		
		if(dev_state == ADMIN_MOD_TIME_CHANGE) {
			// Maximum 9999 seconds can be set - * or # pressing accepts the value
			lcd_cls();
			lcd_str_P(PSTR("Zatwierdz *, #:"));
			uint8_t local_keys_pressed_num = 0;
			while(dev_state == ADMIN_MOD_TIME_CHANGE) {
				if(local_keys_pressed_num != keys_pressed_num) {
					lcd_locate(1, 0);
					lcd_str((char*)keys_pressed);
					local_keys_pressed_num = keys_pressed_num;
				}
			}
			// After changes inform about the changes.
			lcd_cls();
			lcd_str_P(PSTR("Zmieniono czas"));
			lcd_locate(1, 0);
			lcd_str_P(PSTR("na "));
			lcd_str((char*)keys_pressed);
			lcd_str_P(PSTR(" sekund"));
			delay_ms_x(4000);
			lcd_cls();
		}
		LED_OFF;
	}
	DDRD &=	~(1<<PD6);
	
	
	// When device were switched off while armed.
	if(eeprom_read_byte(&sw_off_while_armed)) {
	
		LED_ON;
		
		dev_state = BLOCKED;
		
		// Inform why the device is blocked.
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
		}
		
		lcd_cls();
		
		if(dev_state == ROOT_KEY_ENTERING) {
			code_entering(ROOT_KEY_ENTERING);
		}
		
		lcd_cls();
		lcd_str_P(PSTR("Odblokowano"));
		delay_ms_x(5000);
		lcd_cls();
		
		LED_OFF;
	}
	
	dev_state = UNARMED;
	
	lcd_str_P(PSTR("Rozbrojona..."));
    
    while (1) {
		
		LED_OFF;
		
		if(dev_state == EXPLODED) {
			LED_ON;
			BUZZER_SET;
			lcd_cls();
			lcd_str_P(PSTR("Terrorysci"));
			lcd_locate(1, 0);
			lcd_str_P(PSTR("wygrali!"));
			delay_ms_x(5000);
			BUZZER_CLR;
			dev_state = UNARMED;
			INT1_EN;
			LED_OFF;
		}
		
		if(dev_state == UNARMING) {
			LED_ON;
			lcd_cls();
			lcd_str_P(PSTR("Rozbrajanie..."));
			lcd_locate(1, 0);
			for(uint8_t i = 0 ; i < LCD_COLS && dev_state == UNARMING; i ++) {
				lcd_char(0xFF);
				delay_ms_x(arm_bar_dur);
			}
			lcd_cls();
			LED_OFF;
		}
		
		if(dev_state == NOT_EXPLODED) {
			LED_ON;
			lcd_cls();
			lcd_str_P(PSTR("Bomba rozbrojona"));
			delay_ms_x(2000);
			dev_state = UNARMED;
			INT1_EN;
			LED_OFF;
		}
		
		if(dev_state == ARMING) {
			LED_ON;
			code_entering(ARMING);
			LED_OFF;
		}
		
		if(dev_state == ARMED) {
			LED_ON;
			lcd_cls();
			lcd_str((char *)keys_pressed);
			lcd_locate(1, 0);
			lcd_str_P(PSTR("UZBROJONO!"));
			while(dev_state == ARMED);
			LED_OFF;
		}
	
		if(dev_state == UNARMED) sleep_mode();	
    }
}

