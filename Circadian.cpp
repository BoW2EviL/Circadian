/*
  Circadian.cpp - A clock synchronized by circadian rhythm.
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

#include "Arduino.h"
#include "Circadian.h"

#define MIN_TRIP_TIME CCTIME(0, 4, 0)  // the time the sample must be stable before the state can be changed
#define MAX_SYNC_DIFF CCTIME(0, 15, 0) // allowed difference between synchronizations

Circadian::Circadian(byte pin, int treshold)
{
  _lightPin = pin;
  _lightHigh = treshold;

  _state = 3;  // 0:night, 1:day, 3:guess
  _tripTime = 0;
  _lastSample = 0;

  _lastSync = 0;
  _lastGoodSync = 0;
  _isInSyncNow = false;
  _isInSync = false;
  _updateOffsets = false;
}


void Circadian::sample()
{
  int a = analogRead(_lightPin);
  sample(a);
}


void Circadian::sample(int value)
{
  long t = ticks();

  // trip on sample
  if((value <= _lightHigh && _lastSample > _lightHigh) || (value > _lightHigh && _lastSample <= _lightHigh))
  {
    _tripTime = t;
  }
  _lastSample = value;

  // handle inner state
  switch(_state)
  {
  case 3:
    // guess state
    if(value > _lightHigh)
    {
      // it's light: assume it's noon
      _state = 1;
      _offsetDawnLG = _offsetDawn = (CCTPD + t - CCTIME(6, 0, 0)) % CCTPD;  // assume dawn was six hours ago
    }
    else
    {
      // it's dark: assume it's midnight
      _state = 0;
      _offsetDawnLG = _offsetDawn = (CCTPD + t + CCTIME(6, 0, 0)) % CCTPD;
    }
    _offsetDuskLG = _offsetDusk = (_offsetDawn + CCTIME(12, 0, 0)) % CCTPD;  // assume dusk is 12 hours after dawn
    sync();
    _isInSyncNow = false;
    _isInSync = false;
    _offsetLG = _offset;
    _lastOffset = _offset;

    _triggerNow = _triggerLast = time();
    break;
  case 0:
    // it's night
    if(value > _lightHigh && (CCTPD + t - _tripTime) % CCTPD >= MIN_TRIP_TIME)
    {
      _state = 1;
      _offsetDawn = _tripTime;
      sync();
    }
    break;
  case 1:
    // it's day
    if(value <= _lightHigh && (CCTPD + t - _tripTime) % CCTPD >= MIN_TRIP_TIME)
    {
      _state = 0;
      _offsetDusk = _tripTime;
      sync();
    }
    break;
  }

  // update last good offsets after midnight
  if(_updateOffsets && isIn(CCTIME(0,0,0), CCTIME(0,15,0)))
  {
    _updateOffsets = false;
    // update offsets
    _offsetDawnLG = _offsetDawn;
    _offsetDuskLG = _offsetDusk;
    _offsetLG = _offset;
  }
}


void Circadian::sync()
{
  _lastOffset = _offset;
  if(_offsetDawn > _offsetDusk)
  {
    _offset = (_offsetDusk + (_offsetDawn - _offsetDusk) / 2) % CCTPD;
  }
  else
  {
    _offset = (_offsetDusk + (_offsetDawn + CCTPD - _offsetDusk) / 2) % CCTPD;
  }
  // are we in sync
  long sd = syncDiff();
  unsigned long m = millis();
  _isInSyncNow = (sd < MAX_SYNC_DIFF || sd > CCTPD - MAX_SYNC_DIFF) && (m - _lastSync < 86400000UL);
  _lastSync = m;
  // update last good sync
  if(_isInSyncNow)
  {
    _lastGoodSync = _lastSync;
  }
  // handle last good offsets
  if(!_isInSync && !_isInSyncNow)
  {
    // update immediately if never in sync
    _offsetDawnLG = _offsetDawn;
    _offsetDuskLG = _offsetDusk;
    _offsetLG = _offset;
  }
  if(_isInSyncNow)
  {
    // set flag to update after midnight
    _updateOffsets = true;
  }
  // is sync ever
  if(m - _lastGoodSync < 86400000UL * 28)
  {
    _isInSync |= _isInSyncNow;
  }
  else
  {
    // forget if not synchronised in 28 days
    _isInSync = false;
  }
}


long Circadian::syncDiff()
{
  return (CCTPD + _offset - _lastOffset) % CCTPD;
}

boolean Circadian::inSyncNow()
{
  return _isInSyncNow;
}

boolean Circadian::inSync()
{
  return _isInSync;
}

boolean Circadian::isIn(long n, long t, long u)  // t <= n < u
{
  t = (CCTPD + t) % CCTPD;
  u = (CCTPD + u) % CCTPD;
  if(t < u)
  {
    return n >= t && n < u;
  }
  else
  {
    return n < u || n >= t;
  }
}

boolean Circadian::isIn(long t, long u)  // t <= time() < u
{
  return isIn(time(), t, u);
}


boolean Circadian::doTriggers()
{
  long t = time();
  if(isIn(t, _triggerNow - CCTPD / 2 + 1, _triggerNow + 1))
  {
    // time hasn't moved forward yet
    return false;
  }
  _triggerLast = _triggerNow;
  _triggerNow = t;
  return true;
}


boolean Circadian::trigger(long t)
{
  return isIn(t, _triggerLast + 1, _triggerNow + 1);
}

boolean Circadian::triggerDawn(long t)
{
  return trigger(timeDawn() + t);
}

boolean Circadian::triggerDusk(long t)
{
  return trigger(timeDusk() + t);
}


long Circadian::ticks()
{
  // internal clock: seconds % seconds_per_day
  static unsigned long old = 0;
  static unsigned long ct = 0;
  unsigned long t = millis();
  ct = (ct + (t - old)) % 86400000UL; // 24 hours in ms
  old = t;
  return ct/1000;
}

// use last good offsets
long Circadian::time()
{
  return (CCTPD + ticks() - _offsetLG) % CCTPD;
}


long Circadian::timeDawn()
{
  return (CCTPD + _offsetDawnLG - _offsetLG) % CCTPD;
}


long Circadian::timeDusk()
{
  return (CCTPD + _offsetDuskLG - _offsetLG) % CCTPD;
}


int Circadian::sampleValue()
{
  return _lastSample;
}







