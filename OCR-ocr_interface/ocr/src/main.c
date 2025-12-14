#include "../include/neural_net.h"
#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

// --- PARAMÈTRES DE BASE (AJUSTÉS POUR VOTRE DATASET) ---
#define INPUT_DIM 1024       // 32 * 32 pixels
#define HIDDEN_DIM 256        // Taille de la couche cachée
#define OUTPUT_DIM ALPHABET_SIZE // 26 lettres
#define MAX_SAMPLES 450      // Nombre EXACT de vos échantillons (lettres)

// --- Chemin vers le dataset généré par Python ---
#define TRAIN_DATA_FILE "data/alphabet_train_augmented"
#define TRAIN_SPLIT_RATIO 0.8 // 80% pour l'entraînement, 20% pour la validation

// =========================================================================
// Fonction pour charger les données (adaptée pour le format 1024 floats)
// =========================================================================
int load_dataset(const char *filename, float ***inputs, float ***targets, int *sample_count) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("❌ Erreur: Impossible d'ouvrir le fichier de données");
        return 0;
    }

    // Allocation pour les pointeurs
    float **in_data = malloc(MAX_SAMPLES * sizeof(float*));
    float **tar_data = malloc(MAX_SAMPLES * sizeof(float*));
    if (!in_data || !tar_data) {
        printf("❌ Erreur fatale: Échec de l'allocation pour les pointeurs de dataset.\n");
        if (in_data) free(in_data);
        if (tar_data) free(tar_data);
        fclose(f);
        return 0;
    }

    int count = 0;
    char letter;
    // Boucle de lecture: s'arrête si le fichier se termine OU si MAX_SAMPLES est atteint
    while (fscanf(f, " %c", &letter) == 1 && count < MAX_SAMPLES) {
        // Allocation de la mémoire pour l'entrée (1024 floats) et la sortie (26 floats)
        in_data[count] = malloc(INPUT_DIM * sizeof(float));
        tar_data[count] = calloc(OUTPUT_DIM, sizeof(float)); 
        
        // Mise en place du label cible (One-Hot Encoding)
        if (letter >= 'A' && letter <= 'Z') {
             tar_data[count][letter - 'A'] = 1.0f;
        } else {
            // Gérer les caractères invalides (bien que generate_dataset.py devrait l'empêcher)
            free(in_data[count]);
            free(tar_data[count]);
            continue;
        }

        // Lecture des 1024 pixels (float)
        for (int i = 0; i < INPUT_DIM; i++) {
            if (fscanf(f, "%f", &in_data[count][i]) != 1) {
                printf("❌ Erreur de formatage à la ligne %d (pixel %d)\n", count + 1, i + 1);
                // Libération de la mémoire et sortie d'erreur...
                fclose(f);
                return 0;
            }
        }

        count++;
    }
    
    // Fermeture du fichier
    fclose(f);
    
    // Définition des pointeurs de sortie
    *inputs = in_data;
    *targets = tar_data;
    *sample_count = count;

    return 1;
}

