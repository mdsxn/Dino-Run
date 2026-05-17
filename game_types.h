// game_types.h
#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include "raylib.h"

#define GAME_WIDTH   800
#define GAME_HEIGHT  450
#define MAX_OBSTACLES 6

// Dino sprite sheet: 8 frames, variable width
// x = content start on sheet, w = content width (black padding trimmed)
#define DINO_FRAME_COUNT 8
typedef struct { int x; int w; } FrameInfo;
static const FrameInfo DINO_FRAMES[DINO_FRAME_COUNT] = {
    {20, 76},   // frame 0: full content
    {128,76},   // frame 1
    {228,90},   // frame 2: trimmed 3px left padding
    {342,78},   // frame 3: trimmed 9px left padding
    {451,77},   // frame 4
    {560,77},   // frame 5
    {660,90},   // frame 6: trimmed 3px left padding
    {774,79},   // frame 7: trimmed 3px left padding
};
// Max content width across all frames — used to normalise dest width
#define DINO_MAX_CONTENT_W 90

// Fish sprite sheet: 4 frames
#define FISH_FRAME_COUNT 4
static const FrameInfo FISH_FRAMES[FISH_FRAME_COUNT] = {
    {16,137},{171,146},{327,138},{483,145}
};
#define FISH_SHEET_H 134

// Eye2 sprite: 2 frames (389x142 sheet)
#define EYE_FRAME_COUNT 2
static const FrameInfo EYE_FRAMES[EYE_FRAME_COUNT] = {
    {19,163},{207,164}
};
#define EYE_SHEET_H 142

// Unicorn sprite sheet: 6 real frames
#define UNICORN_FRAME_COUNT 6
static const FrameInfo UNICORN_FRAMES[UNICORN_FRAME_COUNT] = {
    {19,174},{214,174},{408,179},{603,176},{799,173},{992,176}
};
#define UNICORN_SHEET_H 169

typedef enum {
    SCREEN_MENU, SCREEN_PLAYING, SCREEN_GAME_OVER, SCREEN_LEADERBOARD
} GameScreen;

// Obstacle sub-types
typedef enum {
    TYPE_UNICORN,  // ground obstacle (replaces cactus)
    TYPE_FISH,     // flying obstacle
    TYPE_EYE       // flying obstacle (higher)
} ObstacleType;

typedef enum { DESERT, FOREST, SNOW } BiomeType;
typedef enum { DAY, NIGHT }           TimeOfDay;

typedef struct {
    Vector2   position;
    float     velocity;
    float     gravity;
    bool      isJumping;
    bool      isDucking;
    Texture2D texture;
    Vector2   size;
    Vector2   duckSize;
    Color     color;
    // animation
    int       animFrame;
    float     animTimer;
    float     animSpeed;  // seconds per frame
} Player;

typedef struct {
    Vector2      position;
    ObstacleType type;
    bool         active;
    Vector2      size;
    float        speed;
    Color        color;
    // animation
    int          animFrame;
    float        animTimer;
} Obstacle;

#endif