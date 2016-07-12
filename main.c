#include "common.h"


uint8_t arm_code [KEY_NUM] EEMEM;
uint16_t EEMEM to_boom_time;
uint16_t EEMEM to_unarm_time;

// If bomb were switched off while armed.
uint8_t EEMEM sw_off_while_armed;

volatile bool root_code_entered = true;

int main(void)
{
	//TODO: Za³¹czenie podœwietlenia
	
	
	lcd_init();
	
	sei();
	
	//TODO:  Inicjalizacja
	if(eeprom_read_byte(&sw_off_while_armed)) {
		root_code_entered = false;
		while(!root_code_entered) {
			lcd_str_P(PSTR("Wyl. w trakcie"));
			lcd_locate(1, 0);
			lcd_str_P(PSTR("odliczania"));
		
			_delay_ms(2000);
			lcd_cls();
			
			lcd_str_P(PSTR("Dajcie Oskarowi"));
			lcd_locate(1, 0);
			lcd_str_P(PSTR("luj odblokuje"));
			
			_delay_ms(2000);
			lcd_cls();
			// Will probably need lcd_locate(0, 0)
		}
	}
	
    
    while (1) {
		
    }
}

