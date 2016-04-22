/******************************************************************************
*
* Playtune: An Arduino Tune Generator library
*
* Plays a polyphonic musical score.
*
*   (C) Copyright 2011, Len Shustek
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of version 3 of the GNU General Public License as
*   published by the Free Software Foundation at http://www.gnu.org/licenses,
*   with Additional Permissions under term 7(b) that the original copyright
*   notice and author attibution must be preserved and under term 7(c) that
*   modified versions be marked as different from the original.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   This was inspired by and adapted from "A Tone Generator Library",
*   written by Brett Hagman, http://www.roguerobotics.com/
*
**************************************************************************/
/*
* Change log
*  19 January 2011, L.Shustek, V1.0
*     - Initial release.
*  23 February 2011, L. Shustek, V1.1
*     - prevent hang if delay rounds to count of 0
*   4 December 2011, L. Shustek, V1.2
*     - add special TESLA_COIL mods
*  10 June 2013, L. Shustek, V1.3
*     - change for compatibility with Arduino IDE version 1.0.5
*   6 April 2015, L. Shustek, V1.4
*     - change for compatibility with Arduino IDE version 1.6.x
*
*/

/*---------------------------------------------------------------------------------
*
*
*                              About Playtune
*
*
*  Playtune interprets a bytestream of commands that represent a polyphonic musical
*  score.  It uses the Arduino counters for generating tones, so the number of
*  simultaneous notes that can be played depends on which processor you have.
*  The NANO allows 3, and the MEGA 2560 allows 6.  Each timer (tone generator) can
*  be associated with any digital output pin, not just the pins that are internally
*  connected to the timer.
*
*  Once a score starts playing, all of the processing happens in interrupt routines,
*  so any other "real" program can be running at the same time, as long as it doesn't
*  use the timers or output pins that Playtune is using.  Playtune generates a lot of
*  interrupts because the toggling of the output bits is done in software, not by the
*  timer hardware.  But measurements I've made on a NANO show that Playtune uses less
*  than 10% of the available processor cycles even when playing all three channels at
*  pretty high frequencies.
*
*  The easiest way to hear the music is to connect each of the output pins to a resistor
*  (500 ohms, say).  Connect other ends of the resistors together and then to one
*  terminal of an 8-ohm speaker.  The other terminal of the speaker is connected to
*  ground.  No other hardware is needed!  If you are going to connect to an amplifier,
*  you should DC-isolate the signal using a capacitor.
*
*  There is no volume modulation.  All tones are played at the same volume, which
*  makes some scores sound strange.  This is definitely not a high-quality synthesizer.
*
*
*  *****  The public Playtune interface  *****
*
*  There are five public functions and one public variable.
*
*  void tune_initchan(byte pin)
*
*    Call this to initialize each of the tone generators you want to use.  The argument
*    says which pin to use as output.  The pin numbers are the digital "D" pins
*    silkscreened on the Arduino board.  Calling this more times than your processor
*    has timers will do no harm, but will not help either.
*
*  void tune_playscore (byte *score)
*
*    Call this pointing to a "score bytestream" to start playing a tune.  It will
*    only play as many simultaneous notes as you have initialized tone generators;
*    any more will be ignored.  See below for the format of the score bytestream.
*
*  boolean tune_playing
*
*    This global variable will be "true" if a score is playing, and "false" if not.
*    You can use this to see when a score has finished.
*
*  void tune_stopscore()
*
*    This will stop a currently playing score without waiting for it to end by itself.
*
*  void tune_delay (unsigned int msec)
*
*    Delay for "msec" milliseconds.  This is provided because the usual Arduino
*    "delay" function will stop working if you use all of your processor's
*    timers for generating tones.
*
*  void tune_stopchans ()
*
*    This disconnects all the timers from their pins and stops the interrupts.
*    Do this when you don't want to play any more tunes.
*
*
*  *****  The score bytestream  *****
*
*  The bytestream is a series of commands that can turn notes on and off, and can
*  start a waiting period until the next note change.  Here are the details, with
*  numbers shown in hexadecimal.
*
*  If the high-order bit of the byte is 1, then it is one of the following commands:
*
*    9t nn  Start playing note nn on tone generator t.  Generators are numbered
*           starting with 0.  The notes numbers are the MIDI numbers for the chromatic
*           scale, with decimal 60 being Middle C, and decimal 69 being Middle A
*           at 440 Hz.  The highest note is decimal 127 at about 12,544 Hz.
*
*    8t     Stop playing the note on tone generator t.
*
*    F0     End of score: stop playing.
*
*    E0     End of score: start playing again from the beginning.
*
*  If the high-order bit of the byte is 0, it is a command to wait.  The other 7 bits
*  and the 8 bits of the following byte are interpreted as a 15-bit big-endian integer
*  that is the number of milliseconds to wait before processing the next command.
*  For example,
*
*    07 D0
*
*  would cause a wait of 0x07d0 = 2000 decimal millisconds or 2 seconds.  Any tones
*  that were playing before the wait command will continue to play.
*
*  The score is stored in Flash memory ("PROGMEM") along with the program, because
*  there's a lot more of that than data memory.
*
*
*  *****  Where does the score data come from?  *****
*
*  Well, you can write the score by hand from the instructions above, but that's
*  pretty hard.  An easier way is to translate MIDI files into these score commands,
*  and I've written a program called "miditones" to do that.  See the separate
*  documentation for that program, which is also open source.
*
*
*  *****  More gory details  *****
*
*  The number of hardware timers, and therefore the number of tones that can be
*  played simultaneously, depends on the processor that is on your board:
*
*    ATMega8: 2 tones  (timers 2 and 1)
*    ATmega168/328: 3 tones (timers 2, 1, and 0)
*    ATmega1280/2560: 6 tones (timers 2, 3, 4, 5, 1, and 0)
*    ATmega32u: 3 tones (timers 3, 1, 0)
*
*  The order listed above is the order that timers are assigned as you make
*  succesive calls to play_initchan().  Timer 0 is assigned last, because using
*  it will disable the Arduino millis(), delay(), and the PWM functions.
*
*  The lowest MIDI note that can be played using the 8-bit timers (0 and 2)
*  depends on your processor's clock frequency.
*     8 Mhz clock: note 12 (about 16.5 Hz, which is below the piano keyboard)
*    16 Mhz clock: note 24 (about 32.5 Hz, C in octave 1)
*
*  The highest MIDI note (127, about 12,544 Hz) can always be played, but can't
*  always be heard.
*
*
*  *****  Nostalgia from me  *****
*
*  Writing Playtune was a lot of fun, because it essentially duplicates what I did
*  as a graduate student at Stanford University in about 1973.  That project used the
*  then-new Intel 8008 microprocessor, plus three hardware square-wave generators that
*  I built out of 7400-series TTL.  The music compiler was written in Pascal and read
*  scores that were hand-written in a notation I made up that looked something like
*      C  Eb  4G  8G+  2R  +  F  D#
*  This was done was before MIDI had been invented, and anyway I wasn't a pianist so I
*  would not have been able to record my own playing.  I could barely read music well
*  enough to transcribe scores, but I created, slowly, quite a few of them.
*
*  Len Shustek, 4 Feb 2011
*
-------------------------------------------------------------------------------------*/

