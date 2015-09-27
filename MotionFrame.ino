

/*

  motionFrame

  Arduino code for motion detection frame

*/
#include <Adafruit_NeoPixel.h>
#include "pitches.h"

#define ULTRASONIC_FAR   1
#define ULTRASONIC_MEDIUM  2
#define ULTRASONIC_CLOSE 3

#define STATE_CALIBRATING 1
#define STATE_WAITING 2
#define STATE_DETECTED 3

// ultrasonic pins
int echoPin = 8;
int triggerPin = 3;
int lightPin = 6;
int pizeoPin = 11;

Adafruit_NeoPixel statusStrip = Adafruit_NeoPixel(1, lightPin, NEO_GRB + NEO_KHZ800);

// current time in millis
unsigned long time;
unsigned long lastStateChange;
unsigned long lastUltrasonicSensor = 0;
unsigned long sensorReadings[4];
int sensorIndex = 0;
unsigned long startCalibrating = 0;
unsigned long lastDetected = 0;
unsigned long baseReading = 0;

// our state variables
int USSensor = ULTRASONIC_FAR;
int FrameState = STATE_CALIBRATING;

// notes in the melody:
int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};

// Setup routine
void setup() {
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
  Serial.begin(9600);
  for ( int i = 0; i < 4; i++ ) {
    sensorReadings[i] = 0;
  }
  lightCalibrate();
}

// Loop
void loop() {

  // capture the current time
  time = millis();
  if ( startCalibrating == 0 ) {
    startCalibrating = time;
  }

  // first sensors
  doSensors();

  if ( FrameState == STATE_CALIBRATING ) {
     if ( ( time - startCalibrating ) > 2000 ) {
       baseReading = averageReading();
       lightWaiting();
       FrameState = STATE_WAITING;
     }
  }
  else if ( FrameState == STATE_WAITING ) {
    unsigned long avg = averageReading();
    unsigned long diff = baseReading - avg;
    float pct = (diff * 1.0) / (baseReading * 1.0);
    
    Serial.print("avg reading: ");
    Serial.print(avg);
    Serial.print(",");
    Serial.print(diff);
    Serial.print(",");
    Serial.print(pct);
    Serial.println("");
    
    if ( (avg < baseReading) && ( pct > .20 ) ) {
      lightDetected();
      FrameState = STATE_DETECTED; 
      lastDetected = time;
      playMelody();
    }
  }
  else if ( FrameState == STATE_DETECTED ) {
     if ( ( time - lastDetected ) > 5000 ) {
       lightWaiting();
       FrameState = STATE_WAITING;
     } 
  } 
 }

void doSensors() {

  if ( ( time -  lastUltrasonicSensor ) > 250 ) {
    checkUltrasonicSensor();
  }
}

void checkUltrasonicSensor() {

  long duration, distance;

  digitalWrite(triggerPin,LOW);
  delayMicroseconds(2);

  digitalWrite(triggerPin,HIGH);
  delayMicroseconds(10);

  digitalWrite(triggerPin,LOW);
  duration = pulseIn(echoPin,HIGH);
  distance = duration/58.2;

  Serial.print("ultrasonic sensor: ");
  Serial.print(distance);
  Serial.println("");

  if ( distance > 100 ) {
    USSensor = ULTRASONIC_FAR;
  }
  else if ( distance > 10 ) {
    USSensor = ULTRASONIC_MEDIUM;
  }
  else {
    USSensor = ULTRASONIC_CLOSE;
  }
  lastUltrasonicSensor = time;
  
  sensorReadings[sensorIndex] = distance;
  sensorIndex++;
  if ( sensorIndex > 3 ) {
    sensorIndex = 0;
  }

}

unsigned long averageReading() {
  unsigned long total = 0;
  int readings = 0; 
  
  for ( int i = 0; i < 3; i++ ) {
    if ( sensorReadings[i] > 0 ) {
      total += sensorReadings[i];
     readings++; 
    }
  }
  
  return total / readings;
}

void lightCalibrate() {
  statusStrip.begin();
  statusStrip.setPixelColor(0,statusStrip.Color(0, 0, 255));
  statusStrip.setBrightness(128);
  statusStrip.show();
}

void lightWaiting() {
  statusStrip.begin();
  statusStrip.setPixelColor(0,statusStrip.Color(0, 255, 0));
  statusStrip.setBrightness(128);
  statusStrip.show();
}

void lightDetected() {
  statusStrip.begin();
  statusStrip.setPixelColor(0,statusStrip.Color(255, 0, 0));
  statusStrip.setBrightness(128);
  statusStrip.show();
}

void playMelody() {
  // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 8; thisNote++) {

    // to calculate the note duration, take one second
    // divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(pizeoPin, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(pizeoPin);
  }
}


