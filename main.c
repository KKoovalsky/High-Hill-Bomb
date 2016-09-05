#include "common.h"
#include "admin_code.h"
#include <util/atomic.h>

uint16_t changeable_vars[4] EEMEM;

// If bomb were switched off while armed.
uint8_t sw_off_while_armed EEMEM;

volatile uint16_t delay_cnt_ms;

volatile uint8_t key_pressed;
volatile uint8_t keys_pressed_num;

volatile bool wait_flag;

volatile f_ptr_t main_exec;

const char def_arm_key [] PROGMEM = "1993";

const char new_code_msg [] PROGMEM = "Nowy kod uzbr.:";
const char new_disarm_time_msg [] PROGMEM = "Nowy c.uzbr.(*):";
const char new_countdown_time_msg [] PROGMEM = "Nowy c.rozb.(*):";
const char new_admin_code_msg [] PROGMEM = "Nowy kod admin.:";
const char * const msgs [] PROGMEM = {new_code_msg, new_disarm_time_msg, new_countdown_time_msg, new_admin_code_msg};

void Timer0_start() {
	// Start Timer and set prescaling.
	TCCR0B |= (1<<CS02) | (1<<CS00);
	
	// Clear timer register.
	TCNT0 = 0;
}

void Timer0_stop() {
	// Start Timer and set prescaling.
	TCCR0B &= ~((1<<CS02) | (1<<CS00));
}

void Timer2_start() {
	// Clear timer register.
	TCNT2 = 0;
	
	// Start Timer and set prescaling.
	TCCR2B |= (1<<CS21);
}

void Timer2_stop() {
	// Start Timer and set prescaling.
	TCCR2B &= ~((1<<CS22) | (1<<CS21) | (1<<CS20));
}

static inline void init_eeprom_if_default() {
	if(eeprom_read_word(&changeable_vars[ARM_CODE]) == 0xFFFF) eeprom_write_word(&changeable_vars[ARM_CODE], 1993);
	
	if(eeprom_read_word(&changeable_vars[TO_BOOM_TIME]) == 0xFFFF) eeprom_write_word(&changeable_vars[TO_BOOM_TIME], 40);
	
	if(eeprom_read_word(&changeable_vars[TO_DISARM_TIME]) == 0xFFFF) eeprom_write_word(&changeable_vars[TO_DISARM_TIME], 5);
	
	if(eeprom_read_word(&changeable_vars[ADMIN_CODE]) == 0xFFFF) eeprom_write_word(&changeable_vars[ADMIN_CODE], pgm_read_word(&admin_code));
	
	if(eeprom_read_byte(&sw_off_while_armed) == 0xFF) eeprom_write_byte(&sw_off_while_armed, 0);
}

void delay_ms_x (uint16_t ms_del) {
	bool exit = false;
	delay_cnt_ms = ms_del;
	
	Timer2_start();
	
	while(1) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			if(!delay_cnt_ms) exit = true;
		}
		if(exit) break;
	}
	
	Timer2_stop();
}

void set_fptr(volatile f_ptr_t * dest, f_ptr_t src) {
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		*dest = src;
	}
}

bool check_fptr(volatile f_ptr_t fptr) {
	return (fptr != NULL);
}

bool comp_fptr(volatile f_ptr_t left, f_ptr_t right) {
	return (left == right);
}

int main(void) {
	
	// Buzzer pin.
	DDRD |= (1<<PD4); DDRB |= (1<<PB2);
	
	// Backlight pin control.
	DDRC |= (1<<PC0);

	// Disarming pins.
	DDRC &= ~(1<<PC1); DDRD &= ~(1<<PD3);
	
	// Keyboard row input pins.
	DDRC &= ~((1<<PC2) | (1<<PC3) | (1<<PC4) | (1<<PC5));
	// Pullups.
	PORTC |= (1<<PC2) | (1<<PC3) | (1<<PC4) | (1<<PC5);
	// Keyboard column outputs - set 0 state (HIGH-Z, when DDR bits set to zero)
	PORTD &= ~((1<<PD0) | (1<<PD1) | (1<<PD2));
	
	LED_OFF;
	
	init_eeprom_if_default();
	
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	
	Timer0_init();
	Timer1_init();
	Timer2_init();
	ext_int_init();
	lcd_init();
	
	sei();

	// Check if the admin mode is called ('5' key pressed at startup).
	while(check_admin_mode_call()) {
		LED_ON;
		
		set_fptr(&int_exec, get_root_code);
		Timer0_start(); INT1_DIS;
		keys_entering(PSTR("Kod admin.:"));
		if(!CHECK_TIM0_RUN) break;

		// Ask user what does he want to change.
		set_fptr(&int_exec, what_to_change);
		two_msg(PSTR("Co zmienic?"), PSTR("1. Kod uzbr."), PSTR("2. Czas do bum"), PSTR("3. Czas anty-bum"));
		set_fptr(&main_exec, NULL);
		keys_entering((char*)pgm_read_word(&msgs[key_pressed - 48 - 1]));
		if(!CHECK_TIM0_RUN) break;
		Timer0_stop();
		if(check_fptr(main_exec)) main_exec();
		lcd_str_P(PSTR(" Ustawiono"));
		delay_ms_x(4000);
		break;
	}
	
	// When device were switched off while armed.
	if(eeprom_read_byte(&sw_off_while_armed)) {
		LED_ON;
		
		// Inform why the device is blocked.
		set_fptr(&int_exec, get_root_code);
		while(true) {
			two_msg(PSTR("Wylaczona gdy"), PSTR("uzbrojona"), PSTR("Dajcie Oskarowi"), PSTR("luj odblokuje xD"));
			keys_entering(PSTR("Kod admin.:"));
			if(!eeprom_read_byte(&sw_off_while_armed)) break;
		}
		lcd_char(key_pressed); lcd_str_P(PSTR(" Odblokowano"));
		Timer0_stop();
		delay_ms_x(4000);
	}
	
	lcd_cls();
	lcd_str_P(PSTR("Rozbrojona..."));
	
	wait_flag = true;
	
	while (1) {
		if(!wait_flag) {
			LED_ON;
			BUZZER1_CLR; BUZZER2_CLR;
			
			lcd_cls();
			lcd_str_P(PSTR("Wprowadz kod:"));
			lcd_locate(1, 0);
			
			while((!wait_flag)) {
				if(check_fptr(main_exec) && (!wait_flag)) {
					main_exec();
					set_fptr(&main_exec, NULL);
				}
			}
			if(check_fptr(main_exec)) { main_exec(); set_fptr(&main_exec, NULL);}
			
			delay_ms_x(4000);
			
			} else {
			LED_OFF;
			BUZZER1_CLR; BUZZER2_CLR;
			INT1_EN;
			set_fptr(&main_exec, NULL);
			sleep_mode();
			set_fptr(&int_exec, get_arm_code);
		}
	}
	
}

