#ifndef EXT_INT_H_
#define EXT_INT_H_

#define TIMER0_INT_EN (TIMSK0 |= (1<<OCIE0A))
#define TIMER2_INT_EN (TIMSK2 |= (1<<OCIE2A))

typedef void( * f_ptr_t )( void );

extern volatile uint16_t * ee_var_ptr;

extern volatile f_ptr_t int_exec;

// Used for numeric keyboard multiplexation.
inline void Timer0_init() {
	// CTC mode set.
	TCCR0A |= (1<<WGM01);
	
	#define TIMER0_PRESCALER (1024.0)
	#define TIMER0_INT_FREQ (10.0)
	
	// Set ~100 ms interrupt
	OCR0A = F_CPU / TIMER0_PRESCALER / TIMER0_INT_FREQ - 0.5;
	
	TIMER0_INT_EN;
}

// Used for countdown to explosion and disarming countdown.
inline void Timer1_init() {
	// CTC mode set.
	TCCR1B |= (1<<WGM12);
	
	#define TIMER1_PRESCALER (8.0)
	#define TIMER1_INT_FREQ (4.0)
	#define TIMER1_INT_FREQ_INT ((uint8_t)TIMER1_INT_FREQ)
	
	// Start Timer and set prescaling.
	TCCR1B |= (1<<CS11);
	
	// Set ~250 ms interrupt
	OCR1A = F_CPU / TIMER1_PRESCALER / TIMER1_INT_FREQ  - 0.5;
}

// For delay purpose.
inline void Timer2_init() {
	// Set CTC mode.
	TCCR2A |= (1<<WGM21);
	
	#define TIMER2_PRESCALER (8.0)
	#define TIMER2_INT_FREQ (1000.0)
	
	// Set ~1ms interrupt.
	OCR2A = F_CPU / TIMER2_PRESCALER / TIMER2_INT_FREQ - 1;
	
	TIMER2_INT_EN;
}

void ext_int_init();

uint8_t get_key();

void inc_buffer();

#endif /* EXT_INT_H_ */