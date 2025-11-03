#include "neural_net.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

int load_dataset(const char *filename, float ***inputs, float ***targets, int *sample_count) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("❌ Erreur ouverture fichier : %s\n", filename);
        return 0;
    }

    char letter;
    float **in = malloc(26 * sizeof(float*));
    float **tar = malloc(26 * sizeof(float*));
    int count = 0;

    while (fscanf(file, " %c", &letter) == 1 && count < 26) {
        in[count] = malloc(64 * sizeof(float));
        tar[count] = calloc(26, sizeof(float)); 
        tar[count][letter - 'A'] = 1.0f;

        for (int i = 0; i < 64; i++)
            fscanf(file, "%f", &in[count][i]);

        count++;
    }

    fclose(file);

    *inputs = in;
    *targets = tar;
    *sample_count = count;
    return 1;
}

int main(void) {
    // --- 1. Initialisation du réseau ---
    int input_size = 64;
    int hidden_size = 64;  // Augmentation du nombre de neurones cachés
    int output_size = ALPHABET_SIZE;
    NeuralNetwork *nn = init_network(input_size, hidden_size, output_size);

    // --- 2. Chargement du dataset ---
    float **train_inputs;
    float **train_targets;
    int sample_count;

    if (!load_dataset("data/alphabet_train_augmented", &train_inputs, &train_targets, &sample_count)) {
        return 1;
    }

    float **test_inputs = train_inputs;
    float **test_targets = train_targets;
    int test_count = sample_count;

    // --- 3. Boucle d'entraînement ---
    printf("🚀 Training started...\n");
    int epochs = 10000;
    float learning_rate = 0.05f;

    for (int epoch = 1; epoch <= epochs; epoch++) {
        long start = get_time_ms();

        train_epoch(nn, train_inputs, train_targets, sample_count, learning_rate);
        int correct = validate(nn, test_inputs, test_targets, test_count);

        long end = get_time_ms();
        printf("Epoch %d/%d completed! (%ldms). Accuracy: %d/%d\n",
               epoch, epochs, end - start, correct, test_count);
    }

    // --- 4. Sauvegarde du modèle entraîné ---
    save_network(nn, "trained_network.bin");

    // --- 5. Libération mémoire ---
    for (int i = 0; i < sample_count; i++) {
        free(train_inputs[i]);
        free(train_targets[i]);
    }
    free(train_inputs);
    free(train_targets);
    free_network(nn);

    printf("✅ Entraînement terminé et réseau sauvegardé !\n");
    return 0;
}
