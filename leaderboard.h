#ifndef LEADERBOARD_H
#define LEADERBOARD_H

// Aceasta este partea care ii spune compilatorului ce e ala un "PlayerScore"
typedef struct {
    char name[20];
    int score;
} PlayerScore;

void SaveScoreToCSV(const char* name, int score);

// Functia care foloseste structura de mai sus
int LoadTopScores(PlayerScore topScores[], int maxScores);



int LoadTopScores(PlayerScore topScores[], int maxScores);

#endif