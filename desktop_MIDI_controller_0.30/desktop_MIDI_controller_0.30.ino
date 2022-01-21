
/***************************************************************************
    Copyright (C) 2020  Daryl Hanlon (ignotus666@hotmail.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

****************************************************************************/
#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>
#include <SPI.h>
#include <MIDI.h>
#include <Mux.h>
#include <AnalogSmooth.h>
#include <ResponsiveAnalogRead.h>

using namespace admux;

//MCU = Atmega 1248P

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Mux mux(admux::Pin(A0, INPUT, PinType::Analog), Pinset(7, 6, 5)); // Pins used by multiplexer.

ResponsiveAnalogRead pot1(0, true, 0.04); //(1st variable defined below, sleep=enabled (true), snapMultiplier)
ResponsiveAnalogRead pot2(0, true, 0.04);
ResponsiveAnalogRead pot3(0, true, 0.04);
ResponsiveAnalogRead pot4(0, true, 0.04);
ResponsiveAnalogRead pot5(0, true, 0.04);
ResponsiveAnalogRead pot6(0, true, 0.04);
ResponsiveAnalogRead pot7(0, true, 0.04);
ResponsiveAnalogRead pot8(0, true, 0.04);
ResponsiveAnalogRead pot9(0, true, 0.04);
ResponsiveAnalogRead pot10(0, true, 0.04);
ResponsiveAnalogRead pot11(0, true, 0.04);
ResponsiveAnalogRead pot12(0, true, 0.04);

long unsigned startCount = 0;     //Start counting time for stuff.

byte ccNote = 0;                  //Control Number sent.

int potVal = 0;                   //Pot value after updating from Resp. Analog Read.

byte activePot = 0;               //Variables for making sure only one pot will send MIDI at a time.
byte newActivePot = 0;
byte oldActivePot = 0;

int activePotTime = 400;          //Time an active pot stops others from sending MIDI.

byte topNoteStart = 30;           //CC number each row starts from.
byte bottomNoteStart = 75;

#define topRowShift analogRead(A5)
#define bottomRowShift analogRead(A6)
#define shiftReset digitalRead(10)

int topRowBank = 0;    //Levels to display on screen. Zero-indexed.
int bottomRowBank = 0;

const byte activityLed = 1;
const byte midiLed = 2;

bool fHasLooped  = false; //For boot LED sequence.

//Battery stuff:
int battPin = 4; //Analog pin to read battery voltage.
AnalogSmooth as400 = AnalogSmooth(400); //Average value of 400 readings (library modified).
float battRead = 0;
int voltagePerc = 0;
long unsigned lastReading = -20000;
long unsigned currentReading = 0;
bool usbOn = false;

MIDI_CREATE_DEFAULT_INSTANCE();

