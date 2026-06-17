#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "level.h"
#include "brick.h"
#include "common.h"

#define MAX_BRICKS 120
#define MAX_NAME_LENGTH 15
#define MAX_HIGHSCORES  10

typedef enum { MENU, PLAYING, PAUSED, GAME_OVER, ENTER_NAME, HIGH_SCORES } GameState;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    int radius;
    float speed;
    float accel;
    float responseMagnitude;
    bool attached;
    int bounceCount;
} Ball;

typedef struct {
    Rectangle rect;
    float moveSpeed;
    float accel;
    float maxSpeed;
    int lives;
} Paddle;

typedef struct {
    char name[MAX_NAME_LENGTH + 1];
    int score;
} HighScore;

typedef struct {
    HighScore scores[MAX_HIGHSCORES];
    int count;
} HighScoreList;

typedef struct {
    GameState state;
    Ball ball;
    Paddle player;
    Brick bricks[MAX_BRICKS];
    int brickCount;
    int selectedMenuOption;
    int level;
    bool devMode;
    int score;
    int next;
    HighScoreList highScores;
    char playerName[MAX_NAME_LENGTH + 1];
    int nameLength;
    bool newHighScore;
} Game;

typedef enum {
    MENU_ACTION_NONE,
    MENU_ACTION_START,
    MENU_ACTION_QUIT,
    MENU_ACTION_RESUME,
    MENU_ACTION_PLAY_AGAIN,
    MENU_ACTION_MAIN_MENU
} MenuAction;

Game Game_Init(Screen screen);
void Game_Update(Game *game, Screen screen);
void Game_Draw(const Game *game);

#endif // GAME_H