#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "level.h"
#include "brick.h"
#include "common.h"

#define MAX_BRICKS 120

typedef enum { MENU, PLAYING, PAUSED, GAME_OVER } GameState;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    int radius;
    float speed;
    float accel;
    float responseMagnitude;
    bool attached;
} Ball;

typedef struct {
    Rectangle rect;
    int moveSpeed;
    int score;
    int lives;
} Paddle;

typedef struct {
    GameState state;
    Ball ball;
    Paddle player;
    Brick bricks[MAX_BRICKS];
    int brickCount;
    int selectedMenuOption;
    int level;
} Game;

Game Game_Init(Screen screen);
void Game_Update(Game *game, Screen screen);
void Game_Draw(const Game *game);

#endif // GAME_H