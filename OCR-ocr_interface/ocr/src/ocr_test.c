#include "neural_net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define INPUT_SIZE 1024 // ✅ Corrigé pour 32x32
#define OUTPUT_SIZE 26
#define MAX_LINE 8192 // La ligne doit contenir 1024 * ~8 chars/float

int main(void) {
    NeuralNetwork *nn = load_network("trained_network.bin");
    if (!nn) {
        printf("❌ Impossible de charger le réseau entraîné.\n");
        return 1;
    }

    // ✅ Pointe vers le fichier de données plat correct
    FILE *f = fopen("data/alphabet_train_augmented", "r");
    if (!f) {
        perror("❌ Impossible d'ouvrir le fichier de données");
        free_network(nn);
        return 1;
    }

    char expected_label;
    float input_example[INPUT_SIZE];
    int sample_index = 0;
    int correct_count = 0;
    
    printf("--- Test des prédictions du réseau (Input Size: %d) ---\n", INPUT_SIZE);

    // Boucle pour lire toutes les lignes du dataset
    while (fscanf(f, " %c", &expected_label) == 1) {
        
        int count = 0;
        int error = 0;

        // Lire tous les 1024 pixels (flottants)
        for (count = 0; count < INPUT_SIZE; count++) {
            // ✅ Utilise %f pour lire un float, comme dans le fichier généré
            if (fscanf(f, "%f", &input_example[count]) != 1) {
                printf("❌ Erreur de lecture à l'échantillon %d. Pixels manquants.\n", sample_index + 1);
                error = 1;
                break;
            }
        }
        
        if (error) break;
        
        // --- Prédiction ---
        char result = predict_character(nn, input_example);

        if (result == expected_label) {
            printf("✅ Échantillon %d: Prédit %c (Attendu %c)\n", sample_index + 1, result, expected_label);
            correct_count++;
        } else {
            printf("❌ Échantillon %d: Prédit %c (Attendu %c)\n", sample_index + 1, result, expected_label);
        }

        sample_index++;
    }

    printf("\n--- Résultat du test ---\n");
    printf("Précision totale: %d/%d (%.2f%%)\n", correct_count, sample_index, (float)correct_count / sample_index * 100.0);

    fclose(f);
    free_network(nn);
    return 0;
}
