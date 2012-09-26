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


#include "Circadian.h"

#define MIN_TRIP_TIME CCTIME(0, 4, 0)  // the time the sample must be stable before the state can be changed
#define MAX_SYNC_DIFF CCTIME(0, 15, 0) // allowed difference between synchronizations

Circadian::Circadian(byte pin, int treshold)
{
  lightPin = pin;
  lightHigh = treshold;

  state = 3;  // 0:night, 1:day, 3:guess
  tripTime = 0;
  lastSample = 0;

  lastSync = 0;
  lastGoodSync = 0;
  isInSyncNow = false;
  isInSync = false;
  updateOffsets = false;
}


void Circadian::sample()
{
  int a = analogRead(lightPin);
  sample(a);
}


void Circadian::sample(int value)
{
  long t = ticks();

  // trip on sample
  if((value <= lightHigh && lastSample > lightHigh) || (value > lightHigh && lastSample <= lightHigh))
  {
    tripTime = t;
  }
  lastSample = value;

  // handle inner state
  switch(state)
  {
  case 3:
    // guess state
    if(value > lightHigh)
    {
      // it's light: assume it's noon
      state = 1;
      offsetDawnLG = offsetDawn = (CCTPD + t - CCTIME(6, 0, 0)) % CCTPD;  // assume dawn was six hours ago
    }
    else
    {
      // it's dark: assume it's midnight
      state = 0;
      offsetDawnLG = offsetDawn = (CCTPD + t + CCTIME(6, 0, 0)) % CCTPD;
    }
    offsetDuskLG = offsetDusk = (offsetDawn + CCTIME(12, 0, 0)) % CCTPD;  // assume dusk is 12 hours after dawn
    sync();
    isInSyncNow = false;
    isInSync = false;
    offsetLG = offset;
    lastOffset = offset;

    triggerNow = triggerLast = time();
    break;
  case 0:
    // it's night
    if(value > lightHigh && (CCTPD + t - tripTime) % CCTPD >= MIN_TRIP_TIME)
    {
      state = 1;
      offsetDawn = tripTime;
      sync();
    }
    break;
  case 1:
    // it's day
    if(value <= lightHigh && (CCTPD + t - tripTime) % CCTPD >= MIN_TRIP_TIME)
    {
      state = 0;
      offsetDusk = tripTime;
      sync();
    }
    break;
  }

  // update last good offsets after midnight
  if(updateOffsets && isIn(CCTIME(0,0,0), CCTIME(0,15,0)))
  {
    updateOffsets = false;
    // update offsets
    offsetDawnLG = offsetDawn;
    offsetDuskLG = offsetDusk;
    offsetLG = offset;
  }
}


void Circadian::sync()
{
  lastOffset = offset;
  if(offsetDawn > offsetDusk)
  {
    offset = (offsetDusk + (offsetDawn - offsetDusk) / 2) % CCTPD;
  }
  else
  {
    offset = (offsetDusk + (offsetDawn + CCTPD - offsetDusk) / 2) % CCTPD;
  }
  // are we in sync
  long sd = syncDiff();
  unsigned long m = millis();
  isInSyncNow = (sd < MAX_SYNC_DIFF || sd > CCTPD - MAX_SYNC_DIFF) && (m - lastSync < 86400000UL);
  lastSync = m;
  // update last good sync
  if(isInSyncNow)
  {
    lastGoodSync = lastSync;
  }
  // handle last good offsets
  if(!isInSync && !isInSyncNow)
  {
    // update immediately if never in sync
    offsetDawnLG = offsetDawn;
    offsetDuskLG = offsetDusk;
    offsetLG = offset;
  }
  if(isInSyncNow)
  {
    // set flag to update after midnight
    updateOffsets = true;
  }
  // is sync ever
  if(m - lastGoodSync < 86400000UL * 28)
  {
    isInSync |= isInSyncNow;
  }
  else
  {
    // forget if not synchronised in 28 days
    isInSync = false;
  }
}


long Circadian::syncDiff()
{
  return (CCTPD + offset - lastOffset) % CCTPD;
}

boolean Circadian::inSyncNow()
{
  return isInSyncNow;
}

boolean Circadian::inSync()
{
  return isInSync;
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
  if(isIn(t, triggerNow - CCTPD / 2 + 1, triggerNow + 1))
  {
    // time hasn't moved forward yet
    return false;
  }
  triggerLast = triggerNow;
  triggerNow = t;
  return true;
}


boolean Circadian::trigger(long t)
{
  return isIn(t, triggerLast + 1, triggerNow + 1);
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
  return (CCTPD + ticks() - offsetLG) % CCTPD;
}


long Circadian::timeDawn()
{
  return (CCTPD + offsetDawnLG - offsetLG) % CCTPD;
}


long Circadian::timeDusk()
{
  return (CCTPD + offsetDuskLG - offsetLG) % CCTPD;
}


int Circadian::sampleValue()
{
  return lastSample;
}







