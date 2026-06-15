#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

// ===== TYPE DEFINITIONS =====
typedef struct {
    int width;
    int height;
} Screen;

typedef struct {
    Vector2 pos;
    Vector2 vel;
    int radius;
    float speed;
    float accel;
    float responseMagnitude;
} Ball;

typedef struct {
    Rectangle rect;
    int moveSpeed;
    int score;
    int lives;
} Paddle;

typedef enum {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER,
} GameState;

typedef struct {
    GameState state;
    Ball ball;
    Paddle player;
    int selectedMenuOption;
    int level;
} Game;

void DrawDifficultyScreen(Screen screen, int selectedOption);

const char **GetMenuItems(GameState state, int *outCount) {
    static const char *mainMenuItems[] = { "Start", "Quit" };
    static const char *pauseMenuItems[] = { "Resume", "Main Menu", "Quit" };
    static const char *gameOverMenuItems[] = { "Play Again", "Main Menu", "Quit" };

    switch (state) {
        case MENU:
            *outCount = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);
            return mainMenuItems;
        case PAUSED:
            *outCount = sizeof(pauseMenuItems) / sizeof(pauseMenuItems[0]);
            return pauseMenuItems;
        case GAME_OVER:
            *outCount = sizeof(gameOverMenuItems) / sizeof(gameOverMenuItems[0]);
            return gameOverMenuItems;
        default:
            *outCount = 0;
            return NULL;
    }
}

typedef enum {
    MENU_ACTION_NONE,
    MENU_ACTION_START,
    MENU_ACTION_QUIT,
    MENU_ACTION_RESUME,
    MENU_ACTION_PLAY_AGAIN,
    MENU_ACTION_MAIN_MENU
} MenuAction;

MenuAction GetMenuAction(GameState state, int selectedOption) {
    switch (state) {
        case MENU:
            switch (selectedOption) {
                case 0: return MENU_ACTION_START;
                case 1: return MENU_ACTION_QUIT;
                default: return MENU_ACTION_NONE;
            }
        case PAUSED:
            switch (selectedOption) {
                case 0: return MENU_ACTION_RESUME;
                case 1: return MENU_ACTION_MAIN_MENU;
                case 2: return MENU_ACTION_QUIT;
                default: return MENU_ACTION_NONE;
            }
        case GAME_OVER:
            switch (selectedOption) {
                case 0: return MENU_ACTION_PLAY_AGAIN;
                case 1: return MENU_ACTION_MAIN_MENU;
                case 2: return MENU_ACTION_QUIT;
                default: return MENU_ACTION_NONE;
            }
        default:
            return MENU_ACTION_NONE;
    }
}

int GetCurrentMenuOptionCount(GameState state) {
    int count;
    GetMenuItems(state, &count);
    return count;
}

// ===== INITIALIZATION =====
Ball Ball_Init(Screen screen, float initialSpeed, float accel, float responseMagnitude) {
    Ball ball;
    ball.pos = (Vector2){ screen.width / 2.0f, screen.height / 2.0f };
    ball.vel = (Vector2){ 0.0f, -initialSpeed };
    ball.radius = 8;
    ball.speed = initialSpeed;
    ball.accel = accel;
    ball.responseMagnitude = responseMagnitude;
    return ball;
}

/**
 * Initialize a new paddle.
 * @param x The x-coordinate of the paddle's top-left corner.
 * @param y The y-coordinate of the paddle's top-left corner.
 * @param moveSpeed The speed at which the paddle can move.
 * @return The initialized paddle.
 */
Paddle Paddle_Init(float x, float y, int moveSpeed) {
    Paddle paddle;
    paddle.rect = (Rectangle){ x, y, 120, 20 };
    paddle.moveSpeed = moveSpeed;
    paddle.score = 0;
    paddle.lives = 3;
    return paddle;
}

Game Game_Init(Screen screen) {
    Game game;
    game.state = MENU;
    game.ball = Ball_Init(screen, 6.0f, 1.05f, 8.0f);
    
    int paddleMargin = 25;
    game.player = Paddle_Init(paddleMargin, screen.height / 1.0f - 60, 8);
    game.level = 1;
    game.selectedMenuOption = 0;
    
    return game;
}

