#include "raylib.h"
#include "game_types.h"
#include "obstacle_system.h"
#include "leaderboard.h"

typedef struct {
    int score;
    float globalSpeed;
    BiomeType biome;
    TimeOfDay timeOfDay;
} GameState;

int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(GAME_WIDTH, GAME_HEIGHT, "Dino Game");
    SetTargetFPS(60);

    RenderTexture2D target = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);

    GameScreen currentScreen = SCREEN_MENU;
    bool exitGame = false; 

    char playerName[20] = "\0";
    int playerLetters = 0;

    // --- VARIABILE PENTRU LEADERBOARD ---
    PlayerScore top5[5] = {0};
    int loadedScoresCount = 0;

    Player dino = { 0 };
    dino.size = (Vector2){ 40, 40 };
    dino.color = GREEN;
    dino.position = (Vector2){ 50, 200 };
    dino.velocity = 0.0f;
    dino.gravity = 0.6f;
    dino.isJumping = true;

    float groundLevel = GAME_HEIGHT - 20;

    Obstacle obstacles[MAX_OBSTACLES] = { 0 };
    float spawnTimer = 0.0f; 

    int score = 0;
    int framesCounter = 0; 
    float globalSpeedMultiplier = 1.0f; 
    bool scoreSaved = false;

    while(!WindowShouldClose() && !exitGame)
    {
        if(IsKeyPressed(KEY_F11)) ToggleFullscreen();

        float scale = (float)GetScreenWidth() / GAME_WIDTH;
        float scaleY = (float)GetScreenHeight() / GAME_HEIGHT;
        if (scaleY < scale) scale = scaleY;

        Vector2 mousePoint = GetMousePosition();
        Vector2 virtualMouse = { 
            (mousePoint.x - (GetScreenWidth() - (GAME_WIDTH * scale)) * 0.5f) / scale,
            (mousePoint.y - (GetScreenHeight() - (GAME_HEIGHT * scale)) * 0.5f) / scale 
        };

        // ==========================================
        // UPDATE LOGIC 
        // ==========================================
        switch(currentScreen) 
        {
            case SCREEN_MENU:
            {
                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= 32) && (key <= 125) && (playerLetters < 15)) {
                        playerName[playerLetters] = (char)key;
                        playerName[playerLetters+1] = '\0';
                        playerLetters++;
                    }
                    key = GetCharPressed();
                }

                if (IsKeyPressed(KEY_BACKSPACE)) {
                    playerLetters--;
                    if (playerLetters < 0) playerLetters = 0;
                    playerName[playerLetters] = '\0';
                }

                Rectangle btnPlay = { GAME_WIDTH/2 - 100, 190, 200, 50 };
                Rectangle btnLeaderboard = { GAME_WIDTH/2 - 100, 260, 200, 50 };
                Rectangle btnExit = { GAME_WIDTH/2 - 100, 330, 200, 50 };

                if (CheckCollisionPointRec(virtualMouse, btnPlay) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && playerLetters > 0) {
                    score = 0;
                    framesCounter = 0;
                    globalSpeedMultiplier = 1.0f;
                    scoreSaved = false;
                    spawnTimer = 0.0f;
                    dino.position.y = groundLevel - dino.size.y;
                    dino.velocity = 0;
                    dino.isJumping = false;
                    ResetObstacles(obstacles, MAX_OBSTACLES);
                    currentScreen = SCREEN_PLAYING;
                }
                
                // --- NOU: Click pe Leaderboard ---
                if (CheckCollisionPointRec(virtualMouse, btnLeaderboard) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    loadedScoresCount = LoadTopScores(top5, 5); // Citim din CSV fix cand dam click
                    currentScreen = SCREEN_LEADERBOARD;
                }

                if (CheckCollisionPointRec(virtualMouse, btnExit) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    exitGame = true;
                }
            } break;

            case SCREEN_PLAYING:
            {
                dino.velocity += dino.gravity;
                dino.position.y += dino.velocity;

                if (dino.position.y + dino.size.y >= groundLevel) {
                    dino.position.y = groundLevel - dino.size.y; 
                    dino.velocity = 0.0f;                       
                    dino.isJumping = false;                      
                }

                if(IsKeyPressed(KEY_SPACE) && !dino.isJumping) {
                    dino.velocity = -12.0f;
                    dino.isJumping = true;
                }

                framesCounter++;
                if (framesCounter >= 10) {
                    score += 1;
                    framesCounter = 0;
                    globalSpeedMultiplier = 1.0f + (score / 100) * 0.1f; 
                }

                UpdateObstacleSpawner(obstacles, MAX_OBSTACLES, &spawnTimer, groundLevel, globalSpeedMultiplier);
                Rectangle dinoRec = { dino.position.x, dino.position.y, dino.size.x, dino.size.y };

                if (UpdateObstaclesAndCheckCollision(obstacles, MAX_OBSTACLES, globalSpeedMultiplier, dinoRec)) {
                    currentScreen = SCREEN_GAME_OVER; 
                    if (!scoreSaved) {
                        SaveScoreToCSV(playerName, score); 
                        scoreSaved = true;
                    }
                }
            } break;

            case SCREEN_GAME_OVER:
            {
                if (IsKeyPressed(KEY_SPACE)) currentScreen = SCREEN_MENU;
            } break;

            case SCREEN_LEADERBOARD:
            {
                // Buton de inapoi
                Rectangle btnBack = { GAME_WIDTH/2 - 100, 360, 200, 50 };
                if (CheckCollisionPointRec(virtualMouse, btnBack) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    currentScreen = SCREEN_MENU;
                }
            } break;
        }

        // ==========================================
        // DRAW LOGIC 
        // ==========================================
        BeginTextureMode(target);
            ClearBackground(RAYWHITE);

            if (currentScreen == SCREEN_MENU) {
                DrawText("DINO RUN", GAME_WIDTH/2 - MeasureText("DINO RUN", 60)/2, 25, 60, MAROON);
                
                DrawText("Numele tau:", GAME_WIDTH/2 - 100, 100, 20, DARKGRAY);
                Rectangle nameBox = { GAME_WIDTH/2 - 100, 125, 200, 40 };
                DrawRectangleRec(nameBox, LIGHTGRAY);
                DrawRectangleLinesEx(nameBox, 2, DARKGRAY); 
                DrawText(playerName, nameBox.x + 10, nameBox.y + 10, 20, BLACK);
                
                if ((int)(GetTime() * 2.0) % 2 == 0 && playerLetters < 15) {
                    DrawText("_", nameBox.x + 10 + MeasureText(playerName, 20), nameBox.y + 10, 20, DARKGRAY);
                }

                Rectangle btnPlay = { GAME_WIDTH/2 - 100, 190, 200, 50 };
                if (playerLetters == 0) {
                    DrawRectangleRec(btnPlay, DARKGRAY);
                    DrawText("PLAY", btnPlay.x + 70, btnPlay.y + 15, 20, GRAY);
                    DrawText("Introdu numele pt. a juca!", GAME_WIDTH/2 - 110, 245, 15, RED);
                } else {
                    DrawRectangleRec(btnPlay, CheckCollisionPointRec(virtualMouse, btnPlay) ? LIGHTGRAY : GRAY);
                    DrawText("PLAY", btnPlay.x + 70, btnPlay.y + 15, 20, BLACK);
                }

                // Butonul Leaderboard acum este activ!
                Rectangle btnLeaderboard = { GAME_WIDTH/2 - 100, 260, 200, 50 };
                DrawRectangleRec(btnLeaderboard, CheckCollisionPointRec(virtualMouse, btnLeaderboard) ? LIGHTGRAY : GRAY);
                DrawText("LEADERBOARD", btnLeaderboard.x + 25, btnLeaderboard.y + 15, 20, BLACK);

                Rectangle btnExit = { GAME_WIDTH/2 - 100, 330, 200, 50 };
                DrawRectangleRec(btnExit, CheckCollisionPointRec(virtualMouse, btnExit) ? LIGHTGRAY : GRAY);
                DrawText("EXIT", btnExit.x + 75, btnExit.y + 15, 20, BLACK);

            } 
            else if (currentScreen == SCREEN_PLAYING) {
                DrawLine(0, groundLevel, GAME_WIDTH, groundLevel, BLACK);
                DrawRectangleV(dino.position, dino.size, dino.color);
                DrawObstacles(obstacles, MAX_OBSTACLES);
                DrawText(TextFormat("SCORE: %05i", score), GAME_WIDTH - 150, 20, 20, DARKGRAY);
            }
            else if (currentScreen == SCREEN_GAME_OVER) {
                DrawText("GAME OVER!", GAME_WIDTH/2 - MeasureText("GAME OVER!", 50)/2, 150, 50, RED);
                DrawText(TextFormat("FINAL SCORE: %i", score), GAME_WIDTH/2 - 100, 220, 30, DARKGRAY);
                DrawText(TextFormat("Saved for: %s", playerName), GAME_WIDTH/2 - MeasureText(TextFormat("Saved for: %s", playerName), 20)/2, 260, 20, GRAY);
                DrawText("Press SPACE to return to Menu", GAME_WIDTH/2 - 170, 300, 20, GRAY);
            }
            else if (currentScreen == SCREEN_LEADERBOARD) {
                // --- DESENAM TOP 5 ---
                DrawText("TOP 5 JUCATORI", GAME_WIDTH/2 - MeasureText("TOP 5 JUCATORI", 40)/2, 40, 40, ORANGE);
                
                if (loadedScoresCount == 0) {
                    DrawText("Inca nu exista scoruri salvate!", GAME_WIDTH/2 - MeasureText("Inca nu exista scoruri salvate!", 20)/2, 200, 20, DARKGRAY);
                } else {
                    for (int i = 0; i < loadedScoresCount; i++) {
                        DrawText(TextFormat("%d. %s", i + 1, top5[i].name), GAME_WIDTH/2 - 150, 120 + (i * 40), 20, DARKBLUE);
                        DrawText(TextFormat("%d pct", top5[i].score), GAME_WIDTH/2 + 50, 120 + (i * 40), 20, BLACK);
                    }
                }

                // Buton Inapoi
                Rectangle btnBack = { GAME_WIDTH/2 - 100, 360, 200, 50 };
                DrawRectangleRec(btnBack, CheckCollisionPointRec(virtualMouse, btnBack) ? LIGHTGRAY : GRAY);
                DrawText("INAPOI", btnBack.x + 65, btnBack.y + 15, 20, BLACK);
            }

        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            Rectangle sourceRec = { 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height };
            Rectangle destRec = { 
                (GetScreenWidth() - GAME_WIDTH * scale) * 0.5f,
                (GetScreenHeight() - GAME_HEIGHT * scale) * 0.5f,
                GAME_WIDTH * scale,
                GAME_HEIGHT * scale
            };
            DrawTexturePro(target.texture, sourceRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}