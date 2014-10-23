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
  MessageDataArray responseData;
  bool ok = false;
  int tmp;
  int *l;
  if (Serial.available())
  {
    int c = Serial.read();
    //char c = Serial.read();
#ifdef DEBUG
#endif
    switch (c) {
      case PING:
        ok=ping();
#ifdef DEBUG
      Serial.println((ok) ? "PONG OK" : "PONG FAILED");
#endif
        break;
      case '1':
        ok = sendSimpleCommand(LED_HIGH);
        break;
      case '2':
        ok = sendSimpleCommand(LED_LOW);
        break;
      case 'c':
        for (int i = 0; i < 10; i++)
        {
          Serial.print(i);
          Serial.print(": ");
          ok = ping();

          Serial.println((ok) ? "PONG OK" : "PONG FAILED");

          if (ok)
            i=10;
        }
        break;
        
      case 'y':
        tmp = Serial.parseInt();
          ok = sendSimpleCommand(FORWARD_TIME, tmp);
        break;
      case 'h':
        tmp = Serial.parseInt();
          ok = sendSimpleCommand(BACKWARD_TIME, tmp);
        break;
      case 'g':
        tmp = Serial.parseInt();
          ok = sendSimpleCommand(TURN_RIGHT_TIME, tmp);
        break;
      case 'j':
        tmp = Serial.parseInt();
        ok = sendSimpleCommand(TURN_LEFT_TIME, tmp);
        break;
        
      case 'f':
        tmp = Serial.parseInt();
        if (tmp != 0)
          ok = sendSimpleCommand(FORWARD_UNTIL, tmp);
        else
          ok = sendSimpleCommand(FORWARD);
        break;
      case 'b':
        tmp = Serial.parseInt();
        if (tmp != 0)
          ok = sendSimpleCommand(BACKWARD_UNTIL, tmp);
        else
          ok = sendSimpleCommand(BACKWARD);
        break;
      case 's':
        ok = sendSimpleCommand(STOP);
        break;
      case 'r':
        tmp = Serial.parseInt();
        if (tmp != 0)
          ok = sendSimpleCommand(TURN_RIGHT_UNTIL, tmp);
        else
          ok = sendSimpleCommand(TURN_RIGHT);
        break;
      case 'l':
        tmp = Serial.parseInt();
        if (tmp != 0)
          ok = sendSimpleCommand(TURN_LEFT_UNTIL, tmp);
        else
          ok = sendSimpleCommand(TURN_LEFT);
        break;
      case 'v':
        ok = sendSimpleCommand(CHANGE_VEL, Serial.parseInt());
        break;
      case 'w':
        ok = sendSimpleCommand(WHAT_VEL);
        readWaitedMessage(&response, RESPONSE_VEL);
        if (ok) {
          Serial.print("Vel: ");
          Serial.println(response.arg);
        }
        break;
      case '-':
        radio.powerDown(); 
        delay(500);
        radio.powerUp();
        ok = true;
        break;
      case 'p':
        ok = sendSimpleCommand(READ_PIN, Serial.parseInt());
        readWaitedMessage(&response, VAL_PIN);
        if (ok) {
          Serial.print("Pin: ");
          Serial.println(response.arg);
        }
        break;
      case 'm':
        ok = sendSimpleCommand(PLAY_INIC_MUSIC);
        break;
      case 'q':
        ok = sendSimpleCommand(WHAT_COUNTERS);
        readWaitedMessageData(&responseData, RESPONSE_COUNTERS);
        if (ok) {
            Serial.print("Counter: ");
            int *l;
            l = (int *)responseData.arg;
            Serial.print(*l);
            l+=sizeof(int);
            Serial.print(" ");
            Serial.print(*l);
            Serial.println(" ");
        }
        break;
      case '9':
        for (int i=0; i < 20; i++)
        {
          ok = sendSimpleCommand(WHAT_COUNTERS);
          readWaitedMessageData(&responseData, RESPONSE_COUNTERS);
          if (ok) {
            Serial.print("Counter: ");
            int *l;
            l = (int *)responseData.arg;
            Serial.print(*l);
            l+=sizeof(int);
            Serial.print(" ");
            Serial.print(*l);
            Serial.println(" ");
          }
          delay(250);
        }
        break;
      case '7':
              ok = sendSimpleCommand(WHAT_DIRECTIONS);
              readWaitedMessageData(&responseData, RESPONSE_DIRECTIONS);
              if (ok) {
                Serial.print("Directions: ");
                l = (int *)responseData.arg;
                Serial.print(*l);
                l+=sizeof(int);
                Serial.print(" ");
                Serial.print(*l);
                Serial.println(" ");
              }
            delay(250);
        break;
      case 'd':
              ok = sendSimpleCommand(DEBUG_COUNTER_A);
              readWaitedMessageData(&responseData, DEBUG_RESPONSE_A);
              if (ok) {
                Serial.print("Counters A: ");
                l = (int *)responseData.arg;
                for (int i = 0; i < 8; i++)
                {
                  Serial.print(*l);
                  l+=sizeof(int);
                  Serial.print(" ");
                }
                Serial.println(" ");
              }
            delay(250);
        break;
      case '8':
        ok = sendSimpleCommand(WHAT_BATTERY);
        readWaitedMessage(&response, RESPONSE_BATTERY);
        float k = ((((float)response.arg)/1023.0)*5.0);
        k =  k * RA_DIV_RARB;
        if (ok) {
          Serial.print("batery: ");
          Serial.print(response.arg);
          Serial.print(": ");
          Serial.print(k);
          Serial.println("v");
        }
        break;
    }
    Serial.println((ok) ? "COMMAND OK" : "COMMAND FAILED");
  }
}
