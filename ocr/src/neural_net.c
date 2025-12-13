#include "neural_net.h"
#include <stdio.h>
#include <math.h>
#include <float.h>
NeuralNetwork *init_network(int input_size, int hidden_size, int output_size) {
    NeuralNetwork *nn = malloc(sizeof(NeuralNetwork));
    nn->input_size = input_size;
    nn->hidden_size = hidden_size;
    nn->output_size = output_size;
	nn->output = malloc(sizeof(float) * output_size);
    // Allocation mémoire
    nn->hidden_weights = malloc(sizeof(float) * input_size * hidden_size);
    nn->output_weights = malloc(sizeof(float) * hidden_size * output_size);
    nn->hidden_bias = malloc(sizeof(float) * hidden_size);
    nn->output_bias = malloc(sizeof(float) * output_size);

    // Initialisation aléatoire
    srand((unsigned int)time(NULL));
    for (int i = 0; i < input_size * hidden_size; i++)
        nn->hidden_weights[i] = ((float)rand() / RAND_MAX - 0.5f) * 2.0f;
    for (int i = 0; i < hidden_size * output_size; i++)
        nn->output_weights[i] = ((float)rand() / RAND_MAX - 0.5f) * 2.0f;
    for (int i = 0; i < hidden_size; i++)
        nn->hidden_bias[i] = 0.0f;
    for (int i = 0; i < output_size; i++)
        nn->output_bias[i] = 0.0f;

    return nn;
}

void free_network(NeuralNetwork *nn) {
    free(nn->hidden_weights);
    free(nn->output_weights);
    free(nn->hidden_bias);
    free(nn->output_bias);
    free(nn->output);
    free(nn);
}

float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}

float sigmoid_derivative(float x) {
    return x * (1.0f - x);
}

void forward_pass(NeuralNetwork *nn, float *inputs) {
    float hidden[nn->hidden_size];

    // Couche cachée
    for (int h = 0; h < nn->hidden_size; h++) {
        float sum = nn->hidden_bias[h];
        for (int i = 0; i < nn->input_size; i++)
            sum += inputs[i] * nn->hidden_weights[i * nn->hidden_size + h];
        hidden[h] = sigmoid(sum);
    }

    // Couche de sortie (on stocke dans nn->output)
    for (int o = 0; o < nn->output_size; o++) {
        float sum = nn->output_bias[o];
        for (int h = 0; h < nn->hidden_size; h++)
            sum += hidden[h] * nn->output_weights[h * nn->output_size + o];
        nn->output[o] = sigmoid(sum); // ✅ enregistre directement dans le réseau
    }
}
int validate(NeuralNetwork *nn, float **test_inputs, float **test_targets, int sample_count) {
    int correct = 0;
    for (int s = 0; s < sample_count; s++) {
        forward_pass(nn, test_inputs[s]);

        // Trouver l'indice de la sortie prédite (le neurone avec le score le plus élevé)
        int predicted_index = 0;
        float max_output = nn->output[0];

        for (int i = 1; i < nn->output_size; i++) {
            if (nn->output[i] > max_output) {
                max_output = nn->output[i];
                predicted_index = i;
            }
        }

        // Trouver l'indice de la sortie attendue (le label cible)
        int target_index = 0;
        for (int i = 0; i < nn->output_size; i++) {
            if (test_targets[s][i] == 1.0f) { // Les cibles sont codées en One-Hot (0.0 ou 1.0)
                target_index = i;
                break;
            }
        }

        // Comparaison
        if (predicted_index == target_index) {
            correct++;
        }
    }
    return correct;
}




