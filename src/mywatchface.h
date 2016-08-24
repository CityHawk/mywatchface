#include <pebble.h>

#define LABELSIZE 16
#define MINLABELOFFSET 10
#define ANGLEDOFFSET 3

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

struct Label {
    char num[3];
    GPoint point; 
};

