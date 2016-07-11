/**************************************************************************
* 
* Playtune: An Arduino Tune Generator
* 
* Plays a polyphonic musical score
* For documentation, see the Playtune.cpp source code file
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
*  19 January 2011, L.Shustek, V1.0: Initial release.
*  10 June 2013, L. Shustek, V1.3
*     - change for compatibility with Arduino IDE version 1.0.5
*   6 April 2015, L. Shustek, V1.4
*     - change for compatibility with Arduino IDE version 1.6.x
*  28 May 2016, T. Wasiluk
*     - added support for ATmega32U4
*  10 July 2016, Nick Shvelidze
*     - Fixed include file names for Arduino 1.6 on Linux.
*/

#ifndef Playtune_h
#define Playtune_h

#include <Arduino.h>

class Playtune
{
public:
 void tune_initchan (byte pin);			// assign a timer to an output pin
 void tune_playscore (const byte *score);	// start playing a polyphonic score
 volatile static boolean tune_playing;	// is the score still playing?
 void tune_stopscore (void);			// stop playing the score
 void tune_delay (unsigned msec);		// delay in milliseconds
 void tune_stopchans (void);			// stop all timers
};

#endif