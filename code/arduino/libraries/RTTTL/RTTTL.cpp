// *************************************************************
// RTTTL
// This library is modified "https://github.com/MakeLV/Music_Box/blob/master/play_rtttl.ino" 
// Modified by: Lhrod, anphora@gmail.com
// **************************************************************

#include "RTTTL.h"

int notes[]={ 
  0,
  NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4,
  NOTE_C5, NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5,
  NOTE_C6, NOTE_CS6, NOTE_D6, NOTE_DS6, NOTE_E6, NOTE_F6, NOTE_FS6, NOTE_G6, NOTE_GS6, NOTE_A6, NOTE_AS6, NOTE_B6,
  NOTE_C7, NOTE_CS7, NOTE_D7, NOTE_DS7, NOTE_E7, NOTE_F7, NOTE_FS7, NOTE_G7, NOTE_GS7, NOTE_A7, NOTE_AS7, NOTE_B7
};

/****************************************************************************/
RTTTL::RTTTL(int pin)
{
  index = 0;
  tonePin = pin;
  pinMode(tonePin, OUTPUT);
}

/****************************************************************************/
RTTTL::RTTTL(int pin, char *song)
{
  index = 0;
  tonePin = pin;
  pinMode(tonePin, OUTPUT);
  configRtttl(song);
}

/****************************************************************************/
void RTTTL::configRtttl(char *song)
{
  this->song = song;
  // Absolutely no error checking in here

  index = 0;
  default_dur = 4;
  default_oct = 6;
  bpm = 63;

  nextTimeTone = 0;
  
  int num;

  // format: d=N,o=N,b=NNN:
  // find the start (skip name, etc)

  while(song[index] != ':') index++;    // ignore name
  index++;                     		// skip ':'

  // get default duration
  if(song[index] == 'd')
  {
    index++; 
    index++;              // skip "d="
    num = 0;
    while(isdigit(song[index]))
    {
      num = (num * 10) + (song[index++] - '0');
    }
    if(num > 0) default_dur = num;
    index++;                   // skip comma
  }

#ifdef DEBUG
  Serial.print("ddur: "); 
  Serial.println(default_dur, 10);
#endif

  // get default octave
  if(song[index] == 'o')
  {
    index++; 
    index++;              // skip "o="
    num = song[index++] - '0';
    if(num >= 3 && num <=7) default_oct = num;
    index++;                   // skip comma
  }

#ifdef DEBUG
  Serial.print("doct: "); 
  Serial.println(default_oct, 10);
#endif

  // get BPM
  if(song[index] == 'b')
  {
    index++; 
    index++;              // skip "b="
    num = 0;
    while(isdigit(song[index]))
    {
      num = (num * 10) + (song[index++] - '0');
    }
    bpm = num;
    index++;                   // skip colon
  }

#ifdef DEBUG
  Serial.print("bpm: "); 
  Serial.println(bpm, 10);
#endif

  // BPM usually expresses the number of quarter notes per minute
  wholenote = (60 * 1000L / bpm) * 4;  // this is the time for whole note (in milliseconds)

#ifdef DEBUG
  Serial.print("wn: "); 
  Serial.println(wholenote, 10);
#endif

  beginMelody = index;
}

void RTTTL::play(char *song)
{
  configRtttl(song);
  playAtEnd();
}


void RTTTL::playAtEnd()
{
  // now begin note loop
  while(song[index])
  {
     nextNote();
  }
  noTone(tonePin);
}

void RTTTL::playUntil(int lastNote)
{
  lastNote += index;
  // now begin note loop
  while((song[index]) && (index < lastNote))
  {
     nextNote();
  }
  noTone(tonePin);
}

bool RTTTL::isAvailable()
{
	return song[index];
}

bool RTTTL::nextNote()
{
    // first, get note duration, if available
    int num = 0;

    if (millis() >= nextTimeTone)
    {
	noTone(tonePin);
    }
    else
	return false;

    while(isdigit(song[index]))
    {
      num = (num * 10) + (song[index++] - '0');
    }

    long duration = wholenote / ((num) ? num : default_dur);   // we will need to check if we are a dotted note after

    // now get the note
    byte note = 0;

    switch(song[index])
    {
    case 'c':
      note = 1;
      break;
    case 'd':
      note = 3;
      break;
    case 'e':
      note = 5;
      break;
    case 'f':
      note = 6;
      break;
    case 'g':
      note = 8;
      break;
    case 'a':
      note = 10;
      break;
    case 'b':
      note = 12;
      break;
    case 'p':
    default:
      note = 0;
    }
    index++;

    // now, get optional '#' sharp
    if(song[index] == '#')
    {
      note++;
      index++;
    }

    // now, get optional '.' dotted note
    if(song[index] == '.')
    {
      duration += duration/2;
      index++;
    }

    // now, get scale
    if(isdigit(song[index]))
    {
      scale = song[index] - '0';
      index++;
    }
    else
    {
      scale = default_oct;
    }

    scale += OCTAVE_OFFSET;

    if(song[index] == ',')
      index++;       // skip comma for next note (or we may be at the end)

    // now play the note
    if(note)
    {
#ifdef DEBUG
      Serial.print("Playing: ");
      Serial.print(scale, 10); 
      Serial.print(' ');
      Serial.print(note, 10); 
      Serial.print(" (");
      Serial.print(notes[(scale - 4) * 12 + note], 10);
      Serial.print(") ");
      Serial.println(duration, 10);
#endif
      tone(tonePin, notes[(scale - 4) * 12 + note], duration);
    }
#ifdef DEBUG
    else
    {
      Serial.print("Pausing: ");
      Serial.println(duration, 10);
    }
#endif
    nextTimeTone = millis() + duration;
    return true;
}

void RTTTL::rewind()
{
    index = beginMelody;
}
