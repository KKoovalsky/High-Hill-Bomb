#ifndef INT_EXECUTORS_H_
#define INT_EXECUTORS_H_

extern volatile uint8_t sec_cnt;

extern volatile char buffer[];

// Timer 1 interrupt execution functions (for arming countdown and disarming countdown purpose and adequate displaying on LCD)
void arm_countdown();
void disarming();

// Timer 0 interrupt execution functions (for keypad pressing services and adequate displaying on LCD)
void get_root_code();
void get_arm_code();
void what_to_change();
void change_ee_var();

// Additional private functions.
void update_ee();

#endif /* INT_EXECUTORS_H_ */