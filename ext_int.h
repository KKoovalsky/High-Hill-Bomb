#ifndef EXT_INT_H_
#define EXT_INT_H_

// Used for numeric keyboard multiplexation.
void Timer0_init();

// Used when bomb armed.
void Timer1_init();

// Used for delays.
void Timer2_init();

void ext_int_init();

#endif /* EXT_INT_H_ */