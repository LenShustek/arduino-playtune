/*********************************************************************************************

     Playtune: An Arduino polyphonic music generator


                         About Playtune, generally

   Playtune is a family of music players for Arduino-like microcontrollers. They
   each intepret a bytestream of commands that represent a polyphonic musical
   score, and play it using different techniques.

   (1) This original Playtune, first released in 2011, uses a separate hardware timer
   to generate a square wave for each note played simultaneously. The timers run at twice
   the frequency of the note being played, and the interrupt routine flips the output bit.
   It can play only as many simultaneous notes as there are timers available. The sound
   quality? Buzzy square waves.
   https://github.com/LenShustek/arduino-playtune

   (2) The second ("polling") version uses only one hardware timer that interrupts often,
   by default at 20 Khz, or once every 50 microseconds. The interrupt routine determines
   which, if any, of the currently playing notes need to be toggled. It also implements
   primitive volume modulation by changing the duty cycle of the square wave.
   The advantage over the first version is that the number of simultaneous notes is not
   limited by the number of timers, only by the number of output pins. The sound quality
   is still "buzzy square waves".
   https://github.com/LenShustek/playtune_poll

   (3) The third version also uses only one hardware timer interrupting frequently, but
   uses the hardware digital-to-analog converter on high-performance microntrollers like
   the Teensy to generate an analog wave that is the sum of stored samples of sounds. The
   samples are scaled to the right frequency and volume, and any number of instrument
   samples can be used and mapped to MIDI patches. The sound quality is much better,
   although not in league with real synthesizers.
   https://github.com/LenShustek/playtune_samp

   For all these versions, once a score starts playing, the processing happens in
   the interrupt routine.  Any other "real" program can be running at the same time
   as long as it doesn't use the timer or the output pins that Playtune is using.

   **** Details about this version: arduino-playtune

  This uses the Arduino counters for generating tones, so the number of simultaneous
  note that can be played varies from 3 to 6 depending on which processor you have.
  See more information later. No volume modulation, percussion, or instrument simulation
  is done.

  Each timer (tone generator) can be associated with any digital output pin, not just the
  pins that are internally connected to the timer.

  Playtune generates a lot of interrupts because the toggling of the output bits is done
  in software, not by the timer hardware.  But measurements I made on a NANO show that
  Playtune uses less than 10% of the available processor cycles even when playing all
  three channels at pretty high frequencies.

  The easiest way to hear the music is to connect each of the output pins to a resistor
  (500 ohms, say).  Connect other ends of the resistors together and then to one
  terminal of an 8-ohm speaker.  The other terminal of the speaker is connected to
  ground.  No other hardware is needed!  But using an amplifier is nicer.

  ****  The public Playtune interface  ****

  There are five public functions and one public variable.

  void tune_initchan(byte pin)

    Call this to initialize each of the tone generators you want to use.  The argument
    says which pin to use as output.  The pin numbers are the digital "D" pins
    silkscreened on the Arduino board.  Calling this more times than your processor
    has timers will do no harm, but will not help either.

  void tune_playscore(byte *score)

    Call this pointing to a "score bytestream" to start playing a tune.  It will
    only play as many simultaneous notes as you have initialized tone generators;
    any more will be ignored.  See below for the format of the score bytestream.

  boolean tune_playing

    This global variable will be "true" if a score is playing, and "false" if not.
    You can use this to see when a score has finished.

  void tune_stopscore()

    This will stop a currently playing score without waiting for it to end by itself.

  void tune_delay(unsigned int msec)

    Delay for "msec" milliseconds.  This is provided because the usual Arduino
    "delay" function will stop working if you use all of your processor's
    timers for generating tones.

  void tune_stopchans()

    This disconnects all the timers from their pins and stops the interrupts.
    Do this when you don't want to play any more tunes.


   *****  The score bytestream  *****

   The bytestream is a series of commands that can turn notes on and off, and can
   start a waiting period until the next note change.  Here are the details, with
   numbers shown in hexadecimal.

   If the high-order bit of the byte is 1, then it is one of the following commands:

     9t nn  Start playing note nn on tone generator t.  Generators are numbered
            starting with 0.  The notes numbers are the MIDI numbers for the chromatic
            scale, with decimal 60 being Middle C, and decimal 69 being Middle A
            at 440 Hz.  The highest note is decimal 127 at about 12,544 Hz. except
            that percussion notes (instruments, really) range from 128 to 255 when
            relocated from track 9 by Miditones with the -pt option. This version of
            Playtune ignores those percussion notes.

      [vv]  If ASSUME_VOLUME is set to 1, or the file header tells us to,
            then we expect a third byte with the volume ("velocity") value from 1 to
            127. You can generate this from Miditones with the -v option.
            (Everything breaks for headerless files if the assumption is wrong!)
            This version of Playtune ignores volume information.

     8t     Stop playing the note on tone generator t.

     Ct ii  Change tone generator t to play instrument ii from now on.  Miditones will
            generate this with the -i option. This version of Playtune ignores
            instrument information if it is present.

     F0     End of score: stop playing.

     E0     End of score: start playing again from the beginning.

   If the high-order bit of the byte is 0, it is a command to wait.  The other 7 bits
   and the 8 bits of the following byte are interpreted as a 15-bit big-endian integer
   that is the number of milliseconds to wait before processing the next command.
   For example,

     07 D0

   would cause a wait of 0x07d0 = 2000 decimal millisconds or 2 seconds.  Any tones
   that were playing before the wait command will continue to play.

   The score is stored in Flash memory ("PROGMEM") along with the program, because
   there's a lot more of that than data memory.


   ****  Where does the score data come from?  ****

   Well, you can write the score by hand from the instructions above, but that's
   pretty hard.  An easier way is to translate MIDI files into these score commands,
   and I've written a program called "Miditones" to do that.  See the separate
   documentation for that program, which is also open source at
   https://github.com/lenshustek/miditones

   ****  More gory details  ****

   The number of hardware timers, and therefore the number of tones that can be
   played simultaneously, depends on the processor that is on your board, of
   which there is an ever-increasing number. Here are some. I've listed the
   processor, some  boards the use it, and the 8- 10- and 16-bit timers they have,
   in the order that Playtune will use them.

     ATMega8 (old Arduinos): 2 tones
        T1(16b), T2(8b) [Why not T0(8b) ??)
     ATmega168/328 (Nano, Uno, Mini, Fio): 3 tones
        T1(16b), T2(8b), T0(8b)
     ATmega1280/2560 (Mega2560, MegaADK): 6 tones
        T1(16b), T2(8b), T3(16b), T4(16b), T5(16b), T0(8b)
     ATmega32u (Micro, Leonardo): 4 tones
        T1(16b), T0(8b), T3(16b), T4(10b)

   Timer 0 is assigned last (except on the ATmega32u), because using
   it will disable the Arduino millis(), delay(), and the PWM functions.
   Timer 1 is used first and is used to time the score, so it is always
   kept running even if it isn't playing a note.

   The lowest MIDI note that can be played using the 8-bit timers
   depends on your processor's clock frequency.
      8 Mhz clock: note 12 (about 16.5 Hz, which is below the piano keyboard)
     16 Mhz clock: note 24 (about 32.5 Hz, C in octave 1)

   The highest MIDI note (127, about 12,544 Hz) can always be played, but can't
   always be heard.

   ****  Nostalgia from me  ****

   Writing Playtune was a lot of fun, because it essentially duplicates what I did
   as a graduate student at Stanford University in about 1973.  That project used the
   then-new Intel 8008 microprocessor, plus three hardware square-wave generators that
   I built out of 7400-series TTL.  The music compiler was written in Pascal and read
   scores that were hand-written in a notation I made up, which looked something like
   this:  C  Eb  4G  8G+  2R  +  F  D#
   This was before MIDI had been invented, and anyway I wasn't a pianist so I would
   not have been able to record my own playing.  I could barely read music well enough
   to transcribe scores, but I slowly did quite a few of them. MIDI is better!

   Len Shustek, originally 4 Feb 2011, then updated occasionally over the years.

  ------------------------------------------------------------------------------------
   The MIT License (MIT)
   Copyright (c) 2011, 2016, Len Shustek

  Permission is hereby granted, free of charge, to any person obtaining a copy of
  this software and associated documentation files (the "Software"), to deal in
  the Software without restriction, including without limitation the rights to use,
  copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
  Software, and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
**************************************************************************************/
/*
  Change log
   19 January 2011, L.Shustek, V1.0
      - Initial release, inspired by Brett Hagman's Tone Generator Library,
        https://github.com/bhagman/Tone
   23 February 2011, L. Shustek, V1.1
      - prevent hang if delay rounds to count of 0
    4 December 2011, L. Shustek, V1.2
      - add special TESLA_COIL mods
   10 June 2013, L. Shustek, V1.3
      - change for compatibility with Arduino IDE version 1.0.5
    6 April 2015, L. Shustek, V1.4
      - change for compatibility with Arduino IDE version 1.6.x
   28 May 2016, T. Wasiluk
      - added support for ATmega32U4
   10 July 2016, Nick Shvelidze
      - Fixed include file names for Arduino 1.6 on Linux.
   15 August 2016, L. Shustek,
      - Fixed a timing error: T Wasiluk's change to using a 16-bit timer instead
        of an 8-bit timer for score waits exposed a old bug that was in the original
        Brett Hagman code: when writing the timer OCR value, we need to clear the
        timer counter, or else (the manual says) "the counter [might] miss the compare
        match...and will have to count to its maximum value (0xFFFF) and wrap around
        starting at 0 before the compare match can occur". This caused an error that
        was small and not noticeable for the 8-bit timer, but could be hundreds of
        milliseconds for the 16-bit counter. Thanks go to Joey Babcock for pushing me
        to figure out why his music sounded weird, and for discovering that it worked
        ok with the 2013 version that used the 8-bit timer for score waits.
      - Support the optional bytestream header to recognize when volume data is present.
      - Parse and ignore instrument change data.
      - Various reformatting to make it easier to read.
      - Allow use of the fourth timer on the ATmega32U4 (Micro, Leonardo)
      - Change to the more permissive MIT license.

  -----------------------------------------------------------------------------------------*/

