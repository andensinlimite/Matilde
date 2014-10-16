#include <matilde_message.h>
#include <SPI.h>
#include <RF24.h>
#include "nRF24L01.h"
#include <RTTTL.h>

#define IDLOCAL 1
#define IDREMOTA 0

//#define DEBUG
#define TONE
//#define CORRECCION_TRAYECTORIA

#define LED 13

#define RESET_RADIO
#ifdef RESET_RADIO
#define TIME_TO_RESET_RADIO 5000
#endif

#define PIN_MOTOR_I3 12
#define PIN_MOTOR_I4 13
#define PIN_MOTOR_EB 11

#define PIN_MOTOR_I1 7
#define PIN_MOTOR_I2 8
#define PIN_MOTOR_EA 9

#define PIN_SENSOR_A1 4
#define PIN_SENSOR_A2 5
#define PIN_SENSOR_INTA 3
#define SENSOR_A_INT 1

#define PIN_SENSOR_B1 0
#define PIN_SENSOR_B2 1
#define PIN_SENSOR_INTB 2
#define SENSOR_B_INT 0

#define PIN_TONE 16
#define LAST_NOTE_INIC 33
//#define LAST_NOTE_INIC 0

#define PIN_BATTERY 0

#define DEFAULT_TIME_STOP 1000

const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

enum ModeMovementEnum {
  FREE,
  STEP,
  TIME
};

void setModeMove(ModeMovementEnum nMode);

ModeMovementEnum modeMovement;

int vel = 200;
int velA = vel;
int velB = vel;

#ifdef RESET_RADIO
unsigned long nextTimeChecking;
#endif

int dir = 0;
volatile int rotaryCountA = 0;
volatile int rotaryCountB = 0;
volatile int directionA = 0;
volatile int directionB = 0;
volatile int maxStepsA = 0;
volatile int maxStepsB = 0;
volatile Message actCommand;

#ifdef TONE
RTTTL rtttl(PIN_TONE, "korobyeyniki:d=4,o=5,b=160:e6,8b,8c6,8d6,16e6,16d6,8c6,8b,a,8a,8c6,e6,8d6,8c6,b,8b,8c6,d6,e6,c6,a,2a");
#endif

unsigned long timeToStop;

void setup(void)
{  
  inicMatilde(IDLOCAL, IDREMOTA);
  
  pinMode(PIN_MOTOR_I1, OUTPUT);
  pinMode(PIN_MOTOR_I2, OUTPUT);
  pinMode(PIN_MOTOR_EA, OUTPUT);
  pinMode(PIN_MOTOR_I3, OUTPUT);
  pinMode(PIN_MOTOR_I4, OUTPUT);
  pinMode(PIN_MOTOR_EB, OUTPUT);
  
  pinMode(PIN_SENSOR_A1, INPUT);
  pinMode(PIN_SENSOR_A2, INPUT);
  pinMode(PIN_SENSOR_INTA, INPUT);
  pinMode(PIN_SENSOR_B1, INPUT);
  pinMode(PIN_SENSOR_B2, INPUT);
  pinMode(PIN_SENSOR_INTB, INPUT);
  
  attachInterrupt(SENSOR_A_INT, counterA, CHANGE);
  attachInterrupt(SENSOR_B_INT, counterB, CHANGE);
  modeMovement = FREE;

  actCommand.code = NO_COMMAND;
  // Radio interrupt, 21 pin
  attachInterrupt(2, messageArrived, FALLING);
  
  radio.begin();
  radio.setRetries(15,15);

  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1, pipes[0]);
  radio.startListening();
  
#ifdef DEBUG
  Serial.begin(9600);
  Serial.println("Listener begin..."); 
#endif

#ifdef RESET_RADIO
  nextTimeChecking = millis() + TIME_TO_RESET_RADIO;
#endif

#ifdef TONE
  rtttl.playUntil(LAST_NOTE_INIC);
#endif
}