void setup()
{
  pinMode(activityLed, OUTPUT);
  pinMode(midiLed, OUTPUT);
  pinMode(topRowShift, INPUT);
  pinMode(bottomRowShift, INPUT);
  pinMode(shiftReset, INPUT);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println("SSD1306 allocation failed");
    for (;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
  delay(2000);

  //BOOT MESSAGE:
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(8, 0);
  display.print("BT MIDI Controller");
  display.setCursor(60, 13);
  display.print("by");
  display.setCursor(18, 24);
  display.print("Daryl H.  v0.30");
  display.display();
  delay(3000);

  if ( fHasLooped == false )
  {
    for ( int x = 0; x < 2; x++ )
    {
      ledFlash();
    }

    fHasLooped = true;
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  
  //Draw lines to separate spaces on screen:
  display.drawLine(0, 0, 128, 0, WHITE);    //Horizontal top
  display.drawLine(0, 31, 70, 31, WHITE);   //Horizontal bottom
  display.drawLine(0, 16, 70, 16, WHITE);   //Horizontal middle
  display.drawLine(0, 0, 0, 32, WHITE);     //Vertical 1 left
  display.drawLine(10, 0, 10, 32, WHITE);   //Vertical 2 left
  display.drawLine(70, 0, 70, 32, WHITE);   //Vertical 3 middle
  display.drawLine(97, 0, 97, 22, WHITE);   //Vertical 3 right 1
  display.drawLine(127, 0, 127, 22, WHITE); //Vertical 3 right 2
  display.drawLine(70, 22, 128, 22, WHITE); //Horizontal middle
  
  display.setCursor(75, 3);
  display.print("CN:");                     //Control Number
  display.setCursor(102, 3);
  display.print("Val:");                    //Value
  
  topShiftPrint();                          //Print CN ranges for top and bottom rows
  bottomShiftPrint();

  if (analogRead(A4) <= 613)                //If it reads under 613 (3.0v) it's on USB power.
  {
    display.setCursor(72, 25);
    display.print("         ");
    display.setCursor(88, 25);
    display.print("USB");
    display.display();
    usbOn = true;
  }

  MIDI.begin();
}

void loop()
{
  display.setTextSize(1);

  if (millis() - startCount > 4000)         //To clear Control Number and MIDI value.
  {
    display.setCursor(103, 13);
    display.print("   ");
    display.setCursor(74, 13);
    display.print("   ");
    display.display();
  }

  if (millis() - startCount > 5)            //Turn off MIDI LED.
  {
    digitalWrite(midiLed, LOW);
  }

  batteryCheck();

  bankShift();

  //Readings and processing of pots using the Responsive Analog Read library:
  int pot1Val = mux.read(7); pot1.update(pot1Val); pot1.setActivityThreshold(13.0);
  int pot2Val = mux.read(5); pot2.update(pot2Val); pot2.setActivityThreshold(13.0);
  int pot3Val = mux.read(3); pot3.update(pot3Val); pot3.setActivityThreshold(13.0);
  int pot4Val = mux.read(1); pot4.update(pot4Val); pot4.setActivityThreshold(13.0);
  int pot5Val = analogRead(A3); pot5.update(pot5Val); pot5.setActivityThreshold(13.0);
  int pot6Val = analogRead(A1); pot6.update(pot6Val); pot6.setActivityThreshold(13.0);
  int pot7Val = mux.read(6); pot7.update(pot7Val); pot7.setActivityThreshold(13.0);
  int pot8Val = mux.read(4); pot8.update(pot8Val); pot8.setActivityThreshold(13.0);
  int pot9Val = mux.read(2); pot9.update(pot9Val); pot9.setActivityThreshold(13.0);
  int pot10Val = mux.read(0); pot10.update(pot10Val); pot10.setActivityThreshold(13.0);
  int pot11Val = analogRead(A2); pot11.update(pot11Val); pot11.setActivityThreshold(13.0);
  int pot12Val = analogRead(A7); pot12.update(pot12Val); pot12.setActivityThreshold(13.0);

  //Arrays to store states of pots:
  bool pots[12] = {pot1.hasChanged(), pot2.hasChanged(), pot3.hasChanged(), pot4.hasChanged(), pot5.hasChanged(), pot6.hasChanged(),
                   pot7.hasChanged(), pot8.hasChanged(), pot9.hasChanged(), pot10.hasChanged(), pot11.hasChanged(), pot12.hasChanged()
                  };

  int updatedVal[12] = {pot1Val, pot2Val, pot3Val, pot4Val, pot5Val, pot6Val, pot7Val, pot8Val, pot9Val, pot10Val, pot11Val, pot12Val};

  //Read pots:
  for (int p = 0; p < 12; p++)
  {
    if (pots[p] == true)
    {
      newActivePot = p;

      if (millis() > 7000)             //Don't send any MIDI notes at boot.
      {
        potVal = updatedVal[p];

        if (p < 5)
        {
          ccNote = topNoteStart + (topRowBank * 5) + p; //Each row starts at a pre-set MIDI Control Number. Calculate the note each pot sends depending on the bank.
        }

        if (p > 5 && p < 11)
        {
          ccNote = bottomNoteStart + (bottomRowBank * 5) + (p - 6); //Bottom row needs 'p' to be offset by -6 to start again at 0.
        }

        if (p == 11)
        {
          ccNote = 127;     //Same note always sent by pot 12; not affected by banks. For overall volume.
        }

        if (p == 5)
        {
          ccNote = 126;     //Same note always sent by pot 6; for mic volume.
        }

        //Determine whether the same pot is being turned or a new one. Regardless, if activePotTime is over a new one is allowed:
        if (newActivePot == oldActivePot && oldActivePot == activePot || millis() - startCount > activePotTime)
        {
          MIDI.sendControlChange(ccNote, potVal / 8, 1);
          activePot = newActivePot;
          startCount = millis();
          digitalWrite(midiLed, HIGH);
          display.setCursor(103, 13);
          display.print("   ");
          display.setCursor(103, 13);
          display.print(potVal / 8);
          display.setCursor(74, 13);
          display.print("   ");
          display.setCursor(74, 13);
          if ((p < 5) || (p > 5 && p < 11))
          {
            display.print(ccNote);
          }
          else if (p == 5)
          {
            display.print("MIC");
          }
          else
            display.print("VOL");          
            display.display();
        }
      }
    }
  }

  //Only let same pot keep sending messages within activePotTime:
  if (millis() - startCount < activePotTime)
  {
    oldActivePot = activePot;
  }
}

void bankShift()
{
  //Set which ranges of notes will be sent by the pots (top and bottom rows separate):
  if (topRowShift == 1023 && bottomRowShift < 550 && bottomRowShift > 450) //Joystick up. Prevent output with diagonal joystick movements.
  {
    topRowBank++;
    setBanks();
    topShiftPrint();
    ledFlash();
  }

  if (topRowShift == 0 && bottomRowShift < 550 && bottomRowShift > 450) //Joystick down.
  {
    topRowBank--;
    setBanks();
    topShiftPrint();
    ledFlash();
  }

  if (bottomRowShift == 0 && topRowShift < 550 && topRowShift > 450) //Joystick right.
  {
    bottomRowBank++;
    setBanks();
    bottomShiftPrint();
    ledFlash();
  }

  if (bottomRowShift == 1023 && topRowShift < 550 && topRowShift > 450) //Joystick left.
  {
    bottomRowBank--;
    setBanks();
    bottomShiftPrint();
    ledFlash();
  }

  if (shiftReset == HIGH) //Pressing joystick button resets banks back to default.
  {
    topRowBank = 0;
    bottomRowBank = 0;
    topShiftPrint();
    bottomShiftPrint();
    ledFlash();
  }
}

void setBanks()
{
  if (topRowBank > 8)
  {
    topRowBank = 0;
  }

  if (topRowBank < 0)
  {
    topRowBank = 8;
  }

  if (bottomRowBank > 8)
  {
    bottomRowBank = 0;
  }

  if (bottomRowBank < 0)
  {
    bottomRowBank = 8;
  }
}

void topShiftPrint()
{
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(3, 5);
  display.print(topRowBank + 1);
  display.setCursor(14, 5);
  display.print(" (");
  display.print(topNoteStart + (topRowBank * 5));
  display.print("-");
  display.print(topNoteStart + (topRowBank * 5) + 4);
  display.print(")");
  display.display();
}

void bottomShiftPrint()
{
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(3, 20);
  display.print(bottomRowBank + 1);
  display.setCursor(14, 20);
  display.print("         ");
  display.setCursor(14, 20);
  if (bottomNoteStart + (bottomRowBank * 5) >= 100)     //To keep values printed centred.
  {
    display.print("(");
  }
  else
    display.print(" (");
  
  display.print(bottomNoteStart + (bottomRowBank * 5));
  display.print("-");
  display.print(bottomNoteStart + (bottomRowBank * 5) + 4);
  display.print(")");
  display.display();
}

void batteryCheck()
{
  float analogSmooth400 = as400.analogReadSmooth(battPin);
  battRead = analogSmooth400;

  //Print battery voltage every x seconds:
  currentReading = millis();
  if (millis() - lastReading >= 20000 && usbOn == false)
  {
    batteryIndicator();
    lastReading = currentReading;
  }
}

void batteryIndicator()
{
  float battery = battRead;
  float voltage = battery * (5.09 / 1023.0);
  voltagePerc = battRead;
  voltagePerc = map(voltagePerc, 602, 813, 0, 100);
  voltagePerc = constrain(voltagePerc, 0, 100);

  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.setCursor(72, 25);
  display.print("    ");
  display.setCursor(72, 25);
  display.print(voltagePerc);
  display.println("%");
  display.setCursor(98, 25);
  display.println("    ");
  display.setCursor(98, 25);
  display.print(voltage);
  display.println("v");
  display.display();
}

void ledFlash()
{
  digitalWrite(activityLed, HIGH);
  digitalWrite(midiLed, HIGH);
  delay(20);
  digitalWrite(activityLed, LOW);
  digitalWrite(midiLed, LOW);
  delay(70);
  digitalWrite(activityLed, HIGH);
  digitalWrite(midiLed, HIGH);
  delay(20);
  digitalWrite(activityLed, LOW);
  digitalWrite(midiLed, LOW);
  delay(330);
}