#include <Arduino.h>
#include "Playtune.h"

#ifndef DBUG
#define DBUG 0          // debugging?
#endif
#define ASSUME_VOLUME 0 // assume volume information is present in bytestream files without headers?
#define TESLA_COIL 0    // special Tesla Coil version?


struct file_hdr_t {  // the optional bytestream file header
  char id1;     // 'P'
  char id2;     // 't'
  unsigned char hdr_length; // length of whole file header
  unsigned char f1;         // flag byte 1
  unsigned char f2;         // flag byte 2
  unsigned char num_tgens;  // how many tone generators are used by this score
} file_header;
#define HDR_F1_VOLUME_PRESENT 0x80
#define HDR_F1_INSTRUMENTS_PRESENT 0x40
#define HDR_F1_PERCUSSION_PRESENT 0x20

// timer ports and masks

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
volatile byte *timer4_pin_port;
volatile byte timer4_pin_mask;
#endif
#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)
volatile byte *timer5_pin_port;
volatile byte timer5_pin_mask;
#endif

// Define the order to allocate timers.

#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)
#define AVAILABLE_TIMERS 6
const byte PROGMEM tune_pin_to_timer_PGM[] = {
  1, 2, 3, 4, 5, 0
};
#elif defined(__AVR_ATmega8__)
#define AVAILABLE_TIMERS 2
const byte PROGMEM tune_pin_to_timer_PGM[] = {
  1, 2
};
#elif defined(__AVR_ATmega32U4__)
#define AVAILABLE_TIMERS 4
const byte PROGMEM tune_pin_to_timer_PGM[] = {
  1, 0, 3, 4
};
#else
#define AVAILABLE_TIMERS 3
const byte PROGMEM tune_pin_to_timer_PGM[] = {
  1, 2, 0
};
#endif

