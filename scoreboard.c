#include "scoreboard.h"

#include <stdio.h>

void SaveScoreToCSV(const char *playerName, int score)
{
    FILE *file = fopen("leaderboard.csv", "a");
    if (file == NULL) {
        return;
    }

    fprintf(file, "%s,%d\n", playerName, score);
    fclose(file);
}