void train_epoch(NeuralNetwork *nn, float **train_inputs, float **train_targets, int sample_count, float lr) {
    float total_loss = 0.0f;

    for (int s = 0; s < sample_count; s++) {
        float *inputs = train_inputs[s];
        float *targets = train_targets[s];

        // --- 1. Forward pass ---
        float hidden[nn->hidden_size];
        float outputs[nn->output_size];

        for (int h = 0; h < nn->hidden_size; h++) {
            float sum = nn->hidden_bias[h];
            for (int i = 0; i < nn->input_size; i++)
                sum += inputs[i] * nn->hidden_weights[i * nn->hidden_size + h];
            hidden[h] = sigmoid(sum);
        }

        for (int o = 0; o < nn->output_size; o++) {
            float sum = nn->output_bias[o];
            for (int h = 0; h < nn->hidden_size; h++)
                sum += hidden[h] * nn->output_weights[h * nn->output_size + o];
            outputs[o] = sigmoid(sum);
        }

        // --- 2. Calcul des erreurs ---
        float output_errors[nn->output_size];
        for (int o = 0; o < nn->output_size; o++) {
            output_errors[o] = targets[o] - outputs[o];
            total_loss += output_errors[o] * output_errors[o]; // MSE
        }

        // --- 3. Backpropagation ---
        float hidden_errors[nn->hidden_size];
        for (int h = 0; h < nn->hidden_size; h++) {
            float err = 0.0f;
            for (int o = 0; o < nn->output_size; o++)
                err += output_errors[o] * nn->output_weights[h * nn->output_size + o];
            hidden_errors[h] = err * sigmoid_derivative(hidden[h]);
        }

        // --- 4. Mise à jour des poids sortie ---
        for (int h = 0; h < nn->hidden_size; h++) {
            for (int o = 0; o < nn->output_size; o++) {
                float delta = lr * output_errors[o] * sigmoid_derivative(outputs[o]) * hidden[h];
                nn->output_weights[h * nn->output_size + o] += delta;
            }
        }

        // --- 5. Mise à jour des biais sortie ---
        for (int o = 0; o < nn->output_size; o++)
            nn->output_bias[o] += lr * output_errors[o] * sigmoid_derivative(outputs[o]);

        // --- 6. Mise à jour des poids cachés ---
        for (int i = 0; i < nn->input_size; i++) {
            for (int h = 0; h < nn->hidden_size; h++) {
                float delta = lr * hidden_errors[h] * inputs[i];
                nn->hidden_weights[i * nn->hidden_size + h] += delta;
            }
        }

        // --- 7. Mise à jour des biais cachés ---
        for (int h = 0; h < nn->hidden_size; h++)
            nn->hidden_bias[h] += lr * hidden_errors[h];
    }

    // --- 8. Affichage de la loss moyenne ---
    float mean_loss = total_loss / sample_count;
    printf("   -> Mean loss: %.6f\n", mean_loss);
}
void save_network(NeuralNetwork *nn, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) {
        perror("Erreur ouverture fichier sauvegarde");
        return;
    }

    fwrite(&nn->input_size, sizeof(int), 1, f);
    fwrite(&nn->hidden_size, sizeof(int), 1, f);
    fwrite(&nn->output_size, sizeof(int), 1, f);

    fwrite(nn->hidden_weights, sizeof(float), nn->input_size * nn->hidden_size, f);
    fwrite(nn->output_weights, sizeof(float), nn->hidden_size * nn->output_size, f);
    fwrite(nn->hidden_bias, sizeof(float), nn->hidden_size, f);
    fwrite(nn->output_bias, sizeof(float), nn->output_size, f);

    fclose(f);
    printf("✅ Réseau sauvegardé dans %s\n", filename);
}

NeuralNetwork *load_network(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("⚠️  Impossible d'ouvrir le fichier réseau — nouveau réseau créé.");
        return NULL;
    }

    int in, hid, out;
    fread(&in, sizeof(int), 1, f);
    fread(&hid, sizeof(int), 1, f);
    fread(&out, sizeof(int), 1, f);

    NeuralNetwork *nn = init_network(in, hid, out);

    fread(nn->hidden_weights, sizeof(float), in * hid, f);
    fread(nn->output_weights, sizeof(float), hid * out, f);
    fread(nn->hidden_bias, sizeof(float), hid, f);
    fread(nn->output_bias, sizeof(float), out, f);

    fclose(f);
    printf("✅ Réseau chargé depuis %s\n", filename);
    return nn;
}
char predict_character(NeuralNetwork *nn, float *inputs)
{
    forward_pass(nn, inputs);

    // Trouver l’indice de la sortie la plus élevée
    int best_index = 0;
    float best_value = -FLT_MAX;
    for (int i = 0; i < nn->output_size; i++) {
        if (nn->output[i] > best_value) {
            best_value = nn->output[i];
            best_index = i;
        }
    }

    // Convertir l’indice (0–25) en lettre ASCII
    return 'A' + best_index;
}

