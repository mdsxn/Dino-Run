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

typedef struct {
    Vector2 position;
    int type;
    bool active;
    Texture2D texture;
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

    // initalizam crocodilul
    Player dino = { 0 };
    dino.size = (Vector2){ 40, 40 };
    dino.color = GREEN;
    dino.position = (Vector2){ 50, 200};
    dino.velocity = 0.0f;
    dino.gravity = 0.6f;
    dino.isJumping = true;

    float growndLevel = GAME_HEIGHT - 20;

    // game loop
    while(!WindowShouldClose())
    {
        // toggle fullscreen
        if(IsKeyPressed(KEY_F11))
        {
            ToggleFullscreen();
        
    }

    // update dino
    dino.velocity += dino.gravity;
    dino.position.y += dino.velocity;   

    if(dino.position.y + dino.size.y >= growndLevel)
    {
        dino.position.y = growndLevel - dino.size.y;
        dino.isJumping = false;
        dino.velocity = 0.0f;

    }
    // jump
    if(IsKeyPressed(KEY_SPACE) && !dino.isJumping)
    {
        dino.velocity = -12.0f;
        dino.isJumping = true;
    }

        // draw to virtual canva
        BeginTextureMode(target);
        ClearBackground(RAYWHITE);

        DrawLine(0, growndLevel, GAME_WIDTH, growndLevel, BLACK );
        DrawRectangleV(dino.position, dino.size, dino.color);

        DrawText("Press SPACE to jump. Press F11 for fullscreen.", 10, 10, 20, DARKGRAY);
        EndTextureMode();// end draw to virtual canva

        // draw virtual canva to screen
        BeginDrawing();
            ClearBackground(BLACK);

            float scale = (float)GetScreenWidth() / GAME_WIDTH;
            float scaleY = (float)GetScreenHeight() / GAME_HEIGHT;
            
            // maintain aspect ratio
            if (scaleY < scale) scale = scaleY;
            // calculate source and destination rectangles for drawing the texture
            Rectangle sourceRec = { 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height };
            // center the texture on the screen
            Rectangle destRec = { 
            (GetScreenWidth() - GAME_WIDTH * scale) / 0.5f,
            (GetScreenHeight() - GAME_HEIGHT * scale) / 0.5f,
            GAME_WIDTH * scale,
            GAME_HEIGHT * scale
            };
            // draw the scaled texture to the screen
            DrawTexturePro(target.texture, sourceRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
        EndDrawing();
            
        }
    // cleanup
    UnloadRenderTexture(target);
    CloseWindow();

    return 0;


    }



















