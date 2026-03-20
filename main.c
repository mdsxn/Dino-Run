#include "raylib.h"
#include "game_types.h"
#include "obstacle_system.h"
#include "scoreboard.h"

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

    // Variabile globale de control
    GameScreen currentScreen = SCREEN_MENU;
    bool exitGame = false; // Ne ajuta sa inchidem jocul din buton

   
    Player dino = { 0 };
    dino.size = (Vector2){ 40, 40 };
    dino.color = GREEN;
    dino.position = (Vector2){ 50, 200 };
    dino.velocity = 0.0f;
    dino.gravity = 0.6f;
    dino.isJumping = true;

    float groundLevel = GAME_HEIGHT - 20;

    // --- INITIALIZARE ARRAY OBSTACOLE ---
    Obstacle obstacles[MAX_OBSTACLES] = { 0 };
    float spawnTimer = 0.0f; // Cronometru pentru generare

    int score = 0;
    int framesCounter = 0; 
    float globalSpeedMultiplier = 1.0f; 
    bool scoreSaved = false;

    // THE GAME LOOP
    while(!WindowShouldClose() && !exitGame)
    {
        if(IsKeyPressed(KEY_F11)) ToggleFullscreen();

        // --- MATEMATICA PENTRU MOUSE PE ECRAN SCALAT ---
        float scale = (float)GetScreenWidth() / GAME_WIDTH;
        float scaleY = (float)GetScreenHeight() / GAME_HEIGHT;
        if (scaleY < scale) scale = scaleY;

        Vector2 mousePoint = GetMousePosition();
        // Traducem coordonatele reale ale mouse-ului in coordonatele canvas-ului virtual (800x450)
        Vector2 virtualMouse = { 
            (mousePoint.x - (GetScreenWidth() - (GAME_WIDTH * scale)) * 0.5f) / scale,
            (mousePoint.y - (GetScreenHeight() - (GAME_HEIGHT * scale)) * 0.5f) / scale 
        };

        // ==========================================
        // UPDATE LOGIC (Matematica, in functie de ecran)
        // ==========================================
        switch(currentScreen) 
        {
            case SCREEN_MENU:
            {
                // Definim butoanele (x, y, latime, inaltime)
                Rectangle btnPlay = { GAME_WIDTH/2 - 100, 150, 200, 50 };
                Rectangle btnExit = { GAME_WIDTH/2 - 100, 290, 200, 50 };

                // Daca dam click pe Play
                if (CheckCollisionPointRec(virtualMouse, btnPlay) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    // Resetam datele in caz ca venim din Game Over
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
                
                // Daca dam click pe Exit
                if (CheckCollisionPointRec(virtualMouse, btnExit) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    exitGame = true;
                }
            } break;

            case SCREEN_PLAYING:
            {
                // 1. FIZICA DINOZAURULUI
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

                // 2. SCOR SI VITEZA
                framesCounter++;
                if (framesCounter >= 10) {
                    score += 1;
                    framesCounter = 0;
                    globalSpeedMultiplier = 1.0f + (score / 100) * 0.1f; 
                }

                // 3. GENERATORUL ALEATOR DE OBSTACOLE
                UpdateObstacleSpawner(
                    obstacles,
                    MAX_OBSTACLES,
                    &spawnTimer,
                    groundLevel,
                    globalSpeedMultiplier);

                // 4. MISCAREA SI COLIZIUNEA OBSTACOLELOR
                Rectangle dinoRec = { dino.position.x, dino.position.y, dino.size.x, dino.size.y };

                if (UpdateObstaclesAndCheckCollision(
                        obstacles,
                        MAX_OBSTACLES,
                        globalSpeedMultiplier,
                        dinoRec)) {
                    currentScreen = SCREEN_GAME_OVER; // GAME OVER!
                    if (!scoreSaved) {
                        SaveScoreToCSV("Player", score);
                        scoreSaved = true;
                    }
                }
            } break;

            case SCREEN_GAME_OVER:
            {
                // Daca apesi spatiu, te intorci la meniu
                if (IsKeyPressed(KEY_SPACE)) {
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
                // Desenam Meniul
                DrawText("DINO RUN", GAME_WIDTH/2 - MeasureText("DINO RUN", 60)/2, 50, 60, MAROON);
                
                // Buton Play
                Rectangle btnPlay = { GAME_WIDTH/2 - 100, 150, 200, 50 };
                DrawRectangleRec(btnPlay, CheckCollisionPointRec(virtualMouse, btnPlay) ? LIGHTGRAY : GRAY);
                DrawText("PLAY", btnPlay.x + 65, btnPlay.y + 15, 20, BLACK);

                // Buton Leaderboard (Inactiv)
                Rectangle btnLeaderboard = { GAME_WIDTH/2 - 100, 220, 200, 50 };
                DrawRectangleRec(btnLeaderboard, LIGHTGRAY);
                DrawText("LEADERBOARD (WIP)", btnLeaderboard.x + 5, btnLeaderboard.y + 15, 18, DARKGRAY);

                // Buton Exit
                Rectangle btnExit = { GAME_WIDTH/2 - 100, 290, 200, 50 };
                DrawRectangleRec(btnExit, CheckCollisionPointRec(virtualMouse, btnExit) ? LIGHTGRAY : GRAY);
                DrawText("EXIT", btnExit.x + 75, btnExit.y + 15, 20, BLACK);

            } 
            else if (currentScreen == SCREEN_PLAYING) {
                // Desenam Jocul
                DrawLine(0, groundLevel, GAME_WIDTH, groundLevel, BLACK);
                DrawRectangleV(dino.position, dino.size, dino.color);

                DrawObstacles(obstacles, MAX_OBSTACLES);
                
                DrawText(TextFormat("SCORE: %05i", score), GAME_WIDTH - 150, 20, 20, DARKGRAY);
            }
            else if (currentScreen == SCREEN_GAME_OVER) {
                // Desenam ecranul de final
                DrawText("GAME OVER!", GAME_WIDTH/2 - MeasureText("GAME OVER!", 50)/2, 150, 50, RED);
                DrawText(TextFormat("FINAL SCORE: %i", score), GAME_WIDTH/2 - 100, 220, 30, DARKGRAY);
                DrawText("Press SPACE to return to Menu", GAME_WIDTH/2 - 170, 300, 20, GRAY);
            }

        EndTextureMode();

        // Desenam canvas-ul scalat pe monitor
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