#define DBUG 0       /* debugging? */
#define TESLA_COIL 0 /* special Tesla Coil version? */

#include <arduino.h>

#include "playtune.h"

#if defined(__AVR_ATmega8__)
#define TCCR2A TCCR2
#define TCCR2B TCCR2
#define COM2A1 COM21
#define COM2A0 COM20
#define OCR2A OCR2
#define TIMSK2 TIMSK
#define OCIE2A OCIE2
#define TIMER2_COMPA_vect TIMER2_COMP_vect
#define TIMSK1 TIMSK
#endif

//*******  private variables  ***************

// timer ports and masks

#if !defined(__AVR_ATmega8__)
volatile byte *timer0_pin_port;
volatile byte timer0_pin_mask;
#endif

volatile byte *timer1_pin_port;
volatile byte timer1_pin_mask;

#if !defined(__AVR_ATmega32U4__)
volatile byte *timer2_pin_port;
volatile byte timer2_pin_mask;
#endif

#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)||defined(__AVR_ATmega32U4__)
volatile byte *timer3_pin_port;
volatile byte timer3_pin_mask;
#endif

#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)
volatile byte *timer4_pin_port;
volatile byte timer4_pin_mask;

volatile byte *timer5_pin_port;
volatile byte timer5_pin_mask;
#endif

