#ifndef OBSTACLE_SYSTEM_H
#define OBSTACLE_SYSTEM_H

#include "game_types.h"

// Textures for the three obstacle types — set once at startup
void SetObstacleTextures(Texture2D unicorn, Texture2D fish, Texture2D eye);

void ResetObstacles(Obstacle obstacles[], int count);

void UpdateObstacleSpawner(
    Obstacle obstacles[],
    int      count,
    float   *spawnTimer,
    float    groundLevel,
    float    globalSpeedMultiplier,
    int      score);

// Returns true on collision
bool UpdateObstaclesAndCheckCollision(
    Obstacle  obstacles[],
    int       count,
    float     globalSpeedMultiplier,
    Rectangle dinoRec,
    float     dt);

void DrawObstacles(const Obstacle obstacles[], int count);

#endif