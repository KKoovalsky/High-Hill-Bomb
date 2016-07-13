#ifndef KEYBOARD_H_
#define KEYBOARD_H_

extern volatile bool root_key_check;
extern volatile bool arm_key_check;

void get_root_key();	
void get_arm_key();

#endif /* KEYBOARD_H_ */