#include "raylib.h"
#include "game_types.h"
#include "obstacle_system.h"
#include "leaderboard.h"
#include <math.h>
#include <string.h>

// ─────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────
static Color LerpColor(Color a, Color b, float t)
{
    return (Color){
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}
static void DrawCard(Rectangle r, Color fill, Color border, float radius)
{
    DrawRectangleRounded(r, radius, 8, fill);
    DrawRectangleRoundedLines(r, radius, 8, border);
}

// ─────────────────────────────────────────────────────────
//  Parallax background trees (using loaded PNG textures)
//  Two layers: far (tree.png tinted dark) and near (normal)
// ─────────────────────────────────────────────────────────
#define NUM_BG_TREES 16
typedef struct {
    float x, y, scale;
    int   layer;   // 0=far, 1=near
} BgTree;

static BgTree bgTrees[NUM_BG_TREES];
static Texture2D texTreeFar;
static Texture2D texTreeNear;
static Texture2D texTileGrass;
static Texture2D texTileDirt;

static void InitBgTrees(float groundY)
{
    // Spread trees evenly across the screen + overflow buffer.
    // Start first tree at x=200 minimum so it doesn't overlap the dino (x=120).
    for (int i = 0; i < NUM_BG_TREES; i++) {
        bgTrees[i].layer = i % 2;
        bgTrees[i].scale = (bgTrees[i].layer == 0)
                         ? (float)GetRandomValue(35, 52) / 100.0f
                         : (float)GetRandomValue(58, 82) / 100.0f;

        float treeH = texTreeFar.height * bgTrees[i].scale;
        bgTrees[i].y = groundY - treeH + 4.0f;

        // Distribute evenly across [200 .. GAME_WIDTH+500] so no tree starts
        // too close to the left edge and overlaps the player
        float slot = (float)i / NUM_BG_TREES;
        bgTrees[i].x = 200.0f + slot * (GAME_WIDTH + 300.0f) + (float)GetRandomValue(0, 50);
    }
}

static void UpdateDrawBgTrees(float speedMult, float groundY)
{
    float speeds[2] = { 0.35f, 1.0f };

    // Draw far layer first, then near
    for (int layer = 0; layer < 2; layer++) {
        for (int i = 0; i < NUM_BG_TREES; i++) {
            if (bgTrees[i].layer != layer) continue;

            bgTrees[i].x -= speeds[layer] * speedMult;

            float tw = texTreeFar.width  * bgTrees[i].scale;
            float th = texTreeFar.height * bgTrees[i].scale;

            if (bgTrees[i].x + tw < 0) {
                // Respawn off the right edge
                bgTrees[i].x     = GAME_WIDTH + (float)GetRandomValue(20, 100);
                bgTrees[i].scale = (layer == 0)
                    ? (float)GetRandomValue(35, 52) / 100.0f
                    : (float)GetRandomValue(58, 82) / 100.0f;
                tw = texTreeFar.width  * bgTrees[i].scale;
                th = texTreeFar.height * bgTrees[i].scale;
                bgTrees[i].y = groundY - th + 4.0f;
            }

            Rectangle src  = { 0, 0, (float)texTreeFar.width, (float)texTreeFar.height };
            Rectangle dest = { bgTrees[i].x, bgTrees[i].y, tw, th };
            // Far layer: darker/bluer tint; near: slight green tint for depth
            Color tint = (layer == 0)
                ? (Color){ 100, 130, 160, 180 }
                : (Color){ 200, 230, 200, 220 };
            DrawTexturePro(texTreeFar, src, dest, (Vector2){0,0}, 0.0f, tint);
        }
    }
}

// ─────────────────────────────────────────────────────────
//  Clouds
// ─────────────────────────────────────────────────────────
#define NUM_CLOUDS 6
typedef struct { float x, y, w, h, speed; } Cloud;
static Cloud clouds[NUM_CLOUDS];

static void InitClouds(void)
{
    for (int i = 0; i < NUM_CLOUDS; i++) {
        clouds[i].x     = (float)GetRandomValue(0, GAME_WIDTH);
        clouds[i].y     = (float)GetRandomValue(18, 90);
        clouds[i].w     = (float)GetRandomValue(70, 150);
        clouds[i].h     = (float)GetRandomValue(20, 40);
        clouds[i].speed = (float)GetRandomValue(15, 50) / 100.0f;
    }
}
static void UpdateDrawClouds(float speedMult)
{
    for (int i = 0; i < NUM_CLOUDS; i++) {
        clouds[i].x -= clouds[i].speed * speedMult;
        if (clouds[i].x + clouds[i].w < 0) {
            clouds[i].x = GAME_WIDTH + clouds[i].w;
            clouds[i].y = (float)GetRandomValue(18, 90);
        }
        DrawEllipse((int)(clouds[i].x + clouds[i].w * 0.5f),
                    (int)(clouds[i].y),
                    (int)(clouds[i].w * 0.5f),
                    (int)(clouds[i].h * 0.5f),
                    (Color){ 230, 240, 255, 170 });
        DrawEllipse((int)(clouds[i].x + clouds[i].w * 0.3f),
                    (int)(clouds[i].y - clouds[i].h * 0.25f),
                    (int)(clouds[i].w * 0.25f),
                    (int)(clouds[i].h * 0.42f),
                    (Color){ 240, 248, 255, 150 });
    }
}

// ─────────────────────────────────────────────────────────
//  Procedural SFX
// ─────────────────────────────────────────────────────────
#define SFX_SAMPLE_RATE 44100

static Wave MakeSquareWave(float freq, float duration, float volume)
{
    int    frames = (int)(SFX_SAMPLE_RATE * duration);
    short *data   = (short*)MemAlloc(frames * sizeof(short));
    for (int i = 0; i < frames; i++) {
        float env   = 1.0f - (float)i / frames;
        float phase = fmodf((float)i * freq / SFX_SAMPLE_RATE, 1.0f);
        data[i] = (short)((phase < 0.5f ? 1.0f : -1.0f) * env * volume * 32000.0f);
    }
    Wave w = { 0 };
    w.frameCount = frames;
    w.sampleRate = SFX_SAMPLE_RATE;
    w.sampleSize = 16;
    w.channels   = 1;
    w.data       = data;
    return w;
}

// ─────────────────────────────────────────────────────────
//  UI Button helper
// ─────────────────────────────────────────────────────────
typedef struct {
    Rectangle   rect;
    const char *label;
    Color normal, hover, text;
    int fontSize;
} Button;

static bool DrawButton(Button btn, Vector2 mouse, bool disabled)
{
    bool hov  = !disabled && CheckCollisionPointRec(mouse, btn.rect);
    Color fill = disabled ? (Color){60,60,60,200} : (hov ? btn.hover : btn.normal);
    Color tc   = disabled ? DARKGRAY : btn.text;
    DrawRectangleRounded((Rectangle){btn.rect.x+3,btn.rect.y+4,btn.rect.width,btn.rect.height},
                         0.3f,8,(Color){0,0,0,80});
    DrawCard(btn.rect, fill, (Color){255,255,255,30}, 0.3f);
    int tw = MeasureText(btn.label, btn.fontSize);
    DrawText(btn.label,
             (int)(btn.rect.x + (btn.rect.width  - tw)            * 0.5f),
             (int)(btn.rect.y + (btn.rect.height - btn.fontSize)   * 0.5f),
             btn.fontSize, tc);
    return hov && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

// ─────────────────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────────────────
int main(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(GAME_WIDTH, GAME_HEIGHT, "Dino Run");
    SetTargetFPS(60);
    InitAudioDevice();

    RenderTexture2D target = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);

    // ── Load textures ─────────────────────────────────────
    Texture2D texDinoSheet = LoadTexture("assets/dino1.png");
    Texture2D texUnicorn   = LoadTexture("assets/unicorn_spryte.png");
    Texture2D texFish      = LoadTexture("assets/fish.png");
    Texture2D texEye       = LoadTexture("assets/eye2.png");
    texTreeFar  = LoadTexture("assets/tree.png");
    texTreeNear = texTreeFar;
    texTileGrass = LoadTexture("assets/tile_grass.png");
    texTileDirt  = LoadTexture("assets/tile_dirt.png");
    Texture2D texTileDirt2 = LoadTexture("assets/tile_dirt2.png");

    SetTextureFilter(texDinoSheet,  TEXTURE_FILTER_POINT);
    SetTextureFilter(texUnicorn,    TEXTURE_FILTER_POINT);
    SetTextureFilter(texFish,       TEXTURE_FILTER_POINT);
    SetTextureFilter(texEye,        TEXTURE_FILTER_POINT);
    SetTextureFilter(texTreeFar,    TEXTURE_FILTER_POINT);
    SetTextureFilter(texTileGrass,  TEXTURE_FILTER_POINT);
    SetTextureFilter(texTileDirt,   TEXTURE_FILTER_POINT);
    SetTextureFilter(texTileDirt2,  TEXTURE_FILTER_POINT);

    SetObstacleTextures(texUnicorn, texFish, texEye);

    // ── Load music ────────────────────────────────────────
    // Load music — rename the file to "music.mp3" in your assets/ folder
    Music bgMusic = LoadMusicStream("assets/music.mp3");
    SetMusicVolume(bgMusic, 0.85f);
    PlayMusicStream(bgMusic);

    // ── SFX ───────────────────────────────────────────────
    Wave wJump  = MakeSquareWave(523.0f, 0.12f, 0.32f);
    Wave wDeath = MakeSquareWave(110.0f, 0.35f, 0.42f);
    Wave wScore = MakeSquareWave(880.0f, 0.06f, 0.18f);
    Sound sfxJump  = LoadSoundFromWave(wJump);
    Sound sfxDeath = LoadSoundFromWave(wDeath);
    Sound sfxScore = LoadSoundFromWave(wScore);
    UnloadWave(wJump); UnloadWave(wDeath); UnloadWave(wScore);

    // ── State ─────────────────────────────────────────────
    GameScreen currentScreen = SCREEN_MENU;
    bool exitGame = false;
    char playerName[20] = "\0";
    int  playerLetters  = 0;
    PlayerScore top5[5] = { 0 };
    int loadedScoresCount = 0;

    // ── Ground & dino setup ───────────────────────────────
    float groundLevel = GAME_HEIGHT - 30;

    // Dino display: use actual content size from sheet (90px wide x 151px tall content)
    // Display at 46px wide -> 46 * (151/90) = ~77px tall
    float dinoDispW = 46.0f;
    float dinoDispH = dinoDispW * (151.0f / 90.0f);  // ~77px — correct aspect ratio

    Player dino    = { 0 };
    dino.gravity   = 0.55f;
    dino.size      = (Vector2){ dinoDispW, dinoDispH };
    dino.duckSize  = (Vector2){ dinoDispW * 1.3f, dinoDispH * 0.50f };
    dino.isDucking = false;
    dino.isJumping = false;
    dino.position  = (Vector2){ 100.0f, groundLevel - dinoDispH };
    dino.velocity  = 0.0f;
    dino.animFrame = 0;
    dino.animTimer = 0.0f;
    dino.animSpeed = 0.09f;  // seconds per frame while running

    // ── Obstacles ─────────────────────────────────────────
    Obstacle obstacles[MAX_OBSTACLES] = { 0 };
    float spawnTimer = 0.0f;

    // ── Numerics ──────────────────────────────────────────
    int   score               = 0;
    int   hiScore             = 0;
    int   framesCounter       = 0;
    float globalSpeedMultiplier = 1.0f;
    bool  scoreSaved          = false;
    int   lastScoreMilestone  = 0;

    float menuTime  = 0.0f;
    float deathShake = 0.0f;

    InitBgTrees(groundLevel);
    InitClouds();

    // ═════════════════════════════════════════════════════
    while (!WindowShouldClose() && !exitGame)
    {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

        // Scale & virtual mouse
        float scale  = (float)GetScreenWidth()  / GAME_WIDTH;
        float scaleY = (float)GetScreenHeight() / GAME_HEIGHT;
        if (scaleY < scale) scale = scaleY;
        Vector2 mouseRaw = GetMousePosition();
        Vector2 mouse = {
            (mouseRaw.x - (GetScreenWidth()  - GAME_WIDTH  * scale) * 0.5f) / scale,
            (mouseRaw.y - (GetScreenHeight() - GAME_HEIGHT * scale) * 0.5f) / scale
        };

        // Keep music looping
        UpdateMusicStream(bgMusic);

        // ═══════════════════════════════════════════════════
        // UPDATE
        // ═══════════════════════════════════════════════════
        switch (currentScreen)
        {
        // ── MENU ───────────────────────────────────────────
        case SCREEN_MENU:
        {
            menuTime += dt;

            int key = GetCharPressed();
            while (key > 0) {
                if (key >= 32 && key <= 125 && playerLetters < 15) {
                    playerName[playerLetters]   = (char)key;
                    playerName[playerLetters+1] = '\0';
                    playerLetters++;
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && playerLetters > 0)
                playerName[--playerLetters] = '\0';

            // Animate dino on menu screen too (idle walk)
            dino.animTimer += dt;
            if (dino.animTimer >= dino.animSpeed * 1.5f) {
                dino.animTimer = 0.0f;
                dino.animFrame = (dino.animFrame + 1) % DINO_FRAME_COUNT;
            }

            Button btnPlay = {{GAME_WIDTH/2.0f-110,220,220,50},"PLAY",
                {50,180,90,230},{80,220,120,255},WHITE,22};
            Button btnLB   = {{GAME_WIDTH/2.0f-110,285,220,50},"LEADERBOARD",
                {50,100,200,230},{80,140,240,255},WHITE,20};
            Button btnExit = {{GAME_WIDTH/2.0f-110,350,220,50},"EXIT",
                {180,50,50,230},{220,80,80,255},WHITE,22};

            if (DrawButton(btnPlay, mouse, playerLetters == 0)) {
                score = 0; framesCounter = 0; lastScoreMilestone = 0;
                globalSpeedMultiplier = 1.0f; scoreSaved = false; spawnTimer = 0.0f;
                dino.position  = (Vector2){ 100.0f, groundLevel - dinoDispH };
                dino.velocity  = 0.0f;
                dino.isJumping = false; dino.isDucking = false;
                dino.animFrame = 0;    dino.animTimer  = 0.0f;
                ResetObstacles(obstacles, MAX_OBSTACLES);
                currentScreen = SCREEN_PLAYING;
            }
            if (DrawButton(btnLB, mouse, false)) {
                loadedScoresCount = LoadTopScores(top5, 5);
                currentScreen = SCREEN_LEADERBOARD;
            }
            if (DrawButton(btnExit, mouse, false)) exitGame = true;
        } break;

        // ── PLAYING ────────────────────────────────────────
        case SCREEN_PLAYING:
        {
            // Duck
            bool wantDuck  = IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);
            dino.isDucking = wantDuck && !dino.isJumping;

            // Physics
            dino.velocity   += dino.gravity;
            dino.position.y += dino.velocity;

            // Ground snap — always based on full standing height
            if (dino.position.y + dino.size.y >= groundLevel) {
                dino.position.y = groundLevel - dino.size.y;
                dino.velocity   = 0.0f;
                dino.isJumping  = false;
            }

            // Jump
            if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
                && !dino.isJumping) {
                dino.velocity  = -13.5f;
                dino.isJumping = true;
                dino.isDucking = false;
                PlaySound(sfxJump);
            }

            // Dino animation
            dino.animTimer += dt;
            float aspd = dino.isJumping ? 0.12f : (dino.isDucking ? 0.07f : dino.animSpeed);
            if (dino.animTimer >= aspd) {
                dino.animTimer = 0.0f;
                dino.animFrame = (dino.animFrame + 1) % DINO_FRAME_COUNT;
            }

            // Score / speed
            framesCounter++;
            if (framesCounter >= 10) {
                score++;
                framesCounter = 0;
                globalSpeedMultiplier = 1.0f + (score / 100) * 0.12f;
                if (globalSpeedMultiplier > 3.0f) globalSpeedMultiplier = 3.0f;
                if (score / 50 > lastScoreMilestone) {
                    lastScoreMilestone = score / 50;
                    PlaySound(sfxScore);
                }
            }
            if (score > hiScore) hiScore = score;

            UpdateObstacleSpawner(obstacles, MAX_OBSTACLES, &spawnTimer,
                                  groundLevel, globalSpeedMultiplier, score);

            // Hitbox — when ducking, bottom-anchored to ground, half height
            float hbW, hbH, hbY;
            if (dino.isDucking && !dino.isJumping) {
                hbW = dino.duckSize.x;
                hbH = dino.duckSize.y;
                hbY = groundLevel - dino.duckSize.y;
            } else {
                hbW = dino.size.x;
                hbH = dino.size.y;
                hbY = dino.position.y;
            }
            Rectangle dinoRec = { dino.position.x + 5, hbY + 5, hbW - 10, hbH - 10 };

            if (UpdateObstaclesAndCheckCollision(obstacles, MAX_OBSTACLES,
                                                 globalSpeedMultiplier, dinoRec, dt)) {
                currentScreen = SCREEN_GAME_OVER;
                deathShake = 0.5f;
                PlaySound(sfxDeath);
                if (!scoreSaved) {
                    SaveScoreToCSV(playerName, score);
                    scoreSaved = true;
                }
            }
        } break;

        // ── GAME OVER ──────────────────────────────────────
        case SCREEN_GAME_OVER:
        {
            deathShake -= dt;
            if (deathShake < 0) deathShake = 0;
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))
                currentScreen = SCREEN_MENU;
        } break;

        // ── LEADERBOARD ────────────────────────────────────
        case SCREEN_LEADERBOARD:
        {
            Button btnBack = {{GAME_WIDTH/2.0f-90,380,180,45},"BACK",
                {80,80,80,220},{120,120,120,255},WHITE,20};
            if (DrawButton(btnBack, mouse, false))
                currentScreen = SCREEN_MENU;
        } break;
        }

        // ═══════════════════════════════════════════════════
        // DRAW
        // ═══════════════════════════════════════════════════
        BeginTextureMode(target);
        BeginScissorMode(0, 0, GAME_WIDTH, GAME_HEIGHT);

        // ── Terraria day-sky gradient ──────────────────────
        int gY = (int)groundLevel;
        Color skyTop    = (Color){  82, 152, 220, 255 };
        Color skyBottom = (Color){ 148, 205, 248, 255 };
        for (int y = 0; y < gY; y++) {
            float t = (float)y / gY;
            DrawLine(0, y, GAME_WIDTH, y, LerpColor(skyTop, skyBottom, t));
        }

        // Clouds behind trees
        float bgSpd = (currentScreen == SCREEN_PLAYING) ? globalSpeedMultiplier : 0.5f;
        UpdateDrawClouds(bgSpd);

        // Parallax trees
        UpdateDrawBgTrees(bgSpd, (float)gY);

        // ── Tiled ground ──────────────────────────────────
        // Tile size: draw each tile scaled to fit the ground strip height
        // Ground strip = GAME_HEIGHT - gY = 30px total
        // Top half (~15px) = grass tiles, bottom half (~15px) = dirt tiles
        int tileH     = 16;  // display height per tile row
        int tileW     = 16;  // display width per tile (square)
        int grassY    = gY;
        int dirtY     = gY + tileH;

        // Fill any remaining gap below dirt row with solid dirt color
        if (dirtY + tileH < GAME_HEIGHT)
            DrawRectangle(0, dirtY + tileH, GAME_WIDTH, GAME_HEIGHT - dirtY - tileH,
                          (Color){ 105, 72, 42, 255 });

        // Draw grass row
        if (texTileGrass.width > 0) {
            Rectangle src  = { 0, 0, (float)texTileGrass.width, (float)texTileGrass.height };
            for (int x = 0; x < GAME_WIDTH; x += tileW) {
                Rectangle dest = { (float)x, (float)grassY, (float)tileW, (float)tileH };
                DrawTexturePro(texTileGrass, src, dest, (Vector2){0,0}, 0.0f, WHITE);
            }
        } else {
            DrawRectangle(0, grassY, GAME_WIDTH, tileH, (Color){ 85, 170, 65, 255 });
        }

        // Draw dirt row — alternate between two dirt block variants
        if (texTileDirt.width > 0) {
            Rectangle src1 = { 0, 0, (float)texTileDirt.width,  (float)texTileDirt.height };
            Rectangle src2 = { 0, 0, (float)texTileDirt2.width, (float)texTileDirt2.height };
            int tileIdx = 0;
            for (int x = 0; x < GAME_WIDTH; x += tileW) {
                Rectangle dest = { (float)x, (float)dirtY, (float)tileW, (float)tileH };
                DrawTexturePro((tileIdx % 2 == 0) ? texTileDirt : texTileDirt2,
                               (tileIdx % 2 == 0) ? src1 : src2,
                               dest, (Vector2){0,0}, 0.0f, WHITE);
                tileIdx++;
            }
        } else {
            DrawRectangle(0, dirtY, GAME_WIDTH, tileH, (Color){ 105, 72, 42, 255 });
        }

        // ══════════════════════════════════════════════════
        if (currentScreen == SCREEN_MENU)
        {
            // Slight dark overlay so UI pops over background
            DrawRectangle(0,0,GAME_WIDTH,GAME_HEIGHT,(Color){0,15,35,75});

            // Title
            float pulse = 1.0f + sinf(menuTime * 2.5f) * 0.04f;
            int titleSz = (int)(64 * pulse);
            const char *title = "DINO RUN";
            int tw = MeasureText(title, titleSz);
            DrawText(title, GAME_WIDTH/2-tw/2+4, 28, titleSz, (Color){0,0,0,120});
            DrawText(title, GAME_WIDTH/2-tw/2,   26, titleSz, (Color){255,230,55,255});

            // Controls hint
            DrawText("SPACE: Jump  |  DOWN: Duck  |  F11: Fullscreen",
                     GAME_WIDTH/2-MeasureText("SPACE: Jump  |  DOWN: Duck  |  F11: Fullscreen",13)/2,
                     100, 13, (Color){210,235,255,185});

            // Animated dino on menu
            {
                int f = (DINO_FRAME_COUNT - 1) - (dino.animFrame % DINO_FRAME_COUNT);
                float frameDispW = (float)DINO_FRAMES[f].w / DINO_MAX_CONTENT_W * dinoDispW;
                Rectangle src  = { (float)DINO_FRAMES[f].x, 2.0f,
                                   (float)DINO_FRAMES[f].w, 151.0f };
                Rectangle dest = { GAME_WIDTH/2.0f + 130, 115,
                                   frameDispW * 1.4f, dinoDispH * 1.4f };
                DrawTexturePro(texDinoSheet, src, dest, (Vector2){0,0}, 0.0f, WHITE);
            }

            // Name box
            DrawText("ENTER YOUR NAME", GAME_WIDTH/2-90, 138, 16, (Color){205,225,255,215});
            Rectangle nameBox = {GAME_WIDTH/2.0f-110, 158, 220, 44};
            DrawCard(nameBox, (Color){10,25,55,215}, (Color){100,175,255,185}, 0.25f);
            DrawText(playerName, (int)(nameBox.x+12), (int)(nameBox.y+12), 22, WHITE);
            if ((int)(GetTime()*2.0)%2==0 && playerLetters<15)
                DrawText("_", (int)(nameBox.x+12+MeasureText(playerName,22)),
                         (int)(nameBox.y+12), 22, (Color){100,175,255,255});
            if (playerLetters==0)
                DrawText("^ type your name to play",
                         GAME_WIDTH/2-MeasureText("^ type your name to play",13)/2,
                         207, 13, (Color){255,115,115,210});

            Button btnPlay = {{GAME_WIDTH/2.0f-110,220,220,50},"PLAY",
                {50,180,90,230},{80,220,120,255},WHITE,22};
            Button btnLB   = {{GAME_WIDTH/2.0f-110,285,220,50},"LEADERBOARD",
                {50,100,200,230},{80,140,240,255},WHITE,20};
            Button btnExit = {{GAME_WIDTH/2.0f-110,350,220,50},"EXIT",
                {180,50,50,230},{220,80,80,255},WHITE,22};
            DrawButton(btnPlay, mouse, playerLetters==0);
            DrawButton(btnLB,   mouse, false);
            DrawButton(btnExit, mouse, false);

            if (hiScore > 0) {
                const char *hsTxt = TextFormat("HI %05d", hiScore);
                int hsW = MeasureText(hsTxt,16);
                DrawCard((Rectangle){GAME_WIDTH-hsW-30,10,(float)hsW+18,28},
                         (Color){255,200,0,40},(Color){255,200,0,120},0.4f);
                DrawText(hsTxt, GAME_WIDTH-hsW-20, 15, 16, (Color){255,220,50,255});
            }
        }
        // ══════════════════════════════════════════════════
        else if (currentScreen == SCREEN_PLAYING)
        {
            DrawObstacles(obstacles, MAX_OBSTACLES);

            // ---- Draw dino sprite ----
            // src crops to actual content rows (y=2..152), removing 2px top + 10px bottom padding
            // dest is shrunk 4% inside the physics box so the sprite sits fully within it
            int f = (DINO_FRAME_COUNT - 1) - (dino.animFrame % DINO_FRAME_COUNT);
            float frameDispW = (float)DINO_FRAMES[f].w / DINO_MAX_CONTENT_W * dinoDispW;
            Rectangle dinoSrc = { (float)DINO_FRAMES[f].x, 2.0f,
                                  (float)DINO_FRAMES[f].w, 151.0f };
            float shrink = 0.96f;  // 4% smaller texture inside same physics box
            float texW = frameDispW * shrink;
            float texH = dino.size.y * shrink;

            if (dino.isDucking && !dino.isJumping) {
                float duckDrawY = groundLevel - dino.duckSize.y;
                float duckTexH  = dino.duckSize.y * shrink;
                Rectangle dest  = { dino.position.x, duckDrawY + (dino.duckSize.y - duckTexH),
                                    texW * 1.3f, duckTexH };
                DrawTexturePro(texDinoSheet, dinoSrc, dest, (Vector2){0,0}, 0.0f, WHITE);
            } else {
                // Push to bottom of physics box so feet touch the ground
                Rectangle dest = { dino.position.x, dino.position.y + (dino.size.y - texH),
                                   texW, texH };
                DrawTexturePro(texDinoSheet, dinoSrc, dest, (Vector2){0,0}, 0.0f, WHITE);
            }

            // Speed lines at high speed
            if (globalSpeedMultiplier > 1.8f) {
                unsigned char alpha = (unsigned char)((globalSpeedMultiplier-1.8f)/1.2f*100.0f);
                for (int i = 0; i < 5; i++) {
                    int ly   = gY - GetRandomValue(8,55);
                    int llen = GetRandomValue(16,52);
                    DrawLine((int)dino.position.x-llen-8, ly,
                             (int)dino.position.x-5,      ly,
                             (Color){255,255,255,alpha});
                }
            }

            // HUD
            DrawText(TextFormat("SCORE: %05d", score), GAME_WIDTH-180, 12, 20, (Color){255,230,50,255});
            DrawText(TextFormat("HI: %05d",    hiScore), GAME_WIDTH-154, 34, 16, (Color){185,225,255,185});
            if (globalSpeedMultiplier > 1.3f)
                DrawText(TextFormat("x%.1f", globalSpeedMultiplier), 14, 12, 16, (Color){255,175,50,215});
            if (score < 20)
                DrawText("SPACE: Jump  |  DOWN: Duck", 14, GAME_HEIGHT-46, 13, (Color){205,225,255,165});
        }
        // ══════════════════════════════════════════════════
        else if (currentScreen == SCREEN_GAME_OVER)
        {
            int sx = deathShake > 0 ? GetRandomValue(-5,5) : 0;
            int sy = deathShake > 0 ? GetRandomValue(-3,3) : 0;

            DrawRectangle(0,0,GAME_WIDTH,GAME_HEIGHT,(Color){0,0,0,105});
            DrawCard((Rectangle){GAME_WIDTH/2.0f-165+sx,105+sy,330,220},
                     (Color){12,20,50,235},(Color){255,80,80,185},0.1f);
            DrawText("GAME OVER",
                     GAME_WIDTH/2-MeasureText("GAME OVER",46)/2+sx,120+sy,46,
                     (Color){255,80,80,255});
            DrawText(TextFormat("Score: %d", score),
                     GAME_WIDTH/2-MeasureText(TextFormat("Score: %d",score),28)/2+sx,
                     180+sy,28,(Color){255,225,55,255});
            if (score > 0 && score >= hiScore) {
                float glow = (sinf(GetTime()*6.0f)+1.0f)*0.5f;
                Color gc = LerpColor((Color){255,200,0,200},(Color){255,255,180,255},glow);
                DrawText("NEW HI-SCORE!",
                         GAME_WIDTH/2-MeasureText("NEW HI-SCORE!",18)/2+sx,218+sy,18,gc);
            }
            DrawText(TextFormat("Player: %s", playerName),
                     GAME_WIDTH/2-MeasureText(TextFormat("Player: %s",playerName),18)/2+sx,
                     244+sy,18,(Color){165,210,255,215});
            DrawText("SPACE / ENTER to continue",
                     GAME_WIDTH/2-MeasureText("SPACE / ENTER to continue",15)/2+sx,
                     282+sy,15,(Color){145,155,215,195});
        }
        // ══════════════════════════════════════════════════
        else if (currentScreen == SCREEN_LEADERBOARD)
        {
            DrawRectangle(0,0,GAME_WIDTH,GAME_HEIGHT,(Color){0,15,35,105});
            DrawCard((Rectangle){75,18,650,360},(Color){8,18,44,235},(Color){255,200,50,165},0.06f);

            const char *lbTitle = "LEADERBOARD";
            int ltW = MeasureText(lbTitle,38);
            DrawText(lbTitle, GAME_WIDTH/2-ltW/2, 34, 38, (Color){255,205,50,255});
            DrawLine(GAME_WIDTH/2-ltW/2,78,GAME_WIDTH/2+ltW/2,78,(Color){255,200,50,120});

            DrawText("RANK", 138, 94, 15, (Color){165,190,255,185});
            DrawText("NAME", 228, 94, 15, (Color){165,190,255,185});
            DrawText("SCORE",530, 94, 15, (Color){165,190,255,185});

            if (loadedScoresCount == 0) {
                DrawText("No scores yet — go play!",
                         GAME_WIDTH/2-MeasureText("No scores yet — go play!",22)/2,
                         195,22,(Color){165,170,215,215});
            } else {
                Color rcs[5] = {
                    {255,215,  0,255},{192,192,192,255},{205,127,50,255},
                    {185,210,255,205},{165,185,225,185}
                };
                const char *medals[3] = {"1st","2nd","3rd"};
                for (int i = 0; i < loadedScoresCount; i++) {
                    int rowY = 112 + i*46;
                    Color rc = rcs[i<5?i:4];
                    DrawRectangleRounded((Rectangle){100,(float)rowY-4,600,38},
                                        0.3f,4,(Color){rc.r,rc.g,rc.b,18});
                    if (i < 3) DrawText(medals[i], 112, rowY, 16, rc);
                    DrawText(TextFormat("#%d",i+1), 148, rowY, 22, rc);
                    DrawText(top5[i].name, 228, rowY, 22, WHITE);
                    const char *ss = TextFormat("%d pts", top5[i].score);
                    DrawText(ss, 640-MeasureText(ss,22), rowY, 22, rc);
                }
            }

            Button btnBack = {{GAME_WIDTH/2.0f-90,382,180,45},"BACK",
                {80,80,80,220},{120,120,120,255},WHITE,20};
            DrawButton(btnBack, mouse, false);
        }

        EndScissorMode();
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
            Rectangle srcRec  = {0,0,(float)target.texture.width,-(float)target.texture.height};
            Rectangle destRec = {
                (GetScreenWidth()  - GAME_WIDTH  * scale) * 0.5f,
                (GetScreenHeight() - GAME_HEIGHT * scale) * 0.5f,
                GAME_WIDTH * scale, GAME_HEIGHT * scale
            };
            DrawTexturePro(target.texture, srcRec, destRec, (Vector2){0,0}, 0.0f, WHITE);
        EndDrawing();
    }

    // ── Cleanup ───────────────────────────────────────────
    StopMusicStream(bgMusic);
    UnloadMusicStream(bgMusic);
    UnloadSound(sfxJump);
    UnloadSound(sfxDeath);
    UnloadSound(sfxScore);
    UnloadTexture(texDinoSheet);
    UnloadTexture(texUnicorn);
    UnloadTexture(texFish);
    UnloadTexture(texEye);
    UnloadTexture(texTreeFar);
    UnloadTexture(texTileGrass);
    UnloadTexture(texTileDirt);
    UnloadTexture(texTileDirt2);
    UnloadRenderTexture(target);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}