//  Other local varables

byte _tune_pins[AVAILABLE_TIMERS];
byte _tune_num_chans = 0;

/* one of the timers is also used to time
  - score waits (whether or not that timer is playing a note)
  - tune_delay() delay requests
  We currently use timer1, since that is the common one available on different microcontrollers.
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
boolean volume_present = ASSUME_VOLUME;

// Table of midi note frequencies * 2
//   They are times 2 for greater accuracy, yet still fit in a word.
//   Generated from Excel by =ROUND(2*440/32*(2^((x-9)/12)),0) for 0<x<128
// The lowest notes might not work, depending on the Arduino clock frequency

const unsigned int PROGMEM tune_frequencies2_PGM[128] =
{
  16, 17, 18, 19, 21, 22, 23, 24, 26, 28, 29, 31, 33, 35, 37, 39, 41,
  44, 46, 49, 52, 55, 58, 62, 65, 69, 73, 78, 82, 87, 92, 98, 104, 110,
  117, 123, 131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233,
  247, 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494,
  523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988, 1047,
  1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
  2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729,
  3951, 4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040,
  7459, 7902, 8372, 8870, 9397, 9956, 10548, 11175, 11840, 12544,
  13290, 14080, 14917, 15804, 16744, 17740, 18795, 19912, 21096,
  22351, 23680, 25088
};

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
#if DBUG
    Serial.print("init pin "); Serial.print(pin);
    Serial.print(" on timer "); Serial.println(timer_num);
#endif
    switch (timer_num) { // All timers are put in CTC mode

#if !defined(__AVR_ATmega8__)
      case 0:  // 8 bit timer
        TCCR0A = 0;
        TCCR0B = 0;
        bitWrite(TCCR0A, WGM01, 1);
        bitWrite(TCCR0B, CS00, 1);
        timer0_pin_port = portOutputRegister(digitalPinToPort(pin));
        timer0_pin_mask = digitalPinToBitMask(pin);
        break;
#endif
      case 1:  // 16 bit timer
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
      case 2:  // 8 bit timer
        TCCR2A = 0;
        TCCR2B = 0;
        bitWrite(TCCR2A, WGM21, 1);
        bitWrite(TCCR2B, CS20, 1);
        timer2_pin_port = portOutputRegister(digitalPinToPort(pin));
        timer2_pin_mask = digitalPinToBitMask(pin);
        break;
#endif
#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)||defined(__AVR_ATmega32U4__)
      case 3:  // 16 bit timer
        TCCR3A = 0;
        TCCR3B = 0;
        bitWrite(TCCR3B, WGM32, 1); // CTC mode
        bitWrite(TCCR3B, CS30, 1);  // clk/1 (no prescaling)
        timer3_pin_port = portOutputRegister(digitalPinToPort(pin));
        timer3_pin_mask = digitalPinToBitMask(pin);
        break;
#endif
#if defined(__AVR_ATmega32U4__)
      case 4: // 10 bit timer, treated as 8 bit
        TCCR4A = 0;
        TCCR4B = 0;
        bitWrite(TCCR4B, CS40, 1); // clk/1 (no prescaling)
        timer4_pin_port = portOutputRegister(digitalPinToPort(pin));
        timer4_pin_mask = digitalPinToBitMask(pin);
        break;
#endif
#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)
      case 4:  // 16 bit timer
        TCCR4A = 0;
        TCCR4B = 0;
        bitWrite(TCCR4B, WGM42, 1);
        bitWrite(TCCR4B, CS40, 1);
        timer4_pin_port = portOutputRegister(digitalPinToPort(pin));
        timer4_pin_mask = digitalPinToBitMask(pin);
        break;
      case 5:  // 16 bit timer
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
  Serial.print(score_cursor - score_start, HEX);
  Serial.print(", ch");
  Serial.print(chan); Serial.print(' ');
  Serial.println(note, HEX);
#endif
  if (chan < _tune_num_chans) {
    timer_num = pgm_read_byte(tune_pin_to_timer_PGM + chan);
#if TESLA_COIL
    note = teslacoil_checknote(note);  // let teslacoil modify the note
#endif
    if (note > 127) note = 127;
    frequency2 = pgm_read_word (tune_frequencies2_PGM + note);
    // The stuff below really needs a rewrite to avoid so many divisions and to
    // make it easier to add new processors with different timer configurations!
    if (timer_num == 0 || timer_num == 2
#if defined(__AVR_ATmega32U4__)
        || timer_num == 4 // treat the 10-bit counter as an 8-bit counter
#endif
       ) { //***** 8 bit timer ******
      if (note < ( F_CPU <= 8000000UL ? 12 : 24))
        return;   //  too low to be playable
      // scan through prescalars to find the best fit
      ocr = F_CPU / frequency2 - 1;
      prescalarbits = 0b001;  // ck/1: same for all timers
      if (ocr > 255) {
        ocr = F_CPU / frequency2 / 8 - 1;
        prescalarbits = timer_num == 4 ? 0b0100 : 0b010;  // ck/8
        if (timer_num == 2 && ocr > 255) {
          ocr = F_CPU / frequency2 / 32 - 1;
          prescalarbits = 0b011; // ck/32
        }
        if (ocr > 255) {
          ocr = F_CPU / frequency2 / 64 - 1;
          prescalarbits = timer_num == 0 ? 0b011 : (timer_num == 4 ? 0b0111 : 0b100);  // ck/64
          if (timer_num == 2 && ocr > 255) {
            ocr = F_CPU / frequency2 / 128 - 1;
            prescalarbits = 0b101; // ck/128
          }
          if (ocr > 255) {
            ocr = F_CPU / frequency2 / 256 - 1;
            prescalarbits = timer_num == 0 ? 0b100 : (timer_num == 4 ? 0b1001 : 0b110); // clk/256
            if (ocr > 255) {
              // can't do any better than /1024
              ocr = F_CPU / frequency2 / 1024 - 1;
              prescalarbits = timer_num == 0 ? 0b101 : (timer_num == 4 ? 0b1011 : 0b111); // clk/1024
            }
          }
        }
      }
#if !defined(__AVR_ATmega8__)
      if (timer_num == 0) TCCR0B = (TCCR0B & 0b11111000) | prescalarbits;
#if defined(__AVR_ATmega32U4__)
      else if (timer_num == 4) {
        TCCR4B = (TCCR4B & 0b11110000) | prescalarbits;
      }
#endif
      else { // must be timer_num == 2
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
      if (timer_num == 1) TCCR1B = (TCCR1B & 0b11111000) | prescalarbits;
#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)||defined(__AVR_ATmega32U4__)
      else if (timer_num == 3) TCCR3B = (TCCR3B & 0b11111000) | prescalarbits;
#endif
#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)
      else if (timer_num == 4) TCCR4B = (TCCR4B & 0b11111000) | prescalarbits;
      else if (timer_num == 5) TCCR5B = (TCCR5B & 0b11111000) | prescalarbits;
#endif
    }

    // Set the OCR for the timer, zero the counter, then turn on the interrupts
    switch (timer_num) {
#if !defined(__AVR_ATmega8__)
      case 0:
        OCR0A = ocr;
        TCNT0 = 0;
        bitWrite(TIMSK0, OCIE0A, 1);
        break;
#endif
      case 1:
        OCR1A = ocr;
        TCNT1 = 0;
        wait_timer_frequency2 = frequency2;  // for "tune_delay" function
        wait_timer_playing = true;
        bitWrite(TIMSK1, OCIE1A, 1);
        break;
#if !defined(__AVR_ATmega32U4__)
      case 2:
        OCR2A = ocr;
        TCNT2 = 0;
        bitWrite(TIMSK2, OCIE2A, 1);
        break;
#endif
#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)||(__AVR_ATmega32U4__)
      case 3:
        OCR3A = ocr;
        TCNT3 = 0;
        bitWrite(TIMSK3, OCIE3A, 1);
        break;
#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)
      case 4:
        OCR4A = ocr;
        TCNT4 = 0;
        bitWrite(TIMSK4, OCIE4A, 1);
        break;
#endif
#if defined(__AVR_ATmega32U4__)
      case 4:// TOP value compare for this 10-bit register is in C!
        OCR4C = ocr / 2 + 1; //timer4 doesn't have CTC mode, but I don't understand the f/2
        // others have reported problems too, and apparently the chip as has bugs.
        // http://forum.arduino.cc/index.php?topic=261869.0
        // http://electronics.stackexchange.com/questions/245661/atmega32u4-generate-clock-using-timer4
        TCNT4 = 0;
        bitWrite(TIMSK4, OCIE4A, 1);
        break;
#endif
#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)
      case 5:
        OCR5A = ocr;
        TCNT5 = 0;
        bitWrite(TIMSK5, OCIE5A, 1);
        break;
#endif
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
  Serial.println(chan, DEC);
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
      // We leave the timer1 interrupt running for timing delays and score waits
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
    case 4:
      TIMSK4 &= ~(1 << OCIE4A);                 // disable the interrupt
      *timer4_pin_port &= ~(timer4_pin_mask);   // keep pin low after stop
      break;
#endif
#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)
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
  if (tune_playing) tune_stopscore();
  score_start = score;
  volume_present = ASSUME_VOLUME;

  // look for the optional file header
  memcpy_P(&file_header, score, sizeof(file_hdr_t)); // copy possible header from PROGMEM to RAM
  if (file_header.id1 == 'P' && file_header.id2 == 't') { // validate it
    volume_present = file_header.f1 & HDR_F1_VOLUME_PRESENT;
#if DBUG
    Serial.print("header: volume_present="); Serial.println(volume_present);
#endif
    score_start += file_header.hdr_length; // skip the whole header
  }
  score_cursor = score_start;
  tune_stepscore();  /* execute initial commands */
  Playtune::tune_playing = true;  /* release the interrupt routine */
}

