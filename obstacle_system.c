#include "obstacle_system.h"
#include "raylib.h"

// ── Module-level textures ─────────────────────────────────
static Texture2D texUnicorn;
static Texture2D texFish;
static Texture2D texEye;

void SetObstacleTextures(Texture2D unicorn, Texture2D fish, Texture2D eye)
{
    texUnicorn = unicorn;
    texFish    = fish;
    texEye     = eye;
}

// ── Helpers ───────────────────────────────────────────────
static float RightmostObstacleX(Obstacle obstacles[], int count)
{
    float maxX = -9999.0f;
    for (int i = 0; i < count; i++) {
        if (obstacles[i].active) {
            float r = obstacles[i].position.x + obstacles[i].size.x;
            if (r > maxX) maxX = r;
        }
    }
    return maxX;
}

void ResetObstacles(Obstacle obstacles[], int count)
{
    for (int i = 0; i < count; i++)
        obstacles[i].active = false;
}

// ── Spawner ───────────────────────────────────────────────
void UpdateObstacleSpawner(
    Obstacle obstacles[],
    int      count,
    float   *spawnTimer,
    float    groundLevel,
    float    globalSpeedMultiplier,
    int      score)
{
    *spawnTimer -= GetFrameTime();
    if (*spawnTimer > 0.0f) return;

    // Enforce minimum on-screen gap
    float minGap = 260.0f / globalSpeedMultiplier;
    if (RightmostObstacleX(obstacles, count) > GAME_WIDTH - minGap) {
        *spawnTimer = 0.05f;
        return;
    }

    // Find free slot
    int slot = -1;
    for (int i = 0; i < count; i++) {
        if (!obstacles[i].active) { slot = i; break; }
    }
    if (slot == -1) { *spawnTimer = 0.1f; return; }

    obstacles[slot].active     = true;
    obstacles[slot].position.x = GAME_WIDTH + 5.0f;
    obstacles[slot].speed      = 6.0f;
    obstacles[slot].animFrame  = 0;
    obstacles[slot].animTimer  = 0.0f;

    // Type selection: mostly ground, more flying at higher scores
    int flyChance = 20 + (score / 5);
    if (flyChance > 60) flyChance = 60;
    bool spawnFlying = (GetRandomValue(1, 100) <= flyChance);

    if (!spawnFlying) {
        // ── UNICORN (ground) ────────────────────────────
        obstacles[slot].type  = TYPE_UNICORN;
        obstacles[slot].color = WHITE;
        // ~20% smaller than before (was 90px wide)
        float dw = 72.0f;
        float dh = dw * (UNICORN_SHEET_H / 174.0f);
        obstacles[slot].size  = (Vector2){ dw, dh };
        obstacles[slot].position.y = groundLevel - dh;

        // Double unicorn cluster — only at score >= 80, rare (1-in-4 chance)
        // Second unicorn gets a PROPER gap so the player has time to react
        if (score >= 80 && GetRandomValue(0, 3) == 0) {
            int slot2 = -1;
            for (int i = 0; i < count; i++) {
                if (!obstacles[i].active && i != slot) { slot2 = i; break; }
            }
            if (slot2 != -1) {
                obstacles[slot2]            = obstacles[slot];
                // Gap = 1.5-2.5 dino-widths — enough to land between them
                float gap = (float)GetRandomValue(180, 280);
                obstacles[slot2].position.x = GAME_WIDTH + 5.0f + dw + gap;
                obstacles[slot2].animFrame  = GetRandomValue(0, UNICORN_FRAME_COUNT - 1);
            }
        }
    } else {
        // ── FISH or EYE (flying) ───────────────────────
        bool useEye = (GetRandomValue(0, 1) == 0);
        if (useEye) {
            obstacles[slot].type  = TYPE_EYE;
            obstacles[slot].color = WHITE;
            float dw = 44.0f;   // was 55
            float dh = dw * (EYE_SHEET_H / 163.0f);
            obstacles[slot].size  = (Vector2){ dw, dh };
        } else {
            obstacles[slot].type  = TYPE_FISH;
            obstacles[slot].color = WHITE;
            float dw = 56.0f;   // was 70
            float dh = dw * (FISH_SHEET_H / 137.0f);
            obstacles[slot].size  = (Vector2){ dw, dh };
        }

        // Height zones: LOW=duck, MID=ambiguous, HIGH=jump
        int zone = GetRandomValue(0, 2);
        if (score < 30) zone = GetRandomValue(1, 2); // no floor-scrapers early
        if      (zone == 0) obstacles[slot].position.y = groundLevel - obstacles[slot].size.y - 5.0f;
        else if (zone == 1) obstacles[slot].position.y = groundLevel - obstacles[slot].size.y - 55.0f;
        else                obstacles[slot].position.y = groundLevel - obstacles[slot].size.y - 100.0f;
    }

    float interval = (float)GetRandomValue(80, 160) / 100.0f / globalSpeedMultiplier;
    if (interval < 0.65f) interval = 0.65f;
    *spawnTimer = interval;
}

