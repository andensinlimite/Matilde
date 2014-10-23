// *************************************************************
// ControllerNeomatilde
// Lhrod: anphora@gmail.com
// Communication bridge for Neomatilde and computer
// **************************************************************

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#define DEBUG
#include <matilde_message.h>

#define IDLOCAL 0
#define IDREMOTA 1

#define RA 11.88
#define RB 14.71
#define RA_DIV_RARB ((RA+RB)/RB)

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

unsigned long powerResetTime = millis() + 10000;

void setup(void)
{
  inicMatilde(IDLOCAL, IDREMOTA);
  
  radio.begin();
  
  radio.setRetries(15,15);

  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1,pipes[1]);

  Serial.begin(9600);
  Serial.println("Waiting command..."); 
}

void loop()
{
  Message response;
  bool ok = false;
  int tmp;
  if (Serial.available())
  {
    int c = Serial.read();
    switch (c) {
      case PING:
        ok=ping();
        break;
      case LED_HIGH:
        ok = sendSimpleCommand(LED_HIGH);
        break;
      case LED_LOW:
        ok = sendSimpleCommand(LED_LOW);
        break;        
      case FORWARD_TIME:
        tmp = Serial.parseInt();
        ok = sendSimpleCommand(FORWARD_TIME, tmp);
        break;
      case BACKWARD_TIME:
        tmp = Serial.parseInt();
        ok = sendSimpleCommand(BACKWARD_TIME, tmp);
        break;
      case TURN_RIGHT_TIME:
        tmp = Serial.parseInt();
        ok = sendSimpleCommand(TURN_RIGHT_TIME, tmp);
        break;
      case TURN_LEFT_TIME:
        tmp = Serial.parseInt();
        ok = sendSimpleCommand(TURN_LEFT_TIME, tmp);
        break;
      case STOP:
        ok = sendSimpleCommand(STOP);
        break;
      case CHANGE_VEL:
        ok = sendSimpleCommand(CHANGE_VEL, Serial.parseInt());
        break;
      case WHAT_VEL:
        ok = sendSimpleCommand(WHAT_VEL);
        readWaitedMessage(&response, RESPONSE_VEL);
        if (ok) {
          Serial.println(response.arg);
        }
        break;
      case READ_PIN:
        ok = sendSimpleCommand(READ_PIN, Serial.parseInt());
        readWaitedMessage(&response, VAL_PIN);
        if (ok) {
          Serial.println(response.arg);
        }
        break;
      case PLAY_INIC_MUSIC:
        ok = sendSimpleCommand(PLAY_INIC_MUSIC);
        break;
      case WHAT_BATTERY:
        ok = sendSimpleCommand(WHAT_BATTERY);
        readWaitedMessage(&response, RESPONSE_BATTERY);
        float k = ((((float)response.arg)/1023.0)*5.0);
        k =  k * RA_DIV_RARB;
        if (ok) {
          Serial.println(k);
        }
        break;
    }
    Serial.println((ok) ? "OK" : "FAILED");
  }
}
