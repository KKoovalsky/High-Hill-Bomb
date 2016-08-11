/*
 * display.h
 *
 * Created: 2016-08-06 10:14:34
 *  Author: maly_windows
 */ 


#ifndef DISPLAY_H_
#define DISPLAY_H_

// Display message on LCD, which contains more than 2 x 16 letters and less than 4 x 16 letters.
void two_msg(const char * str11, const char * str12, const char * str21, const char * str22);

// Display entered code and incentives.
void keys_entering(const char * str);

// Display key pressed.
void key_display();

// Display key pressed with bad code message.
void key_display_bad_code();

// Display key pressed with clearing bad code message.
void key_display_clear_bad_code();

// Display message, that bomb is armed.
void display_armed();

// Display message, that device is being unarmed.
void display_unarming();

// Display message, that bomb is unarmed.
void display_unarmed();

// Display message, that terrorists won.
void display_exploded();

void display_steady();

#endif /* DISPLAY_H_ */