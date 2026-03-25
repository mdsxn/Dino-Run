// game_types.h
// Aceste doua linii (#ifndef si #define) previn includerea accidentala de doua ori a fisierului
#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include "raylib.h"

#define GAME_WIDTH 800
#define GAME_HEIGHT 450
#define MAX_OBSTACLES 5

typedef enum { SCREEN_MENU, SCREEN_PLAYING, SCREEN_GAME_OVER, SCREEN_LEADERBOARD } GameScreen;
typedef enum { TYPE_CACTUS, TYPE_EAGLE } ObstacleType;
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
    ObstacleType type;
    bool active;
    Texture2D texture;
    Vector2 size;  
    float speed;   
    Color color;   
} Obstacle;

#endif