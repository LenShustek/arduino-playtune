To try out Playtune, 
 - Create a directory in your arduino\libraries directory called "Playtune", and 
   copy the Playtune.cpp and Playtune.h files to it.  
 - Create a new sketch starting with one of the example programs (nano.ino or mega.ino),
   which reference the library by way of "#include <Playtune.h>".
 - Change the definitions of the output pins, if necessary
 - Use the tools/board (etc.) menu of the Arduino IDE to select your board/cpu/port.
 - Use Sketch/upload to load the code.

I've tested this on a Nano, Mega 2560, Uno, and Micro.

*********************************************************************************************

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
   although not in league with real synthesizers. It currently only supports Teensy.
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

