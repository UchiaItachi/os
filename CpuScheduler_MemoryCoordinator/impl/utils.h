//
// Created by itachi on 9/19/17.
//

#pragma once

inline unsigned long long convertNanoSecondsToSeconds(unsigned long long ns)
{
  unsigned long long sec = ((ns / 1000) / 1000) / 10;
  return sec;
}

inline unsigned long long converKiloBytesToMegaBytes(unsigned long long kb)
{
  unsigned long long mb = kb >> 10;
  return mb;
}

inline unsigned long long converMegaBytesToKiloBytes(unsigned long long mb)
{
  unsigned long long kb = mb << 10;
  return kb;
}