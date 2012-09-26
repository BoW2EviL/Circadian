// report circadian clock's state over serial
//
// assuming the circuit on the link below
// http://www.arduino.cc/playground/Learning/PhotoResistor
//

#include <Circadian.h>

// create a clock on analog pin 0 with dusk threshold 500
Circadian cclock(0, 500);

void setup()
{
  // set up a serial connection
  delay(6000);
  Serial.begin(9600);
  Serial.println("Circadian clock.");
}


void loop()
{
  // sample the sensor
  cclock.sample();

  // report time and state every minute
  if(oncePerMinute())
  {
    long t;
    // time since midnight
    t = cclock.time();
    Serial.print(CCHOUR(t));
    Serial.print(":");
    Serial.print(CCMINUTE(t));
    Serial.print("; ");

    t = cclock.timeDawn();
    // when was dawn
    Serial.print(CCHOUR(t));
    Serial.print(":");
    Serial.print(CCMINUTE(t));
    Serial.print("; ");
    
    // is the clock synchronized
    Serial.print((int)cclock.inSync());
    Serial.print("; ");

    // what was the last sample value
    Serial.print((int)cclock.sampleValue());
    Serial.println();
  }

  // handle triggers, if any
  if(cclock.doTriggers())
  {
    if(cclock.triggerDawn(CCTIME(0,30,0)))
    {
      // do something after dawn
      Serial.println("Trigger at dawn + 0:30.");
    }
    if(cclock.triggerDusk(CCTIME(0,-30,0)))
    {
      // do something before dusk
      Serial.println("Trigger at dusk - 0:30.");
    }
    if(cclock.trigger(CCTIME(12,5,0)))
    {
      // do something at 12:05
      Serial.println("Trigger at 12:5.");
    }
  }

  delay(1000);
}


// helper: return true not more than once per minute
boolean oncePerMinute()
{
  static unsigned long last_t = -60000UL;
  unsigned long t = millis();
  if(t - last_t >= 60000UL)
  {
    last_t = t;
    return true;
  }
  return false;
}