void tune_stepscore (void) {
  byte cmd, opcode, chan, note;
  unsigned duration;
  /* Do score commands until a "wait" is found, or the score is stopped.
    This is called initially from tune_playcore, but then is called
    from the interrupt routine when waits expire.
  */
#define CMD_PLAYNOTE	0x90	/* play a note: low nibble is generator #, note is next byte */
#define CMD_STOPNOTE	0x80	/* stop a note: low nibble is generator # */
#define CMD_INSTRUMENT  0xc0 /* change instrument; low nibble is generator #, instrument is next byte */
#define CMD_RESTART	0xe0	/* restart the score from the beginning */
#define CMD_STOP	0xf0	/* stop playing */
  /* if CMD < 0x80, then the other 7 bits and the next byte are a 15-bit big-endian number of msec to wait */

  while (1) {
    cmd = pgm_read_byte(score_cursor++);
    if (cmd < 0x80) { /* wait count in msec. */
      duration = ((unsigned)cmd << 8) | (pgm_read_byte(score_cursor++));
      wait_toggle_count = ((unsigned long) wait_timer_frequency2 * duration + 500) / 1000;
      if (wait_toggle_count == 0) wait_toggle_count = 1;
#if DBUG
      Serial.print("wait "); Serial.print(duration);
      Serial.print("ms, cnt ");
      Serial.print(wait_toggle_count); Serial.print(" freq "); Serial.println(wait_timer_frequency2);
#endif
      break;
    }
    opcode = cmd & 0xf0;
    chan = cmd & 0x0f;
    if (opcode == CMD_STOPNOTE) { /* stop note */
      tune_stopnote (chan);
    }
    else if (opcode == CMD_PLAYNOTE) { /* play note */
      note = pgm_read_byte(score_cursor++); // argument evaluation order is undefined in C!
      if (volume_present) ++score_cursor; // ignore volume if present
      tune_playnote (chan, note);
    }
    else if (opcode == CMD_INSTRUMENT) { /* change a channel's instrument */
      score_cursor++; // ignore it
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
  for (i = 0; i < _tune_num_chans; ++i)
    tune_stopnote(i);
  Playtune::tune_playing = false;
}

//-----------------------------------------------
// Delay a specified number of milliseconds
//-----------------------------------------------

void Playtune::tune_delay (unsigned duration) {

  // We provide this because using timer 0 breaks the Arduino delay() function.
  // Compute the toggle count based on whatever frequency the timer used for
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

  for (chan = 0; chan < _tune_num_chans; ++chan) {
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
#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)||defined(__AVR_ATmega32U4__)
      case 4:
        TIMSK4 &= ~(1 << OCIE4A);
        break;
#endif
#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)
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
//  Timer Interrupt Service Routines
//-----------------------------------------------

#if !defined(__AVR_ATmega8__) && !TESLA_COIL
ISR(TIMER0_COMPA_vect) {  // **** TIMER 0
  *timer0_pin_port ^= timer0_pin_mask; // toggle the pin
}
#endif

ISR(TIMER1_COMPA_vect) {  // **** TIMER 1
  // We keep this running always and use it to time score waits, whether or not it is playing a note.
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
        // Need scaling to avoid 32-bit overflow...
        delay_toggle_count = ( (delay_toggle_count + 4 >> 3) * (wait_timer_frequency2 + 2 >> 2) / wait_timer_old_frequency2 ) << 5;
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
ISR(TIMER2_COMPA_vect) {  // **** TIMER 2
  *timer2_pin_port ^= timer2_pin_mask;  // toggle the pin
}
#endif
#endif

#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)||defined(__AVR_ATmega32U4__)
ISR(TIMER3_COMPA_vect) {  // **** TIMER 3
  *timer3_pin_port ^= timer3_pin_mask;  // toggle the pin
#if TESLA_COIL
  if (*timer3_pin_port & timer3_pin_mask) teslacoil_rising_edge (3);  // do a tesla coil pulse
#endif
}
#endif

#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)||defined(__AVR_ATmega32U4__)
ISR(TIMER4_COMPA_vect) {  // **** TIMER 4
  *timer4_pin_port ^= timer4_pin_mask;  // toggle the pin
#if TESLA_COIL
  if (*timer4_pin_port & timer4_pin_mask) teslacoil_rising_edge (4);  // do a tesla coil pulse
#endif
}
#endif

#if defined(__AVR_ATmega1280__)||defined(__AVR_ATmega2560__)
ISR(TIMER5_COMPA_vect) {  // **** TIMER 5
  *timer5_pin_port ^= timer5_pin_mask;  // toggle the pin
#if TESLA_COIL
  if (*timer5_pin_port & timer5_pin_mask) teslacoil_rising_edge (5);  // do a tesla coil pulse
#endif
}
#endif


