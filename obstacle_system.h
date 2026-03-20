#ifndef OBSTACLE_SYSTEM_H
#define OBSTACLE_SYSTEM_H

#include "game_types.h"

void ResetObstacles(Obstacle obstacles[], int count);

void UpdateObstacleSpawner(
    Obstacle obstacles[],
    int count,
    float *spawnTimer,
    float groundLevel,
    float globalSpeedMultiplier);

bool UpdateObstaclesAndCheckCollision(
    Obstacle obstacles[],
    int count,
    float globalSpeedMultiplier,
    Rectangle dinoRec);

void DrawObstacles(const Obstacle obstacles[], int count);

#endif