void loop()
{
  if (actCommand.code != NO_COMMAND)
  {
    procCommand(actCommand);
    actCommand.code = NO_COMMAND;
  }
  
  if ((modeMovement == TIME) && (millis() >= timeToStop))
    para();
  
#ifdef CORRECCION_TRAYECTORIA
  correctionPath();
#endif

#ifdef RESET_RADIO
  if (nextTimeChecking <= millis())
  {
    radio.powerDown();
    delay(200);
    radio.powerUp();
    nextTimeChecking = millis() + TIME_TO_RESET_RADIO;
  }
#endif
}

void procCommand(Message command)
{
  switch (command.code) 
  {
    case PING:
      sendSimpleCommand(PONG);
      break;
    case LED_HIGH:
      digitalWrite(LED, HIGH);
      break;
    case LED_LOW:
      digitalWrite(LED, LOW);
      break;
      
    case FORWARD:
      setModeMove(FREE);
      avanza();      
      break;
    case BACKWARD:
      setModeMove(FREE);
      retrocede();      
      break;
    case TURN_LEFT:
      setModeMove(FREE);
      giraIzquierda();      
      break;
    case TURN_RIGHT:
      setModeMove(FREE);
      giraDerecha();      
      break;
      
    case FORWARD_TIME:
      setModeMove(TIME);
      timeToStop = (command.arg <= 0) ? DEFAULT_TIME_STOP : command.arg;
      timeToStop += millis();
      avanza();      
      break;
    case BACKWARD_TIME:
      setModeMove(TIME);
      timeToStop = (command.arg <= 0) ? DEFAULT_TIME_STOP : command.arg;
      timeToStop += millis();
      retrocede();      
      break;
    case TURN_LEFT_TIME:
      setModeMove(TIME);
      timeToStop = (command.arg <= 0) ? DEFAULT_TIME_STOP : command.arg;
      timeToStop += millis();
      giraIzquierda();
      break;
    case TURN_RIGHT_TIME:
      setModeMove(TIME);
      timeToStop = (command.arg <= 0) ? DEFAULT_TIME_STOP : command.arg;
      timeToStop += millis();
      giraDerecha();    
      break;
    
    case FORWARD_UNTIL:
      setModeMove(STEP);
      fordwardUntil(command.arg);      
      break;
    case BACKWARD_UNTIL:
      setModeMove(STEP);
      backwardUntil(command.arg);      
      break;
    case TURN_LEFT_UNTIL:
      setModeMove(STEP);
      turnLeftUntil(command.arg);      
      break;
    case TURN_RIGHT_UNTIL:
      setModeMove(STEP);
      turnRightUntil(command.arg);
      break;
      
    case STOP:
      para();
      break;
      
    case CHANGE_VEL:
      setVel(command.arg);
      break;
    case WHAT_VEL:
      sendSimpleCommand(RESPONSE_VEL, vel);
      break;
    case READ_PIN:
      sendSimpleCommand(VAL_PIN, digitalRead(command.arg));
      break;
    case WHAT_COUNTERS:
      sendTwoInteger(RESPONSE_COUNTERS, rotaryCountA, rotaryCountB);
      break;
    case WHAT_DIRECTIONS:
      sendTwoInteger(RESPONSE_DIRECTIONS, directionA, directionB);
      break;
    case WHAT_BATTERY:
      sendSimpleCommand(RESPONSE_BATTERY, analogRead(PIN_BATTERY));
      break;
#ifdef TONE
    case PLAY_INIC_MUSIC:
      para();
      rtttl.rewind();
      rtttl.playAtEnd();
      break;
#endif
    default:
      para();
      break;
  }
#ifdef RESET_RADIO
    nextTimeChecking = millis() + TIME_TO_RESET_RADIO;
#endif
}

void sendTwoInteger(byte code, int a, int b)
{
  messageDataArray.code = code;
  int *l;
  l = (int *) messageDataArray.arg;
  *l = a;
  l+=sizeof(int);
  *l = b;
  sendMessageData(messageDataArray);
}

void sendFourInteger(byte code, int a, int b, int c, int d)
{
  messageDataArray.code = code;
  int *l;
  l = (int *) messageDataArray.arg;
  *l = a;
  l+=sizeof(int);
  *l = b;
  l+=sizeof(int);
  *l = c;
  l+=sizeof(int);
  *l = d;
  sendMessageData(messageDataArray);
}

