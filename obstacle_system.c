#include "obstacle_system.h"

#include "raylib.h"

void ResetObstacles(Obstacle obstacles[], int count)
{
    for (int i = 0; i < count; i++) {
        obstacles[i].active = false;
    }
}

void UpdateObstacleSpawner(
    Obstacle obstacles[],
    int count,
    float *spawnTimer,
    float groundLevel,
    float globalSpeedMultiplier)
{
    *spawnTimer -= GetFrameTime();
    if (*spawnTimer > 0.0f) {
        return;
    }

    for (int i = 0; i < count; i++) {
        if (obstacles[i].active) {
            continue;
        }

        obstacles[i].active = true;
        obstacles[i].position.x = GAME_WIDTH;
        obstacles[i].speed = 6.0f;

        int randomType = GetRandomValue(0, 1);
        if (randomType == 0) {
            obstacles[i].type = TYPE_CACTUS;
            obstacles[i].size = (Vector2){
                (float)GetRandomValue(30, 45),
                (float)GetRandomValue(40, 65)
            };
            obstacles[i].color = DARKGREEN;
            obstacles[i].position.y = groundLevel - obstacles[i].size.y;
        } else {
            obstacles[i].type = TYPE_EAGLE;
            obstacles[i].size = (Vector2){ 40, 20 };
            obstacles[i].color = BLUE;

            int isHigh = GetRandomValue(0, 1);
            obstacles[i].position.y = isHigh ? groundLevel - 75 : groundLevel - 40;
        }

        break;
    }

    *spawnTimer = (float)GetRandomValue(80, 150) / 100.0f / globalSpeedMultiplier;
}

bool UpdateObstaclesAndCheckCollision(
    Obstacle obstacles[],
    int count,
    float globalSpeedMultiplier,
    Rectangle dinoRec)
{
    for (int i = 0; i < count; i++) {
        if (!obstacles[i].active) {
            continue;
        }

        obstacles[i].position.x -= obstacles[i].speed * globalSpeedMultiplier;

        if (obstacles[i].position.x + obstacles[i].size.x < 0) {
            obstacles[i].active = false;
            continue;
        }

        Rectangle obsRec = {
            obstacles[i].position.x,
            obstacles[i].position.y,
            obstacles[i].size.x,
            obstacles[i].size.y
        };

        if (CheckCollisionRecs(dinoRec, obsRec)) {
            return true;
        }
    }

    return false;
}

void DrawObstacles(const Obstacle obstacles[], int count)
{
    for (int i = 0; i < count; i++) {
        if (obstacles[i].active) {
            DrawRectangleV(obstacles[i].position, obstacles[i].size, obstacles[i].color);
        }
    }
}