// ===== UPDATE =====
int Ball_Update(Ball *ball, Screen screen) {
    ball->pos.x += ball->vel.x;
    ball->pos.y += ball->vel.y;

    // Wall bounce (top/left/right)
    if (ball->pos.x - ball->radius <= 0) { // Left wall
        ball->pos.x = ball->radius;
        ball->vel.x = fabsf(ball->vel.x);
        return 1; // Wall hit
    }
    if (ball->pos.x + ball->radius >= screen.width) { // Right wall
        ball->pos.x = screen.width - ball->radius;
        ball->vel.x = -fabsf(ball->vel.x);
        return 1; // Wall hit
    }
    if (ball->pos.y - ball->radius <= 0) { // Top wall
        ball->pos.y = ball->radius;
        ball->vel.y = fabsf(ball->vel.y);
        return 1; // Wall hit
    }
    if (ball->pos.y + ball->radius >= screen.height) { // Bottom wall
        // loose life, reset ball
        Ball_ResetCenter(ball, screen);
        return 1; // Wall hit
    }
    return 0; // No wall hit
}

void Paddle_UpdatePlayer(Paddle *paddle, Screen screen) {
    if (IsKeyDown(KEY_LEFT)) paddle->rect.x -= paddle->moveSpeed;
    if (IsKeyDown(KEY_RIGHT)) paddle->rect.x += paddle->moveSpeed;

    if (paddle->rect.x < 0) paddle->rect.x = 0;
    if (paddle->rect.x + paddle->rect.width > screen.width)
        paddle->rect.x = screen.width - paddle->rect.width;
}

void Ball_ResetCenter(Ball *ball, Screen screen) {
    ball->pos = (Vector2){ screen.width / 2.0f, screen.height / 2.0f };
}

// ===== COLLISION & SCORING =====
void Ball_CheckPaddleCollision(Ball *ball, Paddle *paddle, const Sound *hitSound, int isPlayerSide) {
    if (CheckCollisionCircleRec(ball->pos, ball->radius, paddle->rect)) {
        float relativeHit = (ball->pos.x - (paddle->rect.x + paddle->rect.width / 2)) / (paddle->rect.width / 2);
        ball->vel.y = fabsf(ball->vel.y) * -1.0f;
        ball->vel.x = relativeHit * ball->responseMagnitude;
        PlaySound(*hitSound);
    }
}

// ===== DRAWING =====
void DrawStar(Vector2 center, float radius, Color color) {
    const int points = 5;
    float innerRadius = radius * 0.5f;
    Vector2 vertices[10];

    for (int i = 0; i < 10; i++) {
        float angle = (-90.0f + i * 36.0f) * DEG2RAD;
        float r = (i % 2 == 0) ? radius : innerRadius;
        vertices[i] = (Vector2){ center.x + cosf(angle) * r, center.y + sinf(angle) * r };
    }

    for (int i = 0; i < points; i++) {
        Vector2 a = vertices[(i * 2) % 10];
        Vector2 b = vertices[(i * 2 + 1) % 10];
        Vector2 c = vertices[(i * 2 + 2) % 10];
        DrawTriangle(a, b, c, color);
    }
}

void DrawGameScene(Game *game, Screen screen, int scoreTextOffsetX) {
    DrawRectangleRec(game->player.rect, WHITE);
    DrawCircleV(game->ball.pos, game->ball.radius, RAYWHITE);
    DrawText(TextFormat("%i", game->player.score), screen.width / 2 - scoreTextOffsetX, 30, 50, WHITE);
}