// Define the order to allocate timers, leaving timers 1 and 0 to last.

#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)
#define AVAILABLE_TIMERS 6
const byte PROGMEM tune_pin_to_timer_PGM[] = {
    1, 2, 3, 4, 5, 0 };
#elif defined(__AVR_ATmega8__)
#define AVAILABLE_TIMERS 2
const byte PROGMEM tune_pin_to_timer_PGM[] = {
    1, 2 };
#elif defined(__AVR_ATmega32U4__)
#define AVAILABLE_TIMERS 3
const byte PROGMEM tune_pin_to_timer_PGM[] = {
    1, 0, 3 };
    #else
#define AVAILABLE_TIMERS 3
const byte PROGMEM tune_pin_to_timer_PGM[] = {
    1, 2, 0 };
#endif

//  Other local varables

byte _tune_pins[AVAILABLE_TIMERS];
byte _tune_num_chans = 0;

/* one of the timers is also used to time
- score waits (whether or not that timer is playing a note)
- tune_delay() delay requests
We currently use timer1, since that one is the common one available on different microcontrollers
*/
volatile unsigned wait_timer_frequency2;       /* its current frequency */
volatile unsigned wait_timer_old_frequency2;   /* its previous frequency */
volatile boolean wait_timer_playing = false;   /* is it currently playing a note? */
volatile boolean doing_delay = false;          /* are we using it for a tune_delay()? */
volatile unsigned long wait_toggle_count;      /* countdown score waits */
volatile unsigned long delay_toggle_count;     /* countdown tune_ delay() delays */

volatile const byte *score_start = 0;
volatile const byte *score_cursor = 0;
volatile boolean Playtune::tune_playing = false;

// Table of midi note frequencies * 2
//   They are times 2 for greater accuracy, yet still fits in a word.
//   Generated from Excel by =ROUND(2*440/32*(2^((x-9)/12)),0) for 0<x<128
// The lowest notes might not work, depending on the Arduino clock frequency

const unsigned int PROGMEM tune_frequencies2_PGM[128] =
{
    16,17,18,19,21,22,23,24,26,28,29,31,33,35,37,39,41,
    44,46,49,52,55,58,62,65,69,73,78,82,87,92,98,104,110,
    117,123,131,139,147,156,165,175,185,196,208,220,233,
    247,262,277,294,311,330,349,370,392,415,440,466,494,
    523,554,587,622,659,698,740,784,831,880,932,988,1047,
    1109,1175,1245,1319,1397,1480,1568,1661,1760,1865,1976,
    2093,2217,2349,2489,2637,2794,2960,3136,3322,3520,3729,
    3951,4186,4435,4699,4978,5274,5588,5920,6272,6645,7040,
    7459,7902,8372,8870,9397,9956,10548,11175,11840,12544,
    13290,14080,14917,15804,16744,17740,18795,19912,21096,
    22351,23680,25088};

void tune_playnote (byte chan, byte note);
void tune_stopnote (byte chan);
void tune_stepscore (void);

#if TESLA_COIL
void teslacoil_rising_edge(byte timernum);
byte teslacoil_checknote(byte note);
#endif

//------------------------------------------------------
// Initialize a music channel on a specific output pin
//------------------------------------------------------

