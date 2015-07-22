#pragma once

#include "pebble.h"

#define NUM_CLOCK_TICKS 11
	
static const GPathInfo MINUTE_HAND_POINTS = {
  4,
  (GPoint []) {
    { -6, 18 },
    { 6, 18 },
    { 4, -68 },
    { -4, -68 }
  }
};

static const GPathInfo HOUR_HAND_POINTS = {
  4, (GPoint []){
    {-6, 18},
    {6, 18},
    {4, -43},
    {-4, -43}
  }
};

static const GPathInfo SECOND_HAND_POINTS = {
  7, (GPoint []){
    {-3, 15},
    {3, 15},
    {3, -45},
    {8, -45},
    {0, -55},
    {-8, -45},
    {-3, -45}
  }
};
