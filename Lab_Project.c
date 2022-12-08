/**
* Title:		EE302 Lab Project - Game 	
* Platform:		PICmicro PIC16F877A @ 4 Mhz	
* Compiler:     Microchip XC8
* Written by:	Stephen Gallagher			
*
* Date:			30/11/2022
*
* Function:		Game where the user must jump over obstacles spawning from the 
*               right side. Obstacle timing is determined by timer1.
*               Difficulty is selected using the LDR and the highscore is stored
*               on the EEPROM. UART communication is used to print stats to PUTTY.
*
*/

#include <xc.h>
#include <stdio.h>
#include "ee302lcd.h"
#include "I2C_EE302.h"

// 'C' source line config statements

// CONFIG
#pragma config FOSC = XT        // Oscillator Selection bits (XT oscillator)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable bit (BOR disabled)
#pragma config LVP = OFF        // Low-Voltage (Single-Supply) In-Circuit Serial Programming Enable bit (RB3 is digital I/O, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EEPROM Memory Code Protection bit (Data EEPROM code protection off)
#pragma config WRT = OFF        // Flash Program Memory Write Enable bits (Write protection off; all program memory may be written to by EECON control)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#ifndef _XTAL_FREQ
 // Unless already defined assume 4MHz system frequency
 // This definition is required to calibrate the delay functions, __delay_us() and __delay_ms()
 #define _XTAL_FREQ 4000000
#endif 


// Definitions____________________________________________________________

#define LED1 RD0		//define RD0 as LED1
#define LED2 RD1		//define RD1 as LED2
#define SW1 RB0
#define CLOSED 0
#define oneHzLower 0x3D
#define oneHzHigher 0x5D
#define SPK1 RC2
#define POT RA0

// globals _____________________________________________________
int gLowerTimer = 0xDC;
int gUpperTimer = 0x0B;
int gObstacle = 0; 
int gScore = 0;
int gDiff = 1;
// Prototypes_____________________________________________________________

void setup(void);
void loop(void);
void updateLCD(void);
void gameOver(void);
void sendScoreToEEPROM(int input);
int getScoreFromEEPROM(void);
void sendWord(char * input);
void jumpSound(void);
void sampleADC(void);
void gameOverSound(void);
void sendStats(void);
void changeDiffSound(void);

main(void)
{

	setup();             // Call initialization function

/* super loop */
    for(;;){
        loop();
        //delay depends on game difficulty
        for(int i = 0; i < (4 - gDiff); i++){
            __delay_ms(1000 / 30);
        }

    }
}
void loop()
{
    updateLCD();
}
void updateLCD()
{
    Lcd8_Clear();
    static int airborn = 0; //boolean to check if the play is in the air
    static int airtime = 0; //keeps track of how long player has been airborn
    static int obstaclePos = 15;
    static int prevPressed = 0; 
    int maxAirTime = 8; //dictates how long the player stays in the air
    
    //ensures the player only jumps once from one button press
    if(SW1 == CLOSED && !prevPressed){
        airborn = 1;
        prevPressed = 1;
    }
    if(!(SW1 == CLOSED)) prevPressed = 0;
    
    //draws player in the air
    if(airborn){
        Lcd8_Set_Cursor(1,2);
        Lcd8_Write_Char('D');
        if (!airtime)jumpSound();
        airtime++;
        //makes the player drop after an amount of time
        if(airtime >= maxAirTime){
            airborn = 0;
            airtime = 0;
        }
    }
    else if(!airborn){
        Lcd8_Set_Cursor(2,2);
        Lcd8_Write_Char('D');
    }
    //drawing obstacle
    if(gObstacle && obstaclePos >= 0){
        Lcd8_Set_Cursor(2, obstaclePos);
        Lcd8_Write_Char('O');
        obstaclePos--;
        //if the obstacle reaches the end of the screen, allow another to spawn
        if(obstaclePos <= 0){
            obstaclePos = 15;
            gObstacle = 0;
        }
    }
    //checking for gameOver
    if(obstaclePos == 2 && !airborn) gameOver();
    //incrementing score when player jumps over obstacle
    if(airborn && obstaclePos == 2) gScore++;
    
    //drawing score to screen
    Lcd8_Set_Cursor(1,12);
    unsigned char outString[16];
    sprintf(outString, "%d", gScore);
    Lcd8_Write_String(outString);
}
void setup(void)

