#include "leaderboard.h"
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void SaveScoreToCSV(const char* name, int score) {
    FILE *file = fopen("leaderboard.csv", "a");
    if (file != NULL) {
        fprintf(file, "%s,%d\n", name, score);
        fclose(file);
        TraceLog(LOG_INFO, "MAGIE: Am salvat scorul %d pentru %s!", score, name);
    } else {
        TraceLog(LOG_ERROR, "CRITIC: Nu am putut crea fisierul leaderboard.csv!");
    }
}

int LoadTopScores(PlayerScore topScores[], int maxScores) {
    FILE *file = fopen("leaderboard.csv", "r"); // "r" pentru read
    if (file == NULL) return 0; // Fisierul e gol sau nu exista

    PlayerScore allScores[1000]; // Citim maxim 1000 de inregistrari
    int count = 0;
    char line[100];

    // Citim linie cu linie
    while (fgets(line, sizeof(line), file) != NULL && count < 1000) {
        char *nameToken = strtok(line, ",");
        char *scoreToken = strtok(NULL, "\n");
        
        if (nameToken != NULL && scoreToken != NULL) {
            strncpy(allScores[count].name, nameToken, 19);
            allScores[count].name[19] = '\0';
            allScores[count].score = atoi(scoreToken); // convertim text in int
            count++;
        }
    }
    fclose(file);

    // BUBBLE SORT - Ordonam scorurile de la mare la mic
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (allScores[j].score < allScores[j+1].score) {
                PlayerScore temp = allScores[j];
                allScores[j] = allScores[j+1];
                allScores[j+1] = temp;
            }
        }
    }

    // Returnam doar cate i-am cerut (ex: Top 5)
    int returnCount = (count < maxScores) ? count : maxScores;
    for (int i = 0; i < returnCount; i++) {
        topScores[i] = allScores[i];
    }

    return returnCount;
}