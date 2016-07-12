#ifndef COMMON_H_
#define COMMON_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <stdbool.h>

#include "LCD/lcd44780.h"

#define KEY_NUM (4)

extern volatile bool root_code_entered;


#endif /* COMON_H_ */