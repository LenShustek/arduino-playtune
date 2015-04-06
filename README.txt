PLAYTUNE interprets a sequence of simple commands ("note on", "note off", and "wait") that 
represents a polyphonic musical score without volume modulation.  Once the score has started 
playing, background interrupt routines use the Arduino counters to generate tones in sequence 
at the right time.  As many as six notes can play simultaneously, depending on the processor used.
A separate open-source project called MIDITONES (https://github.com/lenshustek/miditones) can 
generatae the command sequence from a standard MIDI file.

To try out Playtune, create a directory in your arduino\libraries directory called "Playtune", 
and copy the Playtune.cpp and Playtune.h files to it.  Then create a new sketch using one of the
test programs (test_nano.pde or test_mega.pde), which reference the library by way of "#include <Playtune.h>".

Update on 28 Feb 2011: I fixed a bug that caused it to hang for short delays, and updated the sample scores.

Update on 1 Dec 2011: I added a photo and samples of 6-channel playing on the Arduino MEGA 2560 with just 
a 9V battery and a speaker.

Update on 10 June 2013: I made it compatible with the Arduino V1.0.x development environment.

Update on 5 April 2015: I originally put this up at code.google.org in 2011, but since Google is 
getting out of the source hosting business, I've moved it to GitHub.

/*---------------------------------------------------------------------------------
*
*                              About Playtune
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
*  enough to transcribe scores, but I slowly did quite a few of them.
*
*  Len Shustek, 4 Feb 2011
*
-------------------------------------------------------------------------------------*/
