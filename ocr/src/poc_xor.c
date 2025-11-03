#include "neural_net.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    // 🔹 Création du réseau : 2 entrées, 4 neurones cachés, 1 sortie
    NeuralNetwork *nn = init_network(2, 4, 1);

    // 🔹 Données d'entraînement pour la fonction XNOR
    // XNOR = 1 si les deux bits sont identiques (A.B + ¬A.¬B)
    float inputs[4][2] = {
        {0, 0},
        {0, 1},
        {1, 0},
        {1, 1}
    };

    float targets[4][1] = {
        {1},  // 0,0 -> 1
        {0},  // 0,1 -> 0
        {0},  // 1,0 -> 0
        {1}   // 1,1 -> 1
    };

    // 🔹 Conversion pour le réseau
    float *train_inputs[4];
    float *train_targets[4];
    for (int i = 0; i < 4; i++) {
        train_inputs[i] = inputs[i];
        train_targets[i] = targets[i];
    }

    printf("🔧 Entraînement du réseau de neurones sur la fonction XNOR...\n");

    // 🔹 Boucle d'entraînement
    for (int epoch = 1; epoch <= 1000; epoch++) {
        long start = get_time_ms();
        train_epoch(nn, train_inputs, train_targets, 4, 0.5f);
        long end = get_time_ms();
        printf("Epoch %d/30 terminée (%ld ms)\n", epoch, end - start);
    }

    // 🔹 Test du modèle entraîné
    printf("\n🧠 Résultats finaux :\n");
    for (int i = 0; i < 4; i++) {
        forward_pass(nn, inputs[i]);
        printf("Entrée (%.0f, %.0f) → %.3f (attendu: %.0f)\n",
               inputs[i][0], inputs[i][1], nn->output[0], targets[i][0]);
    }

    // 🔹 Sauvegarde du modèle (optionnel)
    save_network(nn, "trained_xnor.bin");

    free_network(nn);
    return 0;
}

