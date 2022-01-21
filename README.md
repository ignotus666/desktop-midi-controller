## Desktop MIDI controller with bluetooth communication
<img src="https://user-images.githubusercontent.com/4263412/150361135-68e5ab0b-eb42-4544-bb77-a443eebeb2da.jpg" width="40%"></img>

Based on an atmega1284P with the arduino bootloader and a 4051 multiplexer to expand the analog inputs. The joystick scrolls through 9 banks of CC numbers independently for the top and bottom rows of potentiometers - up/down for the top row and left/right for the bottom row. Pushing down on the joystick resets all banks to default. Pot 6 on the top row and the pot on the side have fixed numbers for things that should stay the same regardless of the bank (mic/output volume...). The initial CC number for each row is set in the sketch, then each bank will increase it from there by 5.

Information is displayed on an SSD1306 screen. It shows bank number and CC number range for each row, the current pot being used and its MIDI value, and battery charge.

It is not possible to turn more than one pot at a time - this is intentional as they are close together and sometimes I'd accidentally turn an adjacent pot. Once one pot starts turning, there is a small threshold of time after it has stopped before another can be turned.

Bluetooth communication works the same as in my [other controller](https://github.com/ignotus666/Bluetooth-Pedalboard).