void DrawPauseScreen(Screen screen, int selectedOption) {
    const char *titleText = "- - PAUSED - -";
    int menuCount;
    const char **menuItems = GetMenuItems(PAUSED, &menuCount);
    const int menuFontSize = 32;
    const int menuSpacing = 50;
    const int menuStartY = 220;

    DrawText(titleText, screen.width / 2 - MeasureText(titleText, 40) / 2, 150, 40, YELLOW);

    for (int i = 0; i < menuCount; i++) {
        int textWidth = MeasureText(menuItems[i], menuFontSize);
        int textX = screen.width / 2 - textWidth / 2;
        int textY = menuStartY + i * menuSpacing;

        if (i == selectedOption) {
            Rectangle highlight = { textX - 16.0f, (float)textY - 8.0f, textWidth + 32.0f, menuFontSize + 16.0f };
            DrawRectangleRec(highlight, Fade(YELLOW, 0.2f));
            DrawRectangleLinesEx(highlight, 2, YELLOW);
            DrawText(menuItems[i], textX, textY, menuFontSize, BLACK);
        } else {
            DrawText(menuItems[i], textX, textY, menuFontSize, WHITE);
        }
    }

    const char *hintText = "Use UP / DOWN or W / S to choose";
    const char *enterText = "Press ENTER to select";
    const char *resumeText = "Press ESC to resume";
    DrawText(hintText, screen.width / 2 - MeasureText(hintText, 20) / 2, screen.height - 90, 20, LIGHTGRAY);
    DrawText(enterText, screen.width / 2 - MeasureText(enterText, 20) / 2, screen.height - 65, 20, LIGHTGRAY);
    DrawText(resumeText, screen.width / 2 - MeasureText(resumeText, 20) / 2, screen.height - 40, 20, LIGHTGRAY);
}

void DrawMainMenuScreen(Screen screen, int selectedOption, int difficultyLevel) {
    const char *titleText = "Breakout!!!";
    int menuCount;
    const char **menuItems = GetMenuItems(MENU, &menuCount);
    const int menuFontSize = 32;
    const int menuSpacing = 50;
    const int menuStartY = 150;

    DrawText(titleText, screen.width / 2 - MeasureText(titleText, 50) / 2, screen.height / 6, 50, YELLOW);

    for (int i = 0; i < menuCount; i++) {
        int textWidth = MeasureText(menuItems[i], menuFontSize);
        int textX = screen.width / 2 - textWidth / 2;
        int textY = menuStartY + i * menuSpacing;

        if (i == selectedOption) {
            Rectangle highlight = { textX - 16.0f, (float)textY - 8.0f, textWidth + 32.0f, menuFontSize + 16.0f };
            DrawRectangleRec(highlight, Fade(YELLOW, 0.2f));
            DrawRectangleLinesEx(highlight, 2, YELLOW);
            DrawText(menuItems[i], textX, textY, menuFontSize, BLACK);
        } else {
            DrawText(menuItems[i], textX, textY, menuFontSize, WHITE);
        }

        if (i == 2) {
            float starRadius = 8.0f;
            float starY = textY + menuFontSize / 2.0f;
            float startX = textX + textWidth + 16.0f + starRadius;
            for (int starIndex = 0; starIndex <= difficultyLevel; starIndex++) {
                Vector2 starCenter = { startX + starIndex * (starRadius * 2.5f), starY };
                DrawStar(starCenter, starRadius, WHITE);
            }
        }
    }

    const char *hintText = "Use UP / DOWN or W / S to choose";
    const char *enterText = "Press ENTER to select";
    DrawText(hintText, screen.width / 2 - MeasureText(hintText, 20) / 2, screen.height - 80, 20, LIGHTGRAY);
    DrawText(enterText, screen.width / 2 - MeasureText(enterText, 20) / 2, screen.height - 50, 20, LIGHTGRAY);
}

void DrawGameOverScreen(Screen screen, const char *winnerText, int selectedOption) {
    int menuCount;
    const char **menuItems = GetMenuItems(GAME_OVER, &menuCount);
    const int menuStartY = 220;
    const int menuSpacing = 50;
    const int menuFontSize = 32;

    DrawText(winnerText, screen.width / 2 - MeasureText(winnerText, 40) / 2, 150, 40, YELLOW);

    for (int i = 0; i < menuCount; i++) {
        int textWidth = MeasureText(menuItems[i], menuFontSize);
        int textX = screen.width / 2 - textWidth / 2;
        int textY = menuStartY + i * menuSpacing;

        if (i == selectedOption) {
            Rectangle highlight = { textX - 16.0f, (float)textY - 8.0f, textWidth + 32.0f, menuFontSize + 16.0f };
            DrawRectangleRec(highlight, Fade(YELLOW, 0.2f));
            DrawRectangleLinesEx(highlight, 2, YELLOW);
            DrawText(menuItems[i], textX, textY, menuFontSize, BLACK);
        } else {
            DrawText(menuItems[i], textX, textY, menuFontSize, WHITE);
        }
    }

    const char *hintText = "Use UP / DOWN or W / S to choose";
    const char *enterText = "Press ENTER to select";
    DrawText(hintText, screen.width / 2 - MeasureText(hintText, 20) / 2, screen.height - 80, 20, LIGHTGRAY);
    DrawText(enterText, screen.width / 2 - MeasureText(enterText, 20) / 2, screen.height - 50, 20, LIGHTGRAY);
}

