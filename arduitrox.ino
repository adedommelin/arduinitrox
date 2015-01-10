/*****************************************************************************
* Arduinitrox - v0.1
*
* Alexandre De Dommelin <adedommelin@tuxz.net>
*
* Credits
* -------
*   Inspired by Lionel Pawlowski's work (https://github.com/eocean/arduitrox)
*
* License
* -------
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*****************************************************************************/
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <LiquidCrystal.h>
#include <RunningAverage.h>


/** Define some constants **/

// Program name
const char PROGRAM_NAME[] = "ArduiNitrox v0.1";

// Operating modes
#define MODE_CALIBRATION_AUTO 0
#define MODE_CALIBRATION_MANU 1
#define MODE_READ             2

// Keypad
#define keyRIGHT  0
#define keyUP     1
#define keyDOWN   2
#define keyLEFT   3
#define keySELECT 4
#define keyNONE   5

// Gas & Calibration stuff
#define CALIBRATION_NEEDED_SAMPLES 500
#define GAIN 0.0078125
float calibgas = 21;



Adafruit_ADS1115 ads;
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
RunningAverage RA(10);


int opmode = MODE_CALIBRATION_AUTO;
int lcd_key = keyNONE;
int cnt = 0;
int samples = 0;
float oldRA;
float sig1, sig;
float relpourc;
float pour;




/**
 * Calculate MOD (Maximum Operating Depth) of the mix for a given PPo2
 * 
 * @param float Oxygen percentage in the mix
 * @param float PPo2
 * @return float Maximum Operating Depth
 */
float calc_max_operating_depth (float percentage, float ppo2 = 1.4) {
  return 10 * ( (ppo2/(percentage/100)) - 1 );
}


/**
 * Identify which key has been pressed
 *
 * @return int keyID
 */
int keypad_get_key()
{
    int adc_key_in = analogRead(0);
    if (adc_key_in > 1000) return keyNONE; // exit fast
    
    if (adc_key_in < 50)   return keyRIGHT;  
    if (adc_key_in < 250)  return keyUP; 
    if (adc_key_in < 450)  return keyDOWN; 
    if (adc_key_in < 650)  return keyLEFT; 
    if (adc_key_in < 850)  return keySELECT;  

    return keyNONE;
}



void setup(void)
{
        Serial.begin(9600);	
	ads.setGain(GAIN_SIXTEEN); // 16x GAIN 1 bit = 0.0078125mV

	ads.begin();
	lcd.begin(16, 2);
	lcd.clear();
	lcd.print(PROGRAM_NAME);
	lcd.setCursor(0, 1);
	lcd.print("");
	delay(3000);
	//oldRA = 0;
}


void loop(void)
{
	int16_t adc0;
	oldRA = RA.getAverage();
	adc0 = ads.readADC_Differential_0_1();
	RA.addValue(adc0);
	cnt += 1;
	sig = (RA.getAverage() - oldRA) / oldRA;
	sig1 = abs(sig);

        lcd_key = keypad_get_key();
 
	if ( lcd_key != keyNONE )
	{
		opmode = MODE_CALIBRATION_MANU;
		samples = 0;
	}

	switch ( opmode )
	{
		case MODE_CALIBRATION_AUTO:
			if ( sig1 < 0.0002 )
			{
				samples += 1;
			}
	                lcd.setCursor(0, 0);
	                lcd.print("CALIBRATION AUTO");

			lcd.setCursor(0, 1);
                        lcd.print("%02: ");
			lcd.print(calibgas, 1);
			
			pour = samples / 5;
                        lcd.print(" | ");
			lcd.print(pour, 0);
			lcd.print("% ");
/*
			if ( (RA.getAverage()*GAIN) < 0.02)
			{
				// No data received, giving up :'(
				lcd.setCursor(0, 0);
				lcd.print("SENSOR FAILURE !");
				for(;;)
					;
			}
*/
			Serial.print("CAL,");
			Serial.print(cnt);
			Serial.print(",");
			Serial.print(RA.getAverage()*GAIN, 4);
			Serial.print(",");
			Serial.println(calibgas);

			if ( samples == CALIBRATION_NEEDED_SAMPLES )
			{
				samples = 0;
				opmode = MODE_READ;
				lcd.clear();
				lcd.print("CALIBRATION OK");
				delay(3000);
				relpourc = 100 * calibgas / RA.getAverage();
			}
			break;


		case MODE_CALIBRATION_MANU:
			lcd.setCursor(0, 0);
			lcd.print("CALIBRATION MANU");
			lcd.setCursor(0, 1);
			lcd.print("%O2 REF: ");
			lcd.setCursor(9, 1);
			lcd.print(calibgas, 1);
			lcd.print("         ");
                        
                        switch ( lcd_key ) {
                          case keyUP:
                            calibgas = (calibgas < 99) ? calibgas+1 : calibgas;
                            delay(200);
                          break;
                        
                          case keyRIGHT:
                            calibgas = (calibgas < 100) ? calibgas+0.5 : calibgas;
                            delay(200);
                          break;
                        
                          case keyDOWN:
                            calibgas = (calibgas > 1) ? calibgas-1 : calibgas;
                            delay(200);
                          break;
                        
                          case keyLEFT:
                            calibgas = (calibgas > 0.5) ? calibgas-0.5 : calibgas;
                            delay(200);
                          break;
                    
                          case keySELECT: // Not sure about this one, should be validated when sensor received
                            opmode = MODE_CALIBRATION_AUTO;
                            delay(200);
                          break;      
                        }
                        
                        
			break;


		case MODE_READ:
			lcd.setCursor(0, 0);
			lcd.print("%O2: ");
			pour = RA.getAverage() * relpourc / 100;
			lcd.print(pour, 1);
			
			if (sig1 < 0.0002)
			{
                          lcd.print("   ");
                          lcd.print("====");
			}


			lcd.setCursor(0, 1);
			lcd.print("MOD: ");
                        lcd.print(calc_max_operating_depth(pour, 1.4), 1);
                        lcd.print("m ");
                        
                        lcd.print(calc_max_operating_depth(pour, 1.6), 1);
                        lcd.print("m ");


/*
			Serial.print("MES,");
			Serial.print(cnt);
			Serial.print(",");
			Serial.print(RA.getAverage()*GAIN, 4);
			Serial.print(",");
			Serial.println(pour, 2);
*/
			delay(200);
			break;
	}
}