// =========================================================================
// Fonction principale
// =========================================================================
int main(void) {
    // --- 1. Initialisation ou Chargement du réseau ---
    int input_size = INPUT_DIM;
    int hidden_size = HIDDEN_DIM;
    int output_size = OUTPUT_DIM;

    // Tente de charger un réseau existant pour reprendre l'entraînement
    NeuralNetwork *nn = load_network("trained_network.bin");
    if (!nn) {
        printf("⚠️ Aucun réseau existant (trained_network.bin) trouvé. Initialisation d'un nouveau réseau...\n");
        nn = init_network(input_size, hidden_size, output_size);
    } else {
        printf("✅ Reprise de l'entraînement avec le réseau chargé...\n");
    }
    if (!nn) return 1;

    // --- 2. Chargement du dataset ---
    float **all_inputs;
    float **all_targets;
    int total_samples;

    if (!load_dataset(TRAIN_DATA_FILE, &all_inputs, &all_targets, &total_samples)) {
        free_network(nn);
        return 1;
    }
    
    // --- 3. Séparation en ensembles d'entraînement (Train) et de validation (Test) ---
    // Utilise 80% des données pour l'entraînement et 20% pour la validation
    int train_count = (int)floor(total_samples * TRAIN_SPLIT_RATIO); // 450 * 0.8 = 360
    int test_count = total_samples - train_count;               // 450 - 360 = 90

    float **train_inputs = all_inputs;
    float **train_targets = all_targets;
    float **test_inputs = all_inputs + train_count;
    float **test_targets = all_targets + train_count;

    printf("\n📊 Dataset chargé: %d échantillons au total.\n", total_samples);
    printf("   - Entraînement (Train): %d échantillons.\n", train_count);
    printf("   - Validation (Test): %d échantillons.\n", test_count);

    // --- 4. Paramètres d'entraînement ---
    float lr = 0.05f;  // Learning Rate initial (Augmenté pour sortir du plateau)
    int epochs = 3000; // Nombre d'époques pour la nouvelle session
    
    // Variables pour la sélection du meilleur modèle (Early Stopping)
    float best_accuracy = 0.0f; 
    int best_epoch = 0;
    
    // Calculer la précision initiale du modèle chargé pour démarrer la comparaison
    int correct_initial = validate(nn, test_inputs, test_targets, test_count);
    best_accuracy = (float)correct_initial / test_count;
    if (best_accuracy > 0.0f) {
        printf("🎯 Précision de départ (modèle chargé): %.2f%%\n", best_accuracy * 100.0f);
        best_epoch = -1; // Indique que c'est une précision pré-entraînement
    }


    printf("\n🔧 Début de l'entraînement sur %d époques (LR initial: %.4f)...\n", epochs, lr);

    // --- 5. Boucle d'entraînement ---
    for (int epoch = 1; epoch <= epochs; epoch++) {
        long start = get_time_ms();
        
        // Entraînement sur la partie "train"
        train_epoch(nn, train_inputs, train_targets, train_count, lr);
        
        // Validation sur la partie "test"
        int correct = validate(nn, test_inputs, test_targets, test_count);
        float current_accuracy = (float)correct / test_count;

        long end = get_time_ms();
        
        printf("Epoch %d/%d terminée (%ld ms). Précision Validation: %d/%d (%.2f%%)", 
               epoch, epochs, end - start, correct, test_count, current_accuracy * 100.0f);
        
        // --- Logique de Sauvegarde Conditionnelle (Early Stopping) ---
        if (current_accuracy > best_accuracy) {
            best_accuracy = current_accuracy;
            best_epoch = epoch;
            
            // 💾 SAUVEGARDE DU NOUVEAU MEILLEUR MODÈLE
            save_network(nn, "trained_network.bin");
            printf(" >>> NOUVEAU MEILLEUR MODÈLE SAUVEGARDÉ (%.2f%%)", best_accuracy * 100.0f);
        }
        printf("\n");
        
        // 💡 Ajuster le Learning Rate pour aider à sortir des minima locaux
        if (epoch % 200 == 0) { // Réduit le LR tous les 200 époques
            lr *= 0.7f; 
            printf("--- Learning Rate ajusté à: %.4f ---\n", lr); 
        }
    }

    // --- 6. Fin et Libération ---
    printf("\n✅ Entraînement terminé. Meilleure précision obtenue: %.2f%% à l'Époque %d.\n", 
           best_accuracy * 100.0f, best_epoch);

    // Libération de la mémoire du dataset
    for (int i = 0; i < total_samples; i++) {
        free(all_inputs[i]);
        free(all_targets[i]);
    }
    free(all_inputs);
    free(all_targets);

    // Libération de la mémoire du réseau
    free_network(nn);

    return 0;
}