void Playtune::tune_initchan(byte pin) {
    byte timer_num;

    if (_tune_num_chans < AVAILABLE_TIMERS) {
        timer_num = pgm_read_byte(tune_pin_to_timer_PGM + _tune_num_chans);
        _tune_pins[_tune_num_chans] = pin;
        _tune_num_chans++;
        pinMode(pin, OUTPUT);

        // Set timer specific stuff
        // All timers in CTC mode
        // 8 bit timers will require changing prescalar values,
        // whereas 16 bit timers are set to either ck/1 or ck/64 prescalar

        switch (timer_num) {

#if !defined(__AVR_ATmega8__)
        case 0:
            // 8 bit timer
            TCCR0A = 0;
            TCCR0B = 0;
            bitWrite(TCCR0A, WGM01, 1);
            bitWrite(TCCR0B, CS00, 1);
            timer0_pin_port = portOutputRegister(digitalPinToPort(pin));
            timer0_pin_mask = digitalPinToBitMask(pin);
            break;
#endif

        case 1:
            // 16 bit timer
            TCCR1A = 0;
            TCCR1B = 0;
            bitWrite(TCCR1B, WGM12, 1);
            bitWrite(TCCR1B, CS10, 1);
            timer1_pin_port = portOutputRegister(digitalPinToPort(pin));
            timer1_pin_mask = digitalPinToBitMask(pin);
            tune_playnote (0, 60);  /* start and stop channel 0 (timer 1) on middle C so wait/delay works */
            tune_stopnote (0);
            break;

#if !defined(__AVR_ATmega32U4__)
        case 2:
            // 8 bit timer
            TCCR2A = 0;
            TCCR2B = 0;
            bitWrite(TCCR2A, WGM21, 1);
            bitWrite(TCCR2B, CS20, 1);
            timer2_pin_port = portOutputRegister(digitalPinToPort(pin));
            timer2_pin_mask = digitalPinToBitMask(pin);
            break;
#endif

#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)||defined(__AVR_ATmega32U4__)
        case 3:
            // 16 bit timer
            TCCR3A = 0;
            TCCR3B = 0;
            bitWrite(TCCR3B, WGM32, 1);
            bitWrite(TCCR3B, CS30, 1);
            timer3_pin_port = portOutputRegister(digitalPinToPort(pin));
            timer3_pin_mask = digitalPinToBitMask(pin);
            break;
#endif

#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)
        case 4:
            // 16 bit timer
            TCCR4A = 0;
            TCCR4B = 0;
            bitWrite(TCCR4B, WGM42, 1);
            bitWrite(TCCR4B, CS40, 1);
            timer4_pin_port = portOutputRegister(digitalPinToPort(pin));
            timer4_pin_mask = digitalPinToBitMask(pin);
            break;

        case 5:
            // 16 bit timer
            TCCR5A = 0;
            TCCR5B = 0;
            bitWrite(TCCR5B, WGM52, 1);
            bitWrite(TCCR5B, CS50, 1);
            timer5_pin_port = portOutputRegister(digitalPinToPort(pin));
            timer5_pin_mask = digitalPinToBitMask(pin);
            break;
#endif
        }
    }
}


//-----------------------------------------------
// Start playing a note on a particular channel
//-----------------------------------------------

