#include <pebble.h>

static const GPathInfo MINUTE_HAND_POINTS = {
    6, (GPoint []) {
        { -3, 10 },
        { 3, 10 },
        { 3, -58 },
        { 2, -60 },
        {-2, -60},
        {-3, -58}
    }
};

static const GPathInfo HOUR_HAND_POINTS = {
    6, (GPoint []){
        {-3, 10},
        {3, 10},
        {3, -38},
        {2, -40},
        {-2,-40},
        {-3, -38}
    }
};
