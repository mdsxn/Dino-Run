#include "raylib.h"

#define GAME_WIDTH 800
#define GAME_HEIGHT 450

typedef enum { DESERT, FOREST, SNOW } BiomeType;
typedef enum { DAY, NIGHT } TimeOfDay;

typedef struct {
    Vector2 position;
    float velocity;
    float gravity;
    bool isJumping;
    Texture2D texture;
    Vector2 size;
    Color color;
} Player;

// update la structura: size, speed si color
typedef struct {
    Vector2 position;
    int type;
    bool active;
    Texture2D texture;
    Vector2 size;  
    float speed;   
    Color color;   
} Obstacle;

typedef struct {
    int score;
    float globalSpeed;
    BiomeType biome;
    TimeOfDay timeOfDay;
    bool isGameOver;
} GameState;


int main(void)
{
    // drag and resize the window
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(GAME_WIDTH, GAME_HEIGHT, "Dino Game");
    SetTargetFPS(60);

    //Virtual canva
    RenderTexture2D target = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);

    // initalizam balaurul
    Player dino = { 0 };
    dino.size = (Vector2){ 40, 40 };
    dino.color = GREEN;
    dino.position = (Vector2){ 50, 200 };
    dino.velocity = 0.0f;
    dino.gravity = 0.6f;
    dino.isJumping = true;

    float groundLevel = GAME_HEIGHT - 20;

    // Initalizam cactusul
    Obstacle cactus = {0};
    cactus.size = (Vector2){ 30, 50 }; 
    cactus.color = DARKGREEN;
    cactus.position = (Vector2){ GAME_WIDTH, groundLevel - cactus.size.y }; 
    cactus.speed = 6.0f; 
    cactus.active = true;

    // Sistemul de progresie
    int score = 0;
    int framesCounter = 0; 
    float globalSpeedMultiplier = 1.0f; 

    // game loop
    while(!WindowShouldClose())
    {
        // toggle fullscreen
        if(IsKeyPressed(KEY_F11))
        {
            ToggleFullscreen();
        }

        // --- UPDATE LOGIC (Matematica) ---
        
        // 1. FIZICA DINOZAURULUI (Asta lipsea!)
        dino.velocity += dino.gravity;
        dino.position.y += dino.velocity;

        if (dino.position.y + dino.size.y >= groundLevel) {
            dino.position.y = groundLevel - dino.size.y; 
            dino.velocity = 0.0f;                       
            dino.isJumping = false;                      
        }

        if(IsKeyPressed(KEY_SPACE) && !dino.isJumping)
        {
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

        // 3. MISCAREA OBSTACOLELOR
        if (cactus.active) {
            cactus.position.x -= cactus.speed * globalSpeedMultiplier;
        }

        if (cactus.position.x + cactus.size.x < 0) {
            cactus.position.x = GAME_WIDTH; 
            cactus.size.y = (float)GetRandomValue(40, 70); 
            cactus.position.y = groundLevel - cactus.size.y; 
        }

        // 4. COLIZIUNE (GAME OVER)
        Rectangle dinoRec = { dino.position.x, dino.position.y, dino.size.x, dino.size.y };
        Rectangle cactusRec = { cactus.position.x, cactus.position.y, cactus.size.x, cactus.size.y };

        if (CheckCollisionRecs(dinoRec, cactusRec)) {
            // Resetam jocul complet la lovire
            dino.position.y = 200;
            cactus.position.x = GAME_WIDTH;
            score = 0;
            globalSpeedMultiplier = 1.0f;
        }


        // --- DRAW LOGIC (Desenarea) ---
        
        // draw to virtual canva
        BeginTextureMode(target);
            ClearBackground(RAYWHITE);

            DrawLine(0, groundLevel, GAME_WIDTH, groundLevel, BLACK);
            DrawRectangleV(dino.position, dino.size, dino.color);
            
            if (cactus.active) {
                DrawRectangleV(cactus.position, cactus.size, cactus.color);
            }
            
            DrawText(TextFormat("SCORE: %05i", score), GAME_WIDTH - 150, 20, 20, DARKGRAY);
            DrawText(TextFormat("SPEED: %.2fx", globalSpeedMultiplier), GAME_WIDTH - 150, 50, 10, GRAY);

        EndTextureMode();

        // draw virtual canva to screen
        BeginDrawing();
            ClearBackground(BLACK);

            float scale = (float)GetScreenWidth() / GAME_WIDTH;
            float scaleY = (float)GetScreenHeight() / GAME_HEIGHT;
            
            if (scaleY < scale) scale = scaleY;
            
            Rectangle sourceRec = { 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height };
            
            // AM REPARAT MATEMATICA AICI (Am pus * 0.5f)
            Rectangle destRec = { 
                (GetScreenWidth() - GAME_WIDTH * scale) * 0.5f,
                (GetScreenHeight() - GAME_HEIGHT * scale) * 0.5f,
                GAME_WIDTH * scale,
                GAME_HEIGHT * scale
            };
            
            DrawTexturePro(target.texture, sourceRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
        EndDrawing();
            
    }

    // cleanup
    UnloadRenderTexture(target);
    CloseWindow();

    return 0;
}