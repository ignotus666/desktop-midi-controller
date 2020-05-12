# Bluetooth-Pedalboard
MIDI Pedalboard with Bluetooth communication based on an Arduino Mega.
Essentially the same project as in https://github.com/ignotus666/midi-pedalboard but with bluetooth capability added, the code significantly changed to remove thousands of redundant lines and all pots removed. I found that it's not at all practical to have to bend over to turn knobs... 
There is no bluetooth-specific code, just the addition of 2 bluetooth modules and a receiver circuit which is the same MIDI-USB circuit based on the PIC18F2550 that was inside the previous pedalboard, moved to the bluetooth receiver.

The bluetooth modules used are the HC-05; one for the transmitter and another for the receiver. In the pedalboard, the Tx output of the Arduino is fed to the Rx pin of a HC-05 (the emitter) using a voltage divider to bring the 5v down to around 3.3v. The receiver consists of a MIDI-USB circuit that is fed the serial data coming from a second HC-05 module (module Tx -> PIC18F2550 Rx, no voltage divider needed). The pedalboard is powered by an 18650 li-ion battery which has a charge/protection circuit and a DC-DC booster module to output a constant 5v for the Arduino. The battery + terminal is wired to an analog input in order to read its voltage and print the value on the screen, to know when it needs charging. This reading is smoothed using the AnalogSmooth library as otherwise it's quite erratic.

The signal path is as follows:

Arduino Tx -> Rx HC-05 emitter (slave) -> (((BT signal))) -> HC-05 receiver (master) Tx -> Rx PIC18F2550 (MIDI USB) -> PC

In order for the MIDI-bluetooth communication to work, a series of changes need to made to the bluetooth modules, the PIC firmware and the MIDI library.

A small guide is included on what changes need to be made to the bluetooth modules and how to do them. Also included is the modified firmware for the PIC18F2550 in the receiver. I've also included the zipped AnalogSmooth (modified), Arduino_MIDI (modified), marekburiak-ILI9341_due and ResponsiveAnalogRead libraries in a .zip folder so you just have to open that and then import them using the Arduino IDE. The marekburiak-ILI9341_due library folder has the images.h file included so the sketch picks it up. The MIDI library is the same as the standard available from the library manager but with the name changed so updates don't screw it up and revert it to the standard MIDI baud rate, as it's modified to use 115200.

When the pedalboard is powered off, the line going from the battery + terminal to the analog pin should be cut off. I use a 2PDT switch to switch off power and also this line.

Li-ion batteries can be dangerous and should be handled with care. I take NO responsibility for anything bad happening. An alternative is to just use a small power bank to power the pedalboard and then you can leave out all the battery code and circuitry.

FUNCTIONS:

Default mode when booting is 'Preset Mode'. You get 6 presets to choose from using the 'keyPressed[0]' to 'keyPressed[5]' switches. The 'keyPressed[6]' (Bank down) and 'keyPressed[7]' (Bank up) switches scroll through 5 preset banks (more can be added relatively easily; I just don't need that many), each with a set of 6 presets. This mode sends Program Change MIDI messages, each preset with its own note (from 0 to 29, channel 1).

When in preset mode, by pressing the expression pedal + Bank down you can scroll through 3 different functions for the bank change buttons. In default mode they scroll through the pedalboard's banks (no MIDI sent). The next function sets them to send MIDI messages for 'Next Bank' and 'Previous Bank'. The last function sends MIDI messages for 'Next Preset' and 'Previous Preset'.

The 'keyPressed[8]' (Stomp Mode) switch activates 'Stomp Mode'. You get 6 pedals that can be turned on and off using the 'keyPressed[0]' to 'keyPressed[5]' switches. When in 'Stomp Mode', the 'Bank up' and 'Bank down' switches can be used to raise and lower the volume. Each pedal switch sends a CC message that alternates between values 127 and 0, to turn pedals on and off.

The 'keyPressed[9]' (Loop Mode) switch activates 'Loop Mode'. You get 8 loop functions using the 'keyPressed[0]' to 'keyPressed[5]' switches and the 'Bank up' and 'Bank down' switches. These functions are for operating the Sooperlooper software. CC messages are used.

Pressing the 'Stomp Mode' or 'Loop Mode' switch again returns it to 'Preset Mode'.

It is possible to calibrate an expression pedal connected to A0 (change accordingly if using another input). By pressing the expression pedal and 'keyPressed[3]' at the same time it enters calibration mode and you have 5 seconds to move the pedal across its entire range. The min and max levels are stored in eeprom so calibration should only be needed once.

To change preset names, there are 2 arrays of names in the Pedalboard_x.xx_5_banks.ino file, in the global variables. The first array is for the small names printed relative to the button positions; keep these short or abbreviated so they fit. The second array is for the full name of the active preset printed large in the centre of the screen. Names are displayed centred in their position on the screen without the need to enter coordinates.

This video shows most of the functions: https://www.youtube.com/watch?v=eDRC17XOzQg&t=9s. Some aspects of the display have changed since.
