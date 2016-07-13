#include "common.h"
#include "keyboard.h"

volatile bool root_key_check;
volatile bool arm_key_check;

void get_root_key() {
	TIMER0_INT_EN;
	root_key_check = true;
	arm_key_check = false;
}

void get_arm_key() {
	
}