{
    /* setup stuff */
    ADCON0 = 0b01010001;
    ADCON1 = 0b00100010;
    TXSTA = 0x24;
    RCSTA = 0x90;
    SPBRG = 0x19;
    TRISA = 0x04;
    TRISB = 0x07;
    TRISC = 0xD8; //setting RC6 & RC7 as inputs

	TRISD = 0x00;			//	setting up LEDs
	PORTD = 0xff;			//	Turn off all LEDs on PORTD
    T1CON = 0x29;           //configuring timer

	INTCON 	= 0b11000000;   

    TMR1H = gUpperTimer; //ensures the timer is correct by setting non zero value
    TMR1L = gLowerTimer;
    PIE1 = 0x01; //turning on timer
    Lcd8_Init();
    Lcd8_Clear();
    i2c_init();
    
    unsigned char outString[16];
    static int restart = 0;
    if(restart){
        //updating EEPROM if new highscore has been reached
        if(gScore > getScoreFromEEPROM()){
            sendScoreToEEPROM(gScore);
        }
        
        sendStats();
        
        //score and highscore screen
        Lcd8_Set_Cursor(1,0);
        Lcd8_Write_String("GAME OVER");
        __delay_ms(3000);
        Lcd8_Clear();
        
        sprintf(outString, "Score = %d", gScore);
        Lcd8_Set_Cursor(1,0);
        Lcd8_Write_String(outString);
        sprintf(outString, "Hi = %d", getScoreFromEEPROM());
        Lcd8_Set_Cursor(2,0);
        Lcd8_Write_String(outString);
        
        gScore = 0;
        
        while(SW1) {} //waiting for user to repress sw1
        Lcd8_Clear();
    }else{
        //initialising highscore in EEPROM
        sendScoreToEEPROM(0);
        
        int prevVal = gDiff;
        unsigned char tempString [16];
    
        while(SW1){
            Lcd8_Set_Cursor(1,0);
            Lcd8_Write_String("Enter Difficulty");

            //ensuring the LCD only updates once the ADC value changes
            while(prevVal == gDiff){if(SW1 == 0) break; sampleADC();}
            Lcd8_Clear();
            //once the game difficulty is updated, update difficulty on screen
            switch(gDiff){
                case(1): sprintf(tempString, "Easy"); break;
                case(2): sprintf(tempString, "Medium"); break;
                case(3): sprintf(tempString, "Hard"); break;
            }

            Lcd8_Set_Cursor(2,0);
            Lcd8_Write_String(tempString);
            prevVal = gDiff;
            
            changeDiffSound();
        }
    }
    __delay_ms(100);
    restart = 1;
}

void gameOver(){
    gameOverSound();
    Lcd8_Clear();
    setup();
}
//sends character array over UART
void sendWord(char * input)
{
    char *pos = input;
    while(*pos)
    {
        while(!TXIF){}
        TXREG = *pos;
        pos++;
    }
}

void sendScoreToEEPROM(int input){
    i2c_start();
    i2c_write(0xA0);
    i2c_write(0x01);
    i2c_write(0x55);

    i2c_write(input);

    i2c_stop();
    __delay_ms(5);

}
int getScoreFromEEPROM(){
    i2c_start();
    i2c_write(0xA0);
    i2c_write(0x01);
    i2c_write(0x55);
    i2c_repStart();
    
    i2c_write(0xA1);
    
    int result = i2c_read(0);
   
    i2c_stop();
    
    return result;
}
/*GAME SOUNDS*/
void jumpSound(void){
    for(int i = 0; i < 50; i++){
        SPK1 ^= 1;
        __delay_ms(2);
    }
    for(int i = 0; i < 75; i++){
        SPK1 ^= 1;
        __delay_ms(1);
    }
}
void gameOverSound(void){
    for(int i = 0; i < 2; i++){
        for(int j = 0; j < 25; j++){
            SPK1 ^= 1;
            __delay_ms(5);
        }
        __delay_ms(10);
    }
}
void changeDiffSound(void){
    for(int i = 0; i < 25; i++){
        SPK1 ^= 1;
        __delay_ms(1);
    }
}
//samples the ADC and normalises ADRESH from 1-3
void sampleADC(void){
    __delay_us(30);
    GO_nDONE = 1;
    while(GO_nDONE != 0) {}
    
    gDiff = (ADRESH * 3) / 255 + 1;
}
//sends stats to computer screen over UART
void sendStats(void){
    char scoreString [32];
    
    sprintf(scoreString, "\n\rGAME OVER \n\r\0");
    sendWord(scoreString);
    
    switch(gDiff){
        case(1) : sprintf(scoreString, "DIFFICULTY: EASY\n\r"); break;
        case(2) : sprintf(scoreString, "DIFFICULTY: MEDIUM\n\r"); break;
        case(3) : sprintf(scoreString, "DIFFICULTY: HARD\n\r"); break;
    }
    sendWord(scoreString);
    
    sprintf(scoreString, "SCORE: %d \n\rHI: %d \n\r", gScore, getScoreFromEEPROM());
    sendWord(scoreString);
    sprintf(scoreString, "PRESS SW1 TO PLAY AGAIN\n\r");
    sendWord(scoreString);
}
/* Interrupt Service Routine */

void __interrupt()
isr(void)                           
{
    TMR1IF = 0;
    //resetting back to correct position in timer register
    TMR1H = gUpperTimer;
    TMR1L = gLowerTimer;
    
    static int numOfInterrupts = 0;
    //a new obstacle is spawned every 10 interrupts
    if(numOfInterrupts == 10){
        gObstacle = 1;  
        numOfInterrupts = 0;
    }
    numOfInterrupts++;
    
}
