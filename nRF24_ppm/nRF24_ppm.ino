/*
 ##########################################
 #####   Bayang Tx using arduino +   ######
 #####  nRF24L01 to get PPM output   ######
 ##########################################
 #             by 0zoon0                  #
 #                                        #
 #   Most of this project is derived      #
 #     from bikemike/nrf24_multipro.      #
 #   The PPM code got from                #
 #http://forum.arduino.cc/index.php?topic=309672.0#
 ##########################################
 */

#include "iface_nrf24l01.h"

// uncomment to get some debug info
//#define DEBUG

// ############ Wiring ################
//SPI Comm.pins with nRF24L01
//IF you change pin numbers you'll need to adjust SPI outputs too
#define MOSI_pin  11  
#define SCK_pin   13
#define CE_pin    10  
#define MISO_pin  12 
#define CS_pin    9 

// SPI outputs
#define MOSI_on PORTB |= _BV(3)  
#define MOSI_off PORTB &= ~_BV(3)
#define SCK_on PORTB |= _BV(5)   
#define SCK_off PORTB &= ~_BV(5) 
#define CE_on PORTB |= _BV(2)   
#define CE_off PORTB &= ~_BV(2) 
#define CS_on PORTB |= _BV(1)   
#define CS_off PORTB &= ~_BV(1) 
// SPI input
#define  MISO_on (PINB & _BV(4)) 

#define RF_POWER TX_POWER_158mW 
//TX_POWER_5mW  80 20 158

////////////////////// PPM CONFIGURATION//////////////////////////
#define channel_number 6  //set the number of channels
#define sigPin 2  //set PPM signal output pin on the arduino
#define PPM_FrLen 27000  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PPM_PulseLen 400  //set the pulse length
//////////////////////////////////////////////////////////////////

//TODO: transmitter ID is not really used
uint8_t transmitterID[4];
uint8_t packet[32];
static uint8_t emptyPacketsCount = 160;
int ppm[channel_number];
struct MyData {
  uint16_t throttle;
  uint16_t yaw;
  uint16_t pitch;
  uint16_t roll;
  byte aux1;
  // can add four more auxs
};

MyData data;
MyData tmp;

void setup()
{
    pinMode(MOSI_pin, OUTPUT);
    pinMode(SCK_pin, OUTPUT);
    pinMode(CS_pin, OUTPUT);
    pinMode(CE_pin, OUTPUT);
    pinMode(MISO_pin, INPUT);

#ifdef DEBUG
    Serial.begin(115200);
    Serial.println("starting radio");
#endif

    resetData();
    setupPPM();
    delay(50);
    SPI_Begin(); 
    NRF24L01_Reset();
    NRF24L01_Initialize();
    Bayang_init();
    Bayang_bind();
}

void loop()
{
    // process protocol
    Bayang_recv_packet(&tmp);
    if (tmp.throttle==0 && tmp.yaw==0 && tmp.pitch==0 && tmp.roll==0) {
      emptyPacketsCount++;
      if (emptyPacketsCount >= 160) {
        // signal lost?
        resetData(); //set invalid values so that fc can enter fail safe
        emptyPacketsCount = 160;
      }
    }
    else {
      data = tmp;
      emptyPacketsCount = 0;
      setPPMValuesFromData();
    }
}

void resetData() 
{
  //TODO: check safe values
  // 'safe' values to use when no radio input is detected
  data.throttle = 0;
  data.yaw = 511;
  data.pitch = 511;
  data.roll = 511;
  data.aux1 = 0;
  
  setPPMValuesFromData();
}

void setPPMValuesFromData()
{
  ppm[0] = map(data.roll,     0, 1023, 1000, 2000);  
  ppm[1] = map(data.pitch,    0, 1023, 1000, 2000);
  ppm[3] = map(data.yaw,      0, 1023, 1000, 2000);
  ppm[2] = map(data.throttle, 0, 1023, 1000, 2000);
  ppm[4] = map(data.aux1,     0, 1, 1000, 2000);
  ppm[5] = 1000;
}

void setupPPM() {
  pinMode(sigPin, OUTPUT);
  digitalWrite(sigPin, 0);  //set the PPM signal pin to the default state (off)

  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;

  OCR1A = 100;  // compare match register (not very important, sets the timeout for the first interrupt)
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();
}

#error This line is here to intentionally cause a compile error. Please make sure you set clockMultiplier below as appropriate, then delete this line.
#define clockMultiplier 2 // set this to 2 if you are using a 16MHz arduino, leave as 1 for an 8MHz arduino

ISR(TIMER1_COMPA_vect){
  static boolean state = true;

  TCNT1 = 0;

  if ( state ) {
    //end pulse
    PORTD = PORTD & ~B00000100; // turn pin 2 off. Could also use: digitalWrite(sigPin,0)
    OCR1A = PPM_PulseLen * clockMultiplier;
    state = false;
  }
  else {
    //start pulse
    static byte cur_chan_numb;
    static unsigned int calc_rest;

    PORTD = PORTD | B00000100; // turn pin 2 on. Could also use: digitalWrite(sigPin,1)
    state = true;

    if(cur_chan_numb >= channel_number) {
      cur_chan_numb = 0;
      calc_rest += PPM_PulseLen;
      OCR1A = (PPM_FrLen - calc_rest) * clockMultiplier;
      calc_rest = 0;
    }
    else {
      OCR1A = (ppm[cur_chan_numb] - PPM_PulseLen) * clockMultiplier;
      calc_rest += ppm[cur_chan_numb];
      cur_chan_numb++;
    }     
  }
}