void tune_playnote (byte chan, byte note) {
    byte timer_num;
    byte prescalarbits = 0b001;
    unsigned int frequency2; /* frequency times 2 */
    unsigned long ocr;

#if DBUG
    Serial.print ("Play at ");
    Serial.print(score_cursor-score_start,HEX);
    Serial.print(", ");
    Serial.print(chan,HEX);
    Serial.println(note,HEX);
#endif
    if (chan < _tune_num_chans) {
        timer_num = pgm_read_byte(tune_pin_to_timer_PGM + chan);
#if TESLA_COIL
        note = teslacoil_checknote(note);  // let teslacoil modify the note
#endif
        if (note>127) note=127;
        frequency2 = pgm_read_word (tune_frequencies2_PGM + note);

        if (timer_num == 0 || timer_num == 2) { //***** 8 bit timer ******

            // make sure the note isn't too low to be playable
            if (note <
    #if F_CPU <= 8000000UL
                12
    #else
                24
    #endif
                ) return;  // ignore if so

            // scan through prescalars to find the best fit
            ocr = F_CPU / frequency2 - 1;
            prescalarbits = 0b001;  // ck/1: same for both timers
            if (ocr > 255) {
                ocr = F_CPU / frequency2 / 8 - 1;
                prescalarbits = 0b010;  // ck/8: same for both timers

                if (timer_num == 2 && ocr > 255) {
                    ocr = F_CPU / frequency2 / 32 - 1;
                    prescalarbits = 0b011;
                }

                if (ocr > 255) {
                    ocr = F_CPU / frequency2 / 64 - 1;
                    prescalarbits = timer_num == 0 ? 0b011 : 0b100;

                    if (timer_num == 2 && ocr > 255) {
                        ocr = F_CPU / frequency2 / 128 - 1;
                        prescalarbits = 0b101;
                    }

                    if (ocr > 255) {
                        ocr = F_CPU / frequency2 / 256 - 1;
                        prescalarbits = timer_num == 0 ? 0b100 : 0b110;
                        if (ocr > 255) {
                            // can't do any better than /1024
                            ocr = F_CPU / frequency2 / 1024 - 1;
                            prescalarbits = timer_num == 0 ? 0b101 : 0b111;
                        }
                    }
                }
            }

#if !defined(__AVR_ATmega8__)
            if (timer_num == 0)        TCCR0B = (TCCR0B & 0b11111000) | prescalarbits;
            else {
#endif
#if !defined(__AVR_ATmega32U4__)
                TCCR2B = (TCCR2B & 0b11111000) | prescalarbits;
#endif            
            }    
        }
        else  //******  16-bit timer  *********
        { // two choices for the 16 bit timers: ck/1 or ck/64
            ocr = F_CPU / frequency2 - 1;

            prescalarbits = 0b001;
            if (ocr > 0xffff) {
                ocr = F_CPU / frequency2 / 64 - 1;
                prescalarbits = 0b011;
            }

            if (timer_num == 1)        TCCR1B = (TCCR1B & 0b11111000) | prescalarbits;
#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)||defined(__AVR_ATmega32U4__)
            else if (timer_num == 3)        TCCR3B = (TCCR3B & 0b11111000) | prescalarbits;
#endif          
#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)
            else if (timer_num == 4)        TCCR4B = (TCCR4B & 0b11111000) | prescalarbits;
            else if (timer_num == 5)        TCCR5B = (TCCR5B & 0b11111000) | prescalarbits;
#endif
        }

        // Set the OCR for the given timer, then turn on the interrupts
        switch (timer_num) {

#if !defined(__AVR_ATmega8__)
        case 0:
            OCR0A = ocr;
            bitWrite(TIMSK0, OCIE0A, 1);
            break;
#endif

        case 1:
            OCR1A = ocr;
            wait_timer_frequency2 = frequency2;  // for "tune_delay" function
            wait_timer_playing = true;
            bitWrite(TIMSK1, OCIE1A, 1);
            break;
#if !defined(__AVR_ATmega32U4__)
        case 2:
            OCR2A = ocr;            
            bitWrite(TIMSK2, OCIE2A, 1);
            break;
#endif

#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)
        case 3:
            OCR3A = ocr;
            bitWrite(TIMSK3, OCIE3A, 1);
            break;

        case 4:
            OCR4A = ocr;
            bitWrite(TIMSK4, OCIE4A, 1);
            break;

        case 5:
            OCR5A = ocr;
            bitWrite(TIMSK5, OCIE5A, 1);
            break;
#endif
        }
    }
}


//-----------------------------------------------
// Stop playing a note on a particular channel
//-----------------------------------------------

void tune_stopnote (byte chan) {
    byte timer_num;

#if DBUG
    Serial.print ("Stop note ");
    Serial.println(chan,DEC);
#endif

    timer_num = pgm_read_byte(tune_pin_to_timer_PGM + chan);
    switch (timer_num) {
#if !defined(__AVR_ATmega8__)
    case 0:
        TIMSK0 &= ~(1 << OCIE0A);                 // disable the interrupt
        *timer0_pin_port &= ~(timer0_pin_mask);   // keep pin low after stop
        break;
#endif
    case 1:
        // We leave the timer1 interrupt running for the "tune_delay" function.
        wait_timer_playing = false;
        *timer1_pin_port &= ~(timer1_pin_mask);   // keep pin low after stop
        break;
#if !defined(__AVR_ATmega32U4__)
    case 2:
        TIMSK2 &= ~(1 << OCIE1A);                 // disable the interrupt
        *timer2_pin_port &= ~(timer2_pin_mask);   // keep pin low after stop
        break;
#endif
#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)||defined(__AVR_ATmega32U4__)
    case 3:
        TIMSK3 &= ~(1 << OCIE3A);                 // disable the interrupt
        *timer3_pin_port &= ~(timer3_pin_mask);   // keep pin low after stop
        break;
#endif
#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)
    case 4:
        TIMSK4 &= ~(1 << OCIE4A);                 // disable the interrupt
        *timer4_pin_port &= ~(timer4_pin_mask);   // keep pin low after stop
        break;
    case 5:
        TIMSK5 &= ~(1 << OCIE5A);                 // disable the interrupt
        *timer5_pin_port &= ~(timer5_pin_mask);   // keep pin low after stop
        break;
#endif
    }
}