// ── Update + Collision ────────────────────────────────────
bool UpdateObstaclesAndCheckCollision(
    Obstacle  obstacles[],
    int       count,
    float     globalSpeedMultiplier,
    Rectangle dinoRec,
    float     dt)
{
    for (int i = 0; i < count; i++) {
        if (!obstacles[i].active) continue;

        obstacles[i].position.x -= obstacles[i].speed * globalSpeedMultiplier;

        // Animate
        float animSpeed = 0.12f;  // seconds per frame
        obstacles[i].animTimer += dt;
        if (obstacles[i].animTimer >= animSpeed) {
            obstacles[i].animTimer = 0.0f;
            int maxF = (obstacles[i].type == TYPE_UNICORN) ? UNICORN_FRAME_COUNT
                     : (obstacles[i].type == TYPE_FISH)    ? FISH_FRAME_COUNT
                                                           : EYE_FRAME_COUNT;
            obstacles[i].animFrame = (obstacles[i].animFrame + 1) % maxF;
        }

        if (obstacles[i].position.x + obstacles[i].size.x < 0) {
            obstacles[i].active = false;
            continue;
        }

        // Slightly inset hitbox for fairness
        Rectangle obsRec = {
            obstacles[i].position.x + 6,
            obstacles[i].position.y + 6,
            obstacles[i].size.x - 12,
            obstacles[i].size.y - 12
        };
        if (CheckCollisionRecs(dinoRec, obsRec)) return true;
    }
    return false;
}

// ── Draw ──────────────────────────────────────────────────
void DrawObstacles(const Obstacle obstacles[], int count)
{
    for (int i = 0; i < count; i++) {
        if (!obstacles[i].active) continue;

        Rectangle dest = {
            obstacles[i].position.x,
            obstacles[i].position.y,
            obstacles[i].size.x,
            obstacles[i].size.y
        };

        if (obstacles[i].type == TYPE_UNICORN) {
            int f = obstacles[i].animFrame % UNICORN_FRAME_COUNT;
            // Crop src: unicorn content starts at y=6, ends at y=162 (h=157)
            // removing 6px top pad + 7px bottom pad so it sits on the ground
            Rectangle src = {
                (float)UNICORN_FRAMES[f].x,
                6.0f,
                (float)UNICORN_FRAMES[f].w,
                157.0f
            };
            DrawTexturePro(texUnicorn, src, dest, (Vector2){0,0}, 0.0f, WHITE);

        } else if (obstacles[i].type == TYPE_FISH) {
            int f = obstacles[i].animFrame % FISH_FRAME_COUNT;
            Rectangle src = {
                (float)FISH_FRAMES[f].x,
                0.0f,
                (float)FISH_FRAMES[f].w,
                (float)FISH_SHEET_H
            };
            DrawTexturePro(texFish, src, dest, (Vector2){0,0}, 0.0f, WHITE);

        } else { // TYPE_EYE
            int f = obstacles[i].animFrame % EYE_FRAME_COUNT;
            Rectangle src = {
                (float)EYE_FRAMES[f].x,
                0.0f,
                (float)EYE_FRAMES[f].w,
                (float)EYE_SHEET_H
            };
            DrawTexturePro(texEye, src, dest, (Vector2){0,0}, 0.0f, WHITE);
        }
    }
}