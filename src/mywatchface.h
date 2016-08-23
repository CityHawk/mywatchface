#include <pebble.h>

static const GPathInfo MINUTE_HAND_POINTS = {
    6, (GPoint []) {
        { -4, 10 },
        { 4, 10 },
        { 4, -58 },
        { 2, -60 },
        {-2, -60},
        {-4, -58}
    }
};

static const GPathInfo HOUR_HAND_POINTS = {
    6, (GPoint []){
        {-4, 10},
        {4, 10},
        {4, -38},
        {2, -40},
        {-2,-40},
        {-4, -38}
    }
};
