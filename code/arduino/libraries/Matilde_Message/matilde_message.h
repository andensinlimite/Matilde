// *************************************************************
// Matilde Message
// Lhrod: anphora@gmail.com
// Set of codes and utilities for Matilde robot
// **************************************************************

#include "RF24.h"

#define __MATILDE_MESSAGE_H

#define WAITING_RESPONSE_TIME 500
#define RESTART asm("jmp 0x0000")

#define PING 0
#define PONG 2
#define LED_HIGH 1
#define LED_LOW 2
#define FORWARD 3
#define BACKWARD 4
#define STOP 5
#define TURN_LEFT 6
#define TURN_RIGHT 7
#define CHANGE_VEL 8
#define WHAT_VEL 9
#define RESPONSE_VEL 10
#define READ_PIN 11
#define VAL_PIN 12
#define WHAT_COUNTERS 13
#define RESPONSE_COUNTERS 14
#define PLAY_INIC_MUSIC 15
#define WHAT_BATTERY 16
#define RESPONSE_BATTERY 17
#define WHAT_DIRECTIONS 18
#define RESPONSE_DIRECTIONS 19
#define FORWARD_UNTIL 20
#define BACKWARD_UNTIL 21
#define TURN_LEFT_UNTIL 22
#define TURN_RIGHT_UNTIL 23
#define FORWARD_TIME 24
#define BACKWARD_TIME 25
#define TURN_LEFT_TIME 26
#define TURN_RIGHT_TIME 27

#define DEBUG_COUNTER_A 150
#define DEBUG_RESPONSE_A 151

#define NO_COMMAND 255

typedef struct {
  byte idOrig;
  byte idDest;
  byte code;
  int arg;
} Message;

typedef struct {
  byte idOrig;
  byte idDest;
  byte code;
  byte arg[100];
} MessageDataArray;

RF24 radio(49, 53);

Message message;
MessageDataArray messageDataArray;

byte myId;

void inicMatilde(byte localId, byte remoteId)
{
  myId = localId;
  message.idOrig = localId;
  message.idDest = remoteId;
  messageDataArray.idOrig = localId;
  messageDataArray.idDest = remoteId;
}

void printMsg(Message msg)
{
  Serial.print("Message: ");
  Serial.print(msg.idOrig);
  Serial.print(" ");
  Serial.print(msg.idDest);
  Serial.print(" ");
  Serial.print(msg.code);
  Serial.print(" ");
  Serial.println(msg.arg);
}

void printMsgData(MessageDataArray msg)
{
  Serial.print("Message: ");
  Serial.print(msg.idOrig);
  Serial.print(" ");
  Serial.print(msg.idDest);
  Serial.print(" ");
  Serial.print(msg.code);
  Serial.print(" ");
  for (int i=0; i < sizeof(50); i++)
  {
     Serial.print((int)msg.arg[i]);
     Serial.print(" ");
  }
  Serial.println(" ");
}

bool sendMessage(Message message)
{
  radio.stopListening();
  bool ok = radio.write(&message, sizeof(Message));
#ifdef DEBUG
  Serial.println(ok ? "Send ok" : "send failed");
#endif
  radio.startListening();

  return ok;
}

bool sendMessageData(MessageDataArray message)
{
  radio.stopListening();
  bool ok = radio.write(&message, sizeof(MessageDataArray));
#ifdef DEBUG
  Serial.println(ok ? "Send ok" : "send failed");
#endif
  radio.startListening();

  return ok;
}

bool sendSimpleCommand(byte code)
{
  message.code = code;
  return sendMessage(message);
}

bool sendSimpleCommand(byte code, int arg)
{
  message.code = code;
  message.arg = arg;
  return sendMessage(message);
}

bool readMessage(Message *msg)
{
  if (radio.available())
  {
    bool ok = (radio.read(msg, sizeof(Message)));
#ifdef DEBUG
    printMsg(*msg);
#endif
    return (ok && ((*msg).idDest == myId));
  }
#ifdef DEBUG
  else
  {
    Serial.println("No radio available");
  }
#endif
  return false;
}

bool readMessageData(MessageDataArray *msg)
{
  if (radio.available())
  {
    bool ok = (radio.read(msg, sizeof(MessageDataArray)));
#ifdef DEBUG
    printMsgData(*msg);
#endif
    return (ok && ((*msg).idDest == myId));
  }
  return false;
}

bool readWaitedMessage(Message *msg, byte waitedCode)
{
  bool ok;
  radio.startListening();
  unsigned long t = millis() + WAITING_RESPONSE_TIME;
  do
  {
    delay(10);
    ok = readMessage(msg);
  } while (((t) > millis()) && ((*msg).code != waitedCode));
  return (ok && ((*msg).code == waitedCode));
}

bool readWaitedMessageData(MessageDataArray *msg, byte waitedCode)
{
  bool ok;
  radio.startListening();
  unsigned long t = millis() + WAITING_RESPONSE_TIME;
  do
  {
    delay(10);
    ok = readMessageData(msg);
  } while (((t) > millis()) && ((*msg).code != waitedCode));
  return (ok && ((*msg).code == waitedCode));
}

bool ping()
{
  Message response;
  bool ok = sendSimpleCommand(PING);
  
  if (ok)
  {
    ok = readWaitedMessage(&response, PONG);
  }
  return ok && (response.code == PONG);
}