int main(void) {
    Screen screen = { 800, 450 };

    InitWindow(screen.width, screen.height, "Breakout!!!");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    InitAudioDevice();

    // Load sounds
    const Sound paddleHit = LoadSound("assets/paddle_hit.wav");
    const Sound wallHit = LoadSound("assets/wall_hit.wav");
    const Sound scoreSound = LoadSound("assets/score.wav");

    // Initialize game
    Game game = Game_Init(screen);
    const int scoreTextOffsetX = 60;

    while (!WindowShouldClose()) {
        int shouldExit = 0;

        // === INPUT & LOGIC ===
        if (game.state == MENU || game.state == PAUSED || game.state == GAME_OVER) {
            int menuCount = GetCurrentMenuOptionCount(game.state);
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                game.selectedMenuOption = (game.selectedMenuOption + 1) % menuCount;
            }
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                game.selectedMenuOption = (game.selectedMenuOption - 1 + menuCount) % menuCount;
            }

            if (game.state == PAUSED && IsKeyPressed(KEY_ESCAPE)) {
                game.state = PLAYING;
            }

            if (IsKeyPressed(KEY_ENTER)) {
                MenuAction action = GetMenuAction(game.state, game.selectedMenuOption);
                if (action == MENU_ACTION_QUIT) {
                    shouldExit = 1;
                } else if (action == MENU_ACTION_START) {
                    game.state = PLAYING;
                } else if (action == MENU_ACTION_RESUME) {
                    game.state = PLAYING;
                } else if (action == MENU_ACTION_MAIN_MENU) {
                    game.state = MENU;
                    game.selectedMenuOption = 0;
                } else if (action == MENU_ACTION_PLAY_AGAIN) {
                    game.player.score = 0;
                    game.player = Paddle_Init(30, screen.height / 2.0f - 60, 8);
                    game.state = PLAYING;
                    game.ball = Ball_Init(screen, 6.0f, 1.05f, 8.0f);
                } else if (action == MENU_ACTION_MAIN_MENU) {
                    game.state = MENU;
                    game.selectedMenuOption = 0;
                }
            }

            if (shouldExit) break;
        } else if (game.state == PLAYING) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                game.state = PAUSED;
                game.selectedMenuOption = 0;
            }

            // Update paddles
            Paddle_UpdatePlayer(&game.player, screen);

            // Update ball
            int wallBounce = Ball_Update(&game.ball, screen);
            if (wallBounce) {
                PlaySound(wallHit);
            }

            // Check collisions
            Ball_CheckPaddleCollision(&game.ball, &game.player, &paddleHit, 1);
        }

        // === DRAW ===
        BeginDrawing();
            ClearBackground(DARKBLUE);

            if (game.state == MENU) {
                DrawMainMenuScreen(screen, game.selectedMenuOption, 0);
            }

            if (game.state == PLAYING || game.state == PAUSED) {
                DrawGameScene(&game, screen, scoreTextOffsetX);

                if (game.state == PAUSED) {
                    DrawPauseScreen(screen, game.selectedMenuOption);
                }
            } else if (game.state == GAME_OVER) {
                DrawGameOverScreen(screen, "", game.selectedMenuOption);
            }

        EndDrawing();
    }

    // Cleanup
    UnloadSound(paddleHit);
    UnloadSound(wallHit);
    UnloadSound(scoreSound);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
