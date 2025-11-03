#include "neural_net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_SIZE 64
#define MAX_LINE 1024

int main(void) {
    NeuralNetwork *nn = load_network("trained_network.bin");
    if (!nn) {
        printf("❌ Impossible de charger le réseau entraîné.\n");
        return 1;
    }

    FILE *f = fopen("data/alphabet_train", "r");
    if (!f) {
        perror("Impossible d'ouvrir le fichier alphabet_train");
        free_network(nn);
        return 1;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        char *token = strtok(line, " \n\t");
        if (!token) continue;

        char letter = token[0]; // première colonne = lettre

        float input_example[INPUT_SIZE];
        int count = 0;

        // Lire tous les pixels
        while ((token = strtok(NULL, " \n\t")) != NULL && count < INPUT_SIZE) {
            input_example[count++] = (float)atoi(token);
        }

        if (count != INPUT_SIZE) {
            printf("❌ Nombre de pixels incorrect pour %c\n", letter);
            continue;
        }

        char result = predict_character(nn, input_example);
        printf("🧠 Le réseau a prédit : %c (pour la lettre %c)\n", result, letter);
    }

    fclose(f);
    free_network(nn);
    return 0;
}
