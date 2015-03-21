/*
  Circadian.h - A clock synchronized by circadian rhythm.
  Copyright (C) 2011 Andrej Primc <www.fgh.si>.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

 */


#ifndef Circadian_h
#define Circadian_h

#include "Arduino.h"


#define CCTPD 86400L  /* ticks per day */
#define CCTIME(h, m, s) (((h)*3600L+(m)*60L+(s))%CCTPD)
#define CCLOCKMS 43200000L  /* triggers are locked for 12 hours */
#define CCHOUR(t) ((t)/3600)
#define CCMINUTE(t) ((t)/60%60)
#define CCSECOND(t) ((t)%60)


class Circadian
{
public:
  Circadian(byte pin, int treshold);

  void sample();
  void sample(int value);

  boolean inSync();
  boolean inSyncNow();

  long time();
  long timeDawn();
  long timeDusk();

  boolean isIn(long t, long u);  /* t <= time() < u */
  boolean isIn(long n, long t, long u);  /* t <= n < u */

  boolean doTriggers();
  boolean trigger(long t);
  boolean triggerDawn(long t);
  boolean triggerDusk(long t);

  // debug
  long syncDiff();
  int sampleValue();

private:
  byte _lightPin;
  int _treshold;
  int _lightHigh;

  byte _state;
  long _tripTime;
  int _lastSample;

  long _offsetDawn;
  long _offsetDusk;
  long _offset;
  long _lastOffset;

  /* last good */
  long _offsetDawnLG;
  long _offsetDuskLG;
  long _offsetLG;

  long _triggerNow;
  long _triggerLast;

  unsigned long _lastSync;
  unsigned long _lastGoodSync;
  boolean _isInSyncNow;
  boolean _isInSync;
  boolean _updateOffsets;

  long ticks();
  void sync();
};


#endif