//-----------------------------------------------
// Start playing a score
//-----------------------------------------------

void Playtune::tune_playscore (const byte *score) {
    score_start = score;
    score_cursor = score;
    tune_stepscore();  /* execute initial commands */
    Playtune::tune_playing = true;  /* release the interrupt routine */
}

/* Do score commands until a "wait" is found, or the score is stopped.
This is called initially from tune_playcore, but then is called
from the interrupt routine when waits expire.
*/

#define CMD_PLAYNOTE	0x90	/* play a note: low nibble is generator #, note is next byte */
#define CMD_STOPNOTE	0x80	/* stop a note: low nibble is generator # */
#define CMD_RESTART	0xe0	/* restart the score from the beginning */
#define CMD_STOP	0xf0	/* stop playing */
/* if CMD < 0x80, then the other 7 bits and the next byte are a 15-bit big-endian number of msec to wait */


void tune_stepscore (void) {
    byte cmd, opcode, chan;
    unsigned duration;

    while (1) {
        cmd = pgm_read_byte(score_cursor++);
#if DBUG
//      Serial.print("cmd ");
//      Serial.println(cmd, HEX);
#endif
        if (cmd < 0x80) { /* wait count in msec. */
            duration = ((unsigned)cmd << 8) | (pgm_read_byte(score_cursor++));
            wait_toggle_count = ((unsigned long) wait_timer_frequency2 * duration + 500) / 1000;
		if (wait_toggle_count == 0) wait_toggle_count = 1;
#if DBUG
            Serial.print("wait ");
		Serial.print(duration,DEC);
		Serial.print(", cnt ");
		Serial.println(wait_toggle_count, DEC);
#endif
            break;
        }
        opcode = cmd & 0xf0;
        chan = cmd & 0x0f;
        if (opcode == CMD_STOPNOTE) { /* stop note */
            tune_stopnote (chan);
        }
        else if (opcode == CMD_PLAYNOTE) { /* play note */
            tune_playnote (chan, pgm_read_byte(score_cursor++));
        }
        else if (opcode == CMD_RESTART) { /* restart score */
            score_cursor = score_start;
        }
        else if (opcode == CMD_STOP) { /* stop score */
            Playtune::tune_playing = false;
            break;
        }
    }
}


//-----------------------------------------------
// Stop playing a score
//-----------------------------------------------

void Playtune::tune_stopscore (void) {
    int i;
    for (i=0; i<_tune_num_chans; ++i)
        tune_stopnote(i);
    Playtune::tune_playing = false;
}


//-----------------------------------------------
// Delay a specified number of milliseconds
//-----------------------------------------------


void Playtune::tune_delay (unsigned duration) {

    // We provide this because using timer 0 breaks the Arduino delay() function.
    // We compute the toggle count based on whatever frequency the timer used for
    // score waits is running at.  If the frequency of that timer changes, the
    // toggle count will be adjusted by the interrupt routine.

    boolean notdone;

    noInterrupts();
      delay_toggle_count = ((unsigned long) wait_timer_frequency2 * duration + 500) / 1000;
      doing_delay = true;
    interrupts();
    do { // wait until the interrupt routines decrements the toggle count to zero
        noInterrupts();
          notdone = delay_toggle_count != 0;  /* interrupt-safe test */
        interrupts();
    }
    while (notdone);
    doing_delay = false;
}

//-----------------------------------------------
// Stop all channels
//-----------------------------------------------

