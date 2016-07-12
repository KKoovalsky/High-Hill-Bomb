#include "common.h"
#include "ext_int.h"

void Timer0_init() {
	
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