#ifndef BRICK_H
#define BRICK_H

#include "raylib.h"

#define MAX_BRICKS 120

typedef struct {
    Rectangle rect;
    Color color;
    bool active;
    int hitPoints;        // 1 = normal brick, higher = tougher
} Brick;

#endif // BRICK_H