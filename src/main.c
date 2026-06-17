#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../include/game.h"
#include "../include/brick.h"
#include "../include/common.h"

#define SHADOW_OFFSET 8
#define SHADOW_COLOR (Color){ 0, 0, 0, 120 }

void CreateLevel(Brick bricks[], int *brickCount, Screen screen, int level) {
    *brickCount = 0;
    const int bricksPerRow = 12;
    const int brickWidth = (screen.width - 80) / bricksPerRow;  // nice spacing
    const int brickHeight = 24;
    const int startY = 60;
    
    #define MAX_PALETTES 7
    Color palettes[MAX_PALETTES][8] = {
        { MAROON, ORANGE, YELLOW, LIME, SKYBLUE, BLUE, PURPLE, VIOLET },
        { PINK, RED, ORANGE, YELLOW, LIME, SKYBLUE, PURPLE, VIOLET },
        { PINK, BEIGE, SKYBLUE, LIME, VIOLET, ORANGE, GOLD, BROWN },
        { PURPLE, VIOLET, BLUE, DARKPURPLE, DARKGREEN, MAROON, RED, DARKPURPLE },
        { MAROON, RED, ORANGE, GOLD, YELLOW, BROWN, RED, ORANGE },
        { DARKPURPLE, BLUE, SKYBLUE, LIME, DARKGREEN, BLUE, SKYBLUE, DARKBLUE },
        { RED, ORANGE, YELLOW, LIME, SKYBLUE, BLUE, PURPLE, PINK }
    };
    Color *currentPalette = palettes[level % MAX_PALETTES];

    int armoredRows = (level / 2);
    int tripleHPRows = (level >= 8) ? (level - 7) : 0;

    // Clamp
    if (armoredRows > 6) armoredRows = 6;
    if (tripleHPRows > 3) tripleHPRows = 3;

    for (int row = 0; row < 6; row++) {
        int rowHP = 1;
        bool isArmored = false;

        if (level % 2 == 0) { // Even levels: armor from top
            isArmored = (row < armoredRows);
        } else { // Odd levels: armor from bottom
            isArmored = (row >= 6 - armoredRows);
        }

        if (isArmored) {
            if (tripleHPRows > 0 && row < tripleHPRows) {
                rowHP = 3;
            } else {
                rowHP = 2;
            }
        }

        for (int col = 0; col < bricksPerRow; col++) {
            if (*brickCount >= MAX_BRICKS) return;
            
            Brick *b = &bricks[*brickCount];
            b->rect = (Rectangle){
                20 + col * (brickWidth + 4),
                startY + row * (brickHeight + 4),
                brickWidth,
                brickHeight
            };
            b->active = true;
            b->hitPoints = rowHP;
            b->color = currentPalette[row % 6];;
            b->destroying = false;
            b->destroyTimer = 0.0f;
            (*brickCount)++;
        }
    }
}

int CheckBrickCollisions(Ball *ball, Brick bricks[], int brickCount, Sound hitSound) {
    int hitCount = 0;
    
    for (int i = 0; i < brickCount; i++) {
        if (!bricks[i].active) continue;
        
        if (CheckCollisionCircleRec(ball->pos, ball->radius, bricks[i].rect)) {
            bricks[i].hitPoints--;
            ball->bounceCount++;
            
            if (bricks[i].hitPoints <= 0 && !bricks[i].destroying) {
                bricks[i].destroying = true;
                bricks[i].destroyTimer = 0.12f;
            }

            // === PITCH VARIATION BASED ON BRICK HEIGHT ===
            float pitch = 1.0f;
            
            pitch = 0.8f + (bricks[i].rect.y / 400.0f);   // Adjust divisor based on your screen size
            
            // Clamp pitch (recommended range: 0.5 to 2.0)
            if (pitch < 0.6f) pitch = 0.6f;
            if (pitch > 1.8f) pitch = 1.8f;

            SetSoundPitch(hitSound, pitch);
            PlaySound(hitSound);

            // === IMPROVED COLLISION RESPONSE ===
            float overlapLeft   = (ball->pos.x + ball->radius) - bricks[i].rect.x;
            float overlapRight  = (bricks[i].rect.x + bricks[i].rect.width) - (ball->pos.x - ball->radius);
            float overlapTop    = (ball->pos.y + ball->radius) - bricks[i].rect.y;
            float overlapBottom = (bricks[i].rect.y + bricks[i].rect.height) - (ball->pos.y - ball->radius);

            // Find the smallest overlap (this is the side we hit)
            float minOverlap = fminf(fminf(overlapLeft, overlapRight), fminf(overlapTop, overlapBottom));

            if (minOverlap == overlapTop || minOverlap == overlapBottom) {
                // Hit top or bottom of brick → reverse vertical velocity
                ball->vel.y *= -1;
            } else {
                // Hit left or right of brick → reverse horizontal velocity
                ball->vel.x *= -1;
            }

            hitCount++;

            // Push ball out to prevent sticking
            if (minOverlap == overlapLeft)   ball->pos.x -= minOverlap + 1;
            if (minOverlap == overlapRight)  ball->pos.x += minOverlap + 1;
            if (minOverlap == overlapTop)    ball->pos.y -= minOverlap + 1;
            if (minOverlap == overlapBottom) ball->pos.y += minOverlap + 1;
        }
    }
    return hitCount;
}

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
    ball.attached = true;
    ball.bounceCount = 0;
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
    paddle.rect = (Rectangle){ x, y, 80, 16 };
    paddle.moveSpeed = moveSpeed;
    paddle.score = 0;
    paddle.next = 1000;
    paddle.lives = 3;
    return paddle;
}