void sendArrayInteger(byte code, int arr[])
{
  messageDataArray.code = code;
  int *l;
  l = (int *) messageDataArray.arg;
  for (int i = 0; i < sizeof(arr); i++)
  {
    *l = arr[i];
    l+=sizeof(int);
  }
  sendMessageData(messageDataArray);
}

void setModeMove(ModeMovementEnum nMode)
{
  if (nMode != modeMovement)
  {
    switch (nMode)
    {
      case TIME:
      case FREE:
        if ((modeMovement == TIME) || (modeMovement == FREE))
          break;
        detachInterrupt(SENSOR_A_INT);
        detachInterrupt(SENSOR_B_INT);
        attachInterrupt(SENSOR_A_INT, counterA, CHANGE);
        attachInterrupt(SENSOR_B_INT, counterB, CHANGE);
        break;
      case STEP:
        detachInterrupt(SENSOR_A_INT);
        detachInterrupt(SENSOR_B_INT);
        attachInterrupt(SENSOR_A_INT, counterAWithStepControl, CHANGE);
        attachInterrupt(SENSOR_B_INT, counterBWithStepControl, CHANGE);
        break;
    }
    modeMovement = nMode;
  }
}

#ifdef CORRECCION_TRAYECTORIA
/*
void correctionPath()
{
  int rotA, rotB;
  if (rotaryCountA == rotaryCountB)
  {
    setVelA(vel);
    setVelB(vel);
  }
  else
  {
    if (dir != 0)
    {
      rotA = rotaryCountA * directionA;
      rotB = rotaryCountB * directionB;
      if (rotA > rotB)
      {
        setVelA(velA - directionA);
        setVelB(velB + directionB);
      }
      else if (rotB > rotA)
      {
        setVelA(velA + directionA);
        setVelB(velB - directionB);
      }
    }
  }
}
*/


void correctionPath()
{
  if (rotaryCountA == rotaryCountB)
  {
    setVelA(vel);
    setVelB(vel);
  }
  else
  {
    if (dir != 0)
    {
      if (rotaryCountA > rotaryCountB)
      {
        setVelA(velA - directionA);
        setVelB(velB + directionB);
      }
      else if (rotaryCountB > rotaryCountA)
      {
        setVelA(velA + directionA);
        setVelB(velB - directionB);
      }
    }
    else if (dir < 0)
    {
      if (rotaryCountA < rotaryCountB)
      {
        setVelA(velA - directionA);
        setVelB(velB + directionB);
      }
      else if (rotaryCountB < rotaryCountA)
      {
        setVelA(velA + directionA);
        setVelB(velB - directionB);
      }
    }
  }
}
#endif

void setVel(byte newVel)
{
  vel = newVel;
  setVelA(newVel);
  setVelB(newVel);
}

void setVelA(byte newVel)
{
  velA = newVel;
  analogWrite(PIN_MOTOR_EA, velA);
}

void setVelB(byte newVel)
{
  velB = newVel;
  analogWrite(PIN_MOTOR_EB, velB);
}

void turnRightUntil(int numSteps)
{
  para();
  rotaryCountA = 0;
  rotaryCountB = 0;
  maxStepsA = -numSteps;
  maxStepsB = numSteps;
  setModeMove(STEP);
  giraDerecha();
}

void giraDerecha()
{
  digitalWrite(PIN_MOTOR_I1, LOW);
  digitalWrite(PIN_MOTOR_I2, HIGH);
  digitalWrite(PIN_MOTOR_I3, HIGH);
  digitalWrite(PIN_MOTOR_I4, LOW);
  analogWrite(PIN_MOTOR_EA, velA);
  analogWrite(PIN_MOTOR_EB, velB);
  dir = 0;
}

void turnLeftUntil(int numSteps)
{
  para();
  rotaryCountA = 0;
  rotaryCountB = 0;
  maxStepsA = numSteps;
  maxStepsB = -numSteps;
  setModeMove(STEP);
  giraIzquierda();
}

void giraIzquierda()
{
  digitalWrite(PIN_MOTOR_I1, HIGH);
  digitalWrite(PIN_MOTOR_I2, LOW);
  digitalWrite(PIN_MOTOR_I3, LOW);
  digitalWrite(PIN_MOTOR_I4, HIGH);
  analogWrite(PIN_MOTOR_EA, velA);
  analogWrite(PIN_MOTOR_EB, velB);
  dir = 0;
}

