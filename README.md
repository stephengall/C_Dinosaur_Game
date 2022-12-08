# Chrome Dinosaur Game on PIC16F877A Microcontroller
Project completed as part of 3rd year Robotics & Intelligent Devices. The following program demonstrates  
the following peripherals. 
 
- Basic I/O
- ADC
- LCD
- UART
- I2C
- Interrupts
- Timers

## Basic I/O  
The speakers are used for sound effects throughout the game. SW1 on the daughterboard of the microcontroller
is used for player input.

## ADC  
The analogue to digital converter is used to sample the LDR (light dependant resistor). The LDR is used 
for difficulty selection before the game starts.

## LCD  
The LCD displays the game, and is used for the difficulty select screen.

## UART  
The UART is used to send data to the computer screen over serial communication. At the end of each game, 
the highscore, current score and some other messages are sent to the computer over PUTTY.

## I2C   
I2C communication is used to send data to and receive data from the in built EEPROM. The highscore is
stored on the EEPROM.

## Interrupts  
The interrupt service routine is used to spawn new obstacles.

## Timers  
The timer1 module on the PIC microcontroller is used to control the rate at which the above interrupts occur.
The place at which the timer resets to dictates how long the timer will take. This value is tuned to be 500ms.