Game Game_Init(Screen screen) {
    Game game;
    game.state = MENU;
    game.ball = Ball_Init(screen, 6.0f, 1.0f, 6.0f);
    
    int paddleMargin = 25;
    game.player = Paddle_Init(paddleMargin, screen.height / 1.0f - 30, 10);
    game.level = 0;
    game.selectedMenuOption = 0;
    return game;
}

// ===== UPDATE =====
int Ball_Update(Ball *ball, Screen screen, Paddle *paddle, GameState *state, Sound dieSound) {
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
    if (ball->pos.y + ball->radius >= screen.height) {
        // Player lost a life
        paddle->lives--;
        PlaySound(dieSound);
        
        if (paddle->lives <= 0) {
            *state = GAME_OVER;
        } else {
            ball->attached = true; // Re-attach to paddle
        }
        return 1;
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

void UpdateBricks(Game *game, float deltaTime) {
    for (int i = 0; i < game->brickCount; i++) {
        if (game->bricks[i].destroying) {
            game->bricks[i].destroyTimer -= deltaTime;
            
            if (game->bricks[i].destroyTimer <= 0.0f) {
                game->bricks[i].active = false;
                game->bricks[i].destroying = false;
                int row = 6 - (i / 12);
                game->player.score += row * (game->level) + (game->ball.bounceCount);
            }
        }
    }
}

void Ball_ResetCenter(Ball *ball, Screen screen) {
    ball->pos = (Vector2){ screen.width / 2.0f, screen.height / 2.0f };
}

void Ball_AttachToPaddle(Ball *ball, Paddle *paddle) {
    ball->attached = true;
    ball->pos.x = paddle->rect.x + paddle->rect.width / 2.0f;
    ball->pos.y = paddle->rect.y - ball->radius - 2.0f;
}

void Ball_Launch(Ball *ball) {
    if (ball->attached) {
        ball->attached = false;

        // Launch upward at a narrow random angle
        float angle = GetRandomValue(-30, 30) * DEG2RAD; // small angle from vertical
        ball->vel.x = sinf(angle) * ball->speed;
        ball->vel.y = -cosf(angle) * ball->speed;
    }
}

// ===== COLLISION & SCORING =====
void Ball_CheckPaddleCollision(Ball *ball, Paddle *paddle, const Sound *hitSound, int isPlayerSide) {
    if (CheckCollisionCircleRec(ball->pos, ball->radius, paddle->rect)) {
        ball->bounceCount = 0; // reset on paddle hit
        float relativeHit = (ball->pos.x - (paddle->rect.x + paddle->rect.width / 2)) / (paddle->rect.width / 2);
        float maxAngle = 60 * DEG2RAD;
        float angle = relativeHit * maxAngle;

        ball->vel.y = -cosf(angle) * ball->speed;
        ball->vel.x = sinf(angle) * ball->speed;

        PlaySound(*hitSound);
    }
}

// ===== DRAWING =====
void DrawShadow(Rectangle rect, Color shadowColor, int offset) {
    Rectangle shadow = rect;
    shadow.x += offset;
    shadow.y += offset;
    DrawRectangleRec(shadow, shadowColor);
}

void DrawCircleShadow(Vector2 center, float radius, Color shadowColor, int offset) {
    DrawCircleV((Vector2){ center.x + offset, center.y + offset }, radius, shadowColor);
}

void DrawBricks(Brick bricks[], int brickCount) {
    for (int i = 0; i < brickCount; i++) {
        if (!bricks[i].active) continue;

        // Draw a shadow for each active brick
        DrawShadow(bricks[i].rect, SHADOW_COLOR, SHADOW_OFFSET);

        Color baseColor = bricks[i].color;
        int hp = bricks[i].hitPoints;
        float alpha = (float)hp * 85.0f;

        if (bricks[i].destroying) {
            if ((int)(bricks[i].destroyTimer * 60) % 2 == 0) {
                baseColor = WHITE;
                alpha = 100.0f;
            }
        }

        baseColor.a = (unsigned char)alpha;
        Rectangle brick = bricks[i].rect;
        DrawRectangleRec(brick, baseColor);
        DrawRectangle(brick.x + 2, brick.y + 2, brick.width - 4, 4, Fade(WHITE, 0.35f));
        DrawRectangle(brick.x + brick.width - 4, brick.y + 4, 4, brick.height - 8, Fade(BLACK, 0.25f));
        DrawRectangle(brick.x + 4, brick.y + brick.height - 6, brick.width - 8, 4, Fade(BLACK, 0.25f));
        DrawRectangleLinesEx(brick, hp, Fade(WHITE, 0.2f));
    }
}

void DrawDragonSpiralBackground(Screen screen, float time, int level) {
    Vector2 center = { screen.width * 0.5f, screen.height * 0.5f };
    const int steps = 120;
    const int arms = 1 + (level % 4);
    const float baseAngle = time * 0.35f + level * 0.52f;
    const float radiusScale = 7.6f + level * 0.9f;
    const float wobble = 14.0f + (level % 3) * 6.0f;

    Color palette[5] = {
        Fade((Color){ 30, 75, 140, 255 }, 0.14f),
        Fade((Color){ 75, 145, 210, 255 }, 0.12f),
        Fade((Color){ 120, 190, 230, 255 }, 0.10f),
        Fade((Color){ 170, 110, 225, 255 }, 0.09f),
        Fade((Color){ 235, 165, 60, 255 }, 0.08f)
    };

    for (int a = 0; a < arms; a++) {
        Vector2 prev = center;
        float armOffset = a * DEG2RAD * (360.0f / arms);

        for (int i = 0; i < steps; i++) {
            float t = i / (float)steps;
            float angle = i * (0.38f + level * 0.02f) + baseAngle + armOffset;
            float radius = powf((float)i + 1.0f, 0.92f) * radiusScale * (0.8f + a * 0.05f);
            float warp = sinf(i * 0.37f + level * 0.9f) * 0.62f;
            float x = center.x + cosf(angle) * radius + sinf(angle * (1.5f + a * 0.18f)) * wobble * warp;
            float y = center.y + sinf(angle) * radius + cosf(angle * (1.2f + a * 0.14f)) * wobble * warp;
            Vector2 point = { x, y };

            Color lineColor = Fade(palette[(a + i / 24) % 5], 0.07f + t * 0.08f);
            DrawLineEx(prev, point, 1.6f - a * 0.15f, lineColor);

            if (i % 10 == 0) {
                float dotRadius = 1.9f + fabsf(sinf(time * 3.6f + i * 0.18f)) * 1.2f;
                DrawCircleV(point, dotRadius, lineColor);
            }

            prev = point;
        }
    }

    for (int j = 0; j < 7; j++) {
        float ringAngle = time * (0.18f + level * 0.02f) + j * DEG2RAD * 55.0f + level * 0.14f;
        float ringRadius = screen.height * 0.32f + sinf(time * 0.42f + j + level * 0.3f) * 18.0f + level * 3.8f;
        Vector2 ringPoint = { center.x + cosf(ringAngle) * ringRadius, center.y + sinf(ringAngle) * ringRadius };
        DrawCircleV(ringPoint, 8.0f, Fade(WHITE, 0.05f));
        DrawLineEx(center, ringPoint, 0.8f, Fade(WHITE, 0.04f));
    }
}

void DrawGameScene(Game *game, Screen screen, int scoreTextOffsetX) {
    // Draw Shadows first:
    Rectangle playerShadow = game->player.rect;
    playerShadow.x += SHADOW_OFFSET;
    playerShadow.y += SHADOW_OFFSET;
    DrawRectangleRounded(playerShadow, 0.6f, 8, SHADOW_COLOR);
    DrawCircleShadow(game->ball.pos, game->ball.radius, SHADOW_COLOR, SHADOW_OFFSET);

    // Draw EXTRA lives
    for (int i = 1; i < game->player.lives; i++) {
        DrawCircleShadow((Vector2){ 30 + i * 25, 30 }, game->ball.radius, SHADOW_COLOR, SHADOW_OFFSET);
        DrawCircle(30 + i * 25, 30, 8, WHITE);
    }
    DrawText(TextFormat("%i", game->player.score), screen.width / 2 - scoreTextOffsetX, 15, 50, WHITE);
    DrawText(TextFormat("Next: %i", game->player.next), screen.width / 2 + scoreTextOffsetX + 20, 15, 20, WHITE);

    // draw the rest of the game elements
    DrawBricks(game->bricks, game->brickCount);

    // Draw the player's paddle with 3D effect
    Color paddleColor = { 255, 255, 255, 150 };
    Rectangle paddle = game->player.rect;
    
    // Main paddle face
    DrawRectangleRounded(paddle, 0.6f, 8, paddleColor);
    
    // Top highlight (light from above-left)
    Rectangle highlightTop = { paddle.x + 2, paddle.y + 2, paddle.width - 4, 3 };
    DrawRectangleRounded(highlightTop, 0.4f, 4, Fade(WHITE, 0.5f));
    
    // Bottom shadow (opposite side)
    Rectangle shadowBottom = { paddle.x + 2, paddle.y + paddle.height - 5, paddle.width - 4, 3 };
    DrawRectangleRounded(shadowBottom, 0.4f, 4, Fade(BLACK, 0.4f));
    
    // Right side shadow
    Rectangle shadowRight = { paddle.x + paddle.width - 4, paddle.y + 4, 3, paddle.height - 8 };
    DrawRectangleRounded(shadowRight, 0.3f, 2, Fade(BLACK, 0.3f));
    
    // 3D Ball with highlight and shadow
    Color ballBase = RAYWHITE;
    DrawCircleV(game->ball.pos, game->ball.radius, ballBase);
    
    // Highlight (upper-left)
    Vector2 highlightOffset = { game->ball.pos.x - game->ball.radius * 0.35f, game->ball.pos.y - game->ball.radius * 0.35f };
    DrawCircleV(highlightOffset, game->ball.radius * 0.35f, Fade(WHITE, 0.6f));
    
    // Shadow (lower-right)
    Vector2 shadowOffset = { game->ball.pos.x + game->ball.radius * 0.4f, game->ball.pos.y + game->ball.radius * 0.4f };
    DrawCircleV(shadowOffset, game->ball.radius * 0.25f, Fade(DARKGRAY, 0.4f));
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

void DrawMainMenuScreen(Screen screen, int selectedOption) {
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

void gameSetup(Game *game, Screen screen) {
    game->player = Paddle_Init(30, screen.height / 1.0f - 30, 10);
    game->ball = Ball_Init(screen, 6.0f, 1.0f, 6.0f);
    game->level = 1;
    CreateLevel(game->bricks, &game->brickCount, screen, game->level);
    game->state = PLAYING;
}

// ========= Main Game Loop =============
int main(void) {
    Screen screen = { 800, 450 };

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screen.width, screen.height, "Breakout!!!");
    SetWindowMinSize(640, 360);
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    InitAudioDevice();

    // Load sounds
    const Sound selectSound = LoadSound("assets/select.wav");
    const Sound paddleHit = LoadSound("assets/bounce.wav");
    const Sound wallHit = LoadSound("assets/bounce.wav");
    const Sound dieSound = LoadSound("assets/death.wav");
    const Sound brickHit = LoadSound("assets/hit.wav");

    // Initialize game
    Game game = Game_Init(screen);
    const int scoreTextOffsetX = 60;
    int nextInflation = 100;

    while (!WindowShouldClose()) {
        int shouldExit = 0;

        // === INPUT & LOGIC ===
        if (game.state == MENU || game.state == PAUSED || game.state == GAME_OVER) {
            int menuCount = GetCurrentMenuOptionCount(game.state);
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                PlaySound(selectSound);
                game.selectedMenuOption = (game.selectedMenuOption + 1) % menuCount;
            }
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                PlaySound(selectSound);
                game.selectedMenuOption = (game.selectedMenuOption - 1 + menuCount) % menuCount;
            }

            if (game.state == PAUSED && IsKeyPressed(KEY_ESCAPE)) {
                game.state = PLAYING;
            }

            if (IsKeyPressed(KEY_ENTER)) {
                MenuAction action = GetMenuAction(game.state, game.selectedMenuOption);
                game.selectedMenuOption = 0;

                if (action == MENU_ACTION_QUIT) {
                    shouldExit = 1;
                } else if (action == MENU_ACTION_START) {
                    gameSetup(&game, screen);
                } else if (action == MENU_ACTION_RESUME) {
                    game.state = PLAYING;
                } else if (action == MENU_ACTION_MAIN_MENU) {
                    game.state = MENU;
                } else if (action == MENU_ACTION_PLAY_AGAIN) {
                    gameSetup(&game, screen);
                } else if (action == MENU_ACTION_MAIN_MENU) {
                    game.state = MENU;
                }
            }

            if (shouldExit) break;
        } else if (game.state == PLAYING) {
            if (game.player.score >= game.player.next) {
                game.player.lives++;
                game.player.next += 1000 + nextInflation;
                nextInflation = nextInflation * 2;
            }
            // DEV CHEAT FOR TESTING
            if (IsKeyDown(KEY_LEFT_SHIFT) && IsKeyPressed(KEY_F1)) {
                game.devMode = !game.devMode;
            }
            if (game.devMode && IsKeyPressed(KEY_F1)) {
                for (int i = 0; i < game.brickCount; i++) {
                    game.bricks[i].active = false;
                }
            }
            if (game.devMode && IsKeyPressed(KEY_F2)) {
                game.player.rect.width += 100;
            }
            if (game.devMode && IsKeyPressed(KEY_F3)) {
                game.player.rect.width = 80;
            }


            if (IsKeyPressed(KEY_ESCAPE)) {
                game.state = PAUSED;
                game.selectedMenuOption = 0;
            }

            // Update paddles
            Paddle_UpdatePlayer(&game.player, screen);
            UpdateBricks(&game, GetFrameTime());

            // Ball attached to paddle (serve mode)
            if (game.ball.attached) {
                Ball_AttachToPaddle(&game.ball, &game.player);
                
                if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
                    Ball_Launch(&game.ball);
                }
            } 
            else {
                // Normal ball movement
                int wallBounce = Ball_Update(&game.ball, screen, &game.player, &game.state, dieSound);
                if (wallBounce) {
                    PlaySound(wallHit);
                }

                // Paddle collision
                Ball_CheckPaddleCollision(&game.ball, &game.player, &paddleHit, 1);

                // Brick collisions
                CheckBrickCollisions(&game.ball, game.bricks, game.brickCount, brickHit );
            }

            // Check if all bricks are destroyed
            bool allBricksCleared = true;
            for (int i = 0; i < game.brickCount; i++) {
                if (game.bricks[i].active) {
                    allBricksCleared = false;
                    break;
                }
            }

            if (allBricksCleared) {
                // Level complete logic
                game.player.score += 100 * game.level;
                game.level++;
                game.ball.attached = true;
                game.ball.speed = 6.0f + (game.level * 0.25f);
                CreateLevel(game.bricks, &game.brickCount, screen, game.level);
            }
        }

        // === DRAW ===
        BeginDrawing();
            ClearBackground(BLACK);

            float windowScale = fminf((float)GetScreenWidth() / screen.width, (float)GetScreenHeight() / screen.height);
            float renderWidth = screen.width * windowScale;
            float renderHeight = screen.height * windowScale;
            float offsetX = ((float)GetScreenWidth() - renderWidth) * 0.5f;
            float offsetY = ((float)GetScreenHeight() - renderHeight) * 0.5f;

            Camera2D camera = { 0 };
            camera.target = (Vector2){ screen.width * 0.5f, screen.height * 0.5f };
            camera.offset = (Vector2){ offsetX + renderWidth * 0.5f, offsetY + renderHeight * 0.5f };
            camera.zoom = windowScale;
            camera.rotation = 0.0f;

            BeginMode2D(camera);
                ClearBackground((Color){ 5, 5, 60, 255 });
                DrawDragonSpiralBackground(screen, (float)GetTime(), game.level);

                if (game.state == MENU) {
                    DrawMainMenuScreen(screen, game.selectedMenuOption);
                }

                if (game.state == PLAYING || game.state == PAUSED) {
                    DrawGameScene(&game, screen, scoreTextOffsetX);

                    if (game.state == PAUSED) {
                        DrawPauseScreen(screen, game.selectedMenuOption);
                    }
                } else if (game.state == GAME_OVER) {
                    DrawGameOverScreen(screen, "", game.selectedMenuOption);
                }
            EndMode2D();
        EndDrawing();
    }

    // Cleanup
    UnloadSound(paddleHit);
    UnloadSound(wallHit);
    UnloadSound(brickHit);
    UnloadSound(selectSound);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
