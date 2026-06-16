#ifndef BRICK_H
#define BRICK_H

#include "raylib.h"

#define MAX_BRICKS 120

typedef struct {
    Rectangle rect;
    Color color;
    bool active;
    int hitPoints;
    bool destroying;
    float destroyTimer;
} Brick;

// Armor Style Definition
typedef struct {
    Color base;
    Color border;
    Color highlight;
    int borderThickness;
} ArmorStyle;

#endif // BRICK_H