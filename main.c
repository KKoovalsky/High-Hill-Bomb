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

int main(void) {
	
	// Buzzer pin.
	DDRB |= (1<<PB5);

	// Unarming pins.
	DDRB &= ~((1<<PB6) | (1<<PB7));
	
	keys_pressed_tab_clr();
	
	Timer0_init();
	Timer1_init();
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
			
			// When wrong key entered:
			if(local_keys_pressed_num > keys_pressed_num) {
				lcd_str((char*)keys_pressed);
				lcd_locate(1, 0);
				lcd_str_P(PSTR("Zle haslo"));
				delay_ms_x(4000);
				lcd_cls();
			}
			
			// When key entered update LCD content:
			if(local_keys_pressed_num != keys_pressed_num) {
				lcd_str((char*)keys_pressed);
			}
		}
	}
	
	dev_state = UNARMED;
    
    while (1) {
		if(dev_state == EXPLODED) {
			BUZZER_SET;
			delay_ms_x(5000);
			BUZZER_CLR;
			dev_state = UNARMED;
		}
		if(dev_state == UNARMING) {
			lcd_cls();
			lcd_str_P(PSTR("Rozbrajanie..."))
			lcd_locate(1, 0);
			while(dev_state == UNARMING) {
				#define LCD_COLS 16
				
			}
		}
		if(dev_state == NOT_EXPLODED) {
			lcd_cls();
			lcd_str_P(PSTR("Bomba rozbrojona"));
		}
		
    }
}

