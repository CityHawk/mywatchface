#include <pebble.h>

static const GPathInfo MINUTE_HAND_POINTS = {
  4, (GPoint []) {
    { -3, 10 },
    { 3, 10 },
    { 3, -60 },
      {-3, -60}
  }
};

static const GPathInfo HOUR_HAND_POINTS = {
  4, (GPoint []){
    {-3, 10},
    {3, 10},
    {3, -40},
    {-3,-40}
  }
};