void fordwardUntil(int numSteps)
{
  para();
  rotaryCountA = 0;
  rotaryCountB = 0;
  maxStepsA = numSteps;
  maxStepsB = numSteps;
  setModeMove(STEP);
  avanza();
}

void avanza()
{
  dir = 1;
  digitalWrite(PIN_MOTOR_I1, HIGH);
  digitalWrite(PIN_MOTOR_I2, LOW);
  digitalWrite(PIN_MOTOR_I3, HIGH);
  digitalWrite(PIN_MOTOR_I4, LOW);
  analogWrite(PIN_MOTOR_EA, velA);
  analogWrite(PIN_MOTOR_EB, velB);
}

void backwardUntil(int numSteps)
{
  para();
  rotaryCountA = 0;
  rotaryCountB = 0;
  maxStepsA = -numSteps;
  maxStepsB = -numSteps;
  setModeMove(STEP);
  retrocede();
}

void retrocede()
{
  dir = -1;
  digitalWrite(PIN_MOTOR_I1, LOW);
  digitalWrite(PIN_MOTOR_I2, HIGH);
  digitalWrite(PIN_MOTOR_I3, LOW);
  digitalWrite(PIN_MOTOR_I4, HIGH);
  analogWrite(PIN_MOTOR_EA, velA);
  analogWrite(PIN_MOTOR_EB, velB);
}

void para()
{
  paraA();
  paraB();
  dir = 0;
}

void paraA()
{
  digitalWrite(PIN_MOTOR_I1, LOW);
  digitalWrite(PIN_MOTOR_I2, LOW);
  analogWrite(PIN_MOTOR_EA, 0);
}

void paraB()
{
  digitalWrite(PIN_MOTOR_I4, LOW);
  digitalWrite(PIN_MOTOR_I3, LOW);
  analogWrite(PIN_MOTOR_EB, 0);
}

void messageArrived()
{
  Message c;
  if (readMessage(&c))
  {
    actCommand.code = c.code;
    actCommand.arg = c.arg;
  }
}


void counterA()
{
  /*boolean up;

  if (digitalRead (PIN_SENSOR_A1))
  {
    countPositiveA1++;
    up = digitalRead (PIN_SENSOR_A2);
    if (up)
      countPositiveA2++;
    else
      countNegativeA2++;
  }
  else
  {
    countNegativeA1++;
    up = !digitalRead (PIN_SENSOR_A2);
    if (!up)
      countPositiveA2++;
    else
      countNegativeA2++;
  }

  if (up)*/
  if (!((digitalRead (PIN_SENSOR_A2)) ^ (digitalRead (PIN_SENSOR_A1))))
  {
    directionA = 1;
    rotaryCountA--;
    
  }
  else
  {
    directionA = -1;
    rotaryCountA++;
  }
}

void counterB()
{
  /*boolean up;

  if (digitalRead (PIN_SENSOR_B1))
  {
    countPositiveB1++;
    up = digitalRead (PIN_SENSOR_B2);
    if (up)
      countPositiveB2++;
    else
      countNegativeB2++;
  }
  else
  {
    countNegativeB1++;
    up = !digitalRead (PIN_SENSOR_B2);
    if (!up)
      countPositiveB2++;
    else
      countNegativeB2++;
  }

  if (up)*/
  if (!((digitalRead (PIN_SENSOR_B2)) ^ (digitalRead (PIN_SENSOR_B1))))
  {
    directionB = 1;
    rotaryCountB++;
  }
  else
  {
    directionB = -1;
    rotaryCountB--;
  }
}

void counterAWithStepControl()
{
  counterA();

  if (maxStepsA > 0)
  {
    if (rotaryCountA >= maxStepsA)
      paraA();
  }
  else
  {
    if (rotaryCountA <= maxStepsA)
      paraA();
  }
}

void counterBWithStepControl()
{
  counterB();
  
  if (maxStepsB > 0)
  {
    if (rotaryCountB >= maxStepsB)
      paraB();
  }
  else
  {
    if (rotaryCountB <= maxStepsB)
      paraB();
  }
}