void Playtune::tune_stopchans(void) {
    byte chan;
    byte timer_num;

    for (chan=0; chan<_tune_num_chans; ++chan) {
        timer_num = pgm_read_byte(tune_pin_to_timer_PGM + chan);
        switch (timer_num) {

#if !defined(__AVR_ATmega8__)
        case 0:
            TIMSK0 &= ~(1 << OCIE0A);  // disable all timer interrupts
            break;
#endif
        case 1:
            TIMSK1 &= ~(1 << OCIE1A);
            break;
#if !defined(__AVR_ATmega32U4__)
        case 2:
            TIMSK2 &= ~(1 << OCIE2A);
            break;
#endif
#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)||defined(__AVR_ATmega32U4__)
        case 3:
            TIMSK3 &= ~(1 << OCIE3A);
            break;
#endif            
#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)
        case 4:
            TIMSK4 &= ~(1 << OCIE4A);
            break;
        case 5:
            TIMSK5 &= ~(1 << OCIE5A);
            break;
#endif
        }
        digitalWrite(_tune_pins[chan], 0);
    }
    _tune_num_chans = 0;
}

//-----------------------------------------------
//  Timer interrupt Service Routines
//-----------------------------------------------


#if !defined(__AVR_ATmega8__) && !TESLA_COIL  // TIMER 0
ISR(TIMER0_COMPA_vect) {
    *timer0_pin_port ^= timer0_pin_mask; // toggle the pin
}
#endif

ISR(TIMER1_COMPA_vect) {  // TIMER 1

    // Timer 1 - we keep it running always
    // and use it to time score waits, whether or not it is playing a note.

    if (wait_timer_playing) { // toggle the pin if we're sounding a note
    *timer1_pin_port ^= timer1_pin_mask;
#if TESLA_COIL
    if (*timer1_pin_port & timer1_pin_mask) teslacoil_rising_edge (2);  // do a tesla coil pulse
#endif
    }

    if (Playtune::tune_playing && wait_toggle_count && --wait_toggle_count == 0) {
        // end of a score wait, so execute more score commands
        wait_timer_old_frequency2 = wait_timer_frequency2;  // save this timer's frequency
        tune_stepscore ();  // execute commands
        // If this timer's frequency has changed and we're using it for a tune_delay(),
        // recompute the number of toggles to wait for
        if (doing_delay && wait_timer_old_frequency2 != wait_timer_frequency2) {
            if (delay_toggle_count >= 0x20000UL && wait_timer_frequency2 >= 0x4000U) {
                // Need to avoid 32-bit overflow...
                delay_toggle_count = ( (delay_toggle_count+4>>3) * (wait_timer_frequency2+2>>2) / wait_timer_old_frequency2 )<<5;
            }
            else {
                delay_toggle_count = delay_toggle_count * wait_timer_frequency2 / wait_timer_old_frequency2;
            }
        }
    }

    if (doing_delay && delay_toggle_count) --delay_toggle_count;	// countdown for tune_delay()
}

#if !defined(__AVR_ATmega32U4__)
#if !TESLA_COIL
ISR(TIMER2_COMPA_vect) {  // TIMER 2
    *timer2_pin_port ^= timer2_pin_mask;  // toggle the pin
}
#endif
#endif

#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)||defined(__AVR_ATmega32U4__)

ISR(TIMER3_COMPA_vect) {  // TIMER 3
    *timer3_pin_port ^= timer3_pin_mask;  // toggle the pin
#if TESLA_COIL
    if (*timer3_pin_port & timer3_pin_mask) teslacoil_rising_edge (3);  // do a tesla coil pulse
#endif
}
#endif

#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)
ISR(TIMER4_COMPA_vect) {  // TIMER 4
    *timer4_pin_port ^= timer4_pin_mask;  // toggle the pin
#if TESLA_COIL
    if (*timer4_pin_port & timer4_pin_mask) teslacoil_rising_edge (4);  // do a tesla coil pulse
#endif
}

ISR(TIMER5_COMPA_vect) {  // TIMER 5
    *timer5_pin_port ^= timer5_pin_mask;  // toggle the pin
#if TESLA_COIL
    if (*timer5_pin_port & timer5_pin_mask) teslacoil_rising_edge (5);  // do a tesla coil pulse
#endif
}

#endif


