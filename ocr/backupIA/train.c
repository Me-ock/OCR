#include <ctype.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>
#include "defs.h"
#include "learn.h"
#include "save_load.h"
#include "neural_network.h"

// Attention : Ne mettre ces defines que dans UN SEUL fichier .c du projet
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

// --- GESTION OPTIMISÉE DES FICHIERS ---

// Structure pour garder la liste des fichiers en cache
typedef struct {
    char **paths;
    int count;
    // Split indices
    int *train_indices;
    int train_count;
    int *val_indices;
    int val_count;
} FileList;

FileList *global_file_list = NULL;

void shuffle_indices(int *array, int n) {
    if (n > 1) {
        for (int i = 0; i < n - 1; i++) {
            int j = i + rand() / (RAND_MAX / (n - i) + 1);
            int t = array[j];
            array[j] = array[i];
            array[i] = t;
        }
    }
}

void load_file_list(const char *folder) {
    if (global_file_list != NULL) return; // Déjà chargé

    global_file_list = malloc(sizeof(FileList));
    global_file_list->paths = NULL;
    global_file_list->count = 0;

    DIR *dir = opendir(folder);
    if (!dir) {
        perror("Impossible d'ouvrir le dossier d'entrainement");
        exit(1);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue; // Ignorer . et ..

        // Construction du chemin complet
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", folder, entry->d_name);

        // Ajout à la liste
        global_file_list->paths = realloc(global_file_list->paths, sizeof(char*) * (global_file_list->count + 1));
        global_file_list->paths[global_file_list->count] = strdup(path);
        global_file_list->count++;
    }
    closedir(dir);
    printf("Dataset chargé : %d images trouvées dans '%s'.\n", global_file_list->count, folder);

    // Create Train/Validation Split (80/20)
    int *indices = malloc(sizeof(int) * global_file_list->count);
    for(int i=0; i<global_file_list->count; i++) indices[i] = i;
    
    shuffle_indices(indices, global_file_list->count);
    
    global_file_list->val_count = global_file_list->count * 0.2; // 20% validation
    global_file_list->train_count = global_file_list->count - global_file_list->val_count;
    
    global_file_list->val_indices = malloc(sizeof(int) * global_file_list->val_count);
    global_file_list->train_indices = malloc(sizeof(int) * global_file_list->train_count);
    
    for(int i=0; i<global_file_list->val_count; i++) 
        global_file_list->val_indices[i] = indices[i];
        
    for(int i=0; i<global_file_list->train_count; i++) 
        global_file_list->train_indices[i] = indices[global_file_list->val_count + i];
        
    free(indices);
    printf("Split: %d Train / %d Validation\n", global_file_list->train_count, global_file_list->val_count);
}

char* get_random_train_file() {
    if (!global_file_list || global_file_list->train_count == 0) return NULL;
    int idx = rand() % global_file_list->train_count;
    return global_file_list->paths[global_file_list->train_indices[idx]];
}

char* get_val_file(int index) {
    if (!global_file_list || index >= global_file_list->val_count) return NULL;
    return global_file_list->paths[global_file_list->val_indices[index]];
}

// --- TRAITEMENT D'IMAGE ---

void image_to_double64(const char *input_path, double out[64 * 64]) {
    int width, height, channels;
    
    // Force le chargement en 1 seul canal (Gris) pour simplifier
    unsigned char *img = stbi_load(input_path, &width, &height, &channels, 1);
    
    if (img == NULL) {
        fprintf(stderr, "Erreur chargement image : %s\n", input_path);
        exit(1);
    }

    // Initialize with 0
    for(int i=0; i<64*64; i++) out[i] = 0.0;

    // Calculate scale to fit in 64x64 while preserving aspect ratio
    float scale_x = 64.0f / width;
    float scale_y = 64.0f / height;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;

    int new_width = (int)(width * scale);
    int new_height = (int)(height * scale);

    // Center the image
    int offset_x = (64 - new_width) / 2;
    int offset_y = (64 - new_height) / 2;

    // Resize to 64x64 using nearest neighbor with aspect ratio preservation
    for (int y = 0; y < new_height; y++) {
        for (int x = 0; x < new_width; x++) {
            // Map target pixel to source pixel
            int src_x = (int)((float)x / (float)new_width * width);
            int src_y = (int)((float)y / (float)new_height * height);

            // Handle edge cases
            if (src_x >= width) src_x = width - 1;
            if (src_y >= height) src_y = height - 1;
            if (src_x < 0) src_x = 0;
            if (src_y < 0) src_y = 0;

            int src_index = src_y * width + src_x; // 1 channel

            unsigned char value = img[src_index];

            // Normalize to 0.0 - 1.0 AND INVERT
            // We want Text (Black=0) to be 1.0 (Active)
            // We want Background (White=255) to be 0.0 (Inactive)
            out[(y + offset_y) * 64 + (x + offset_x)] = 1.0 - ((double)value / 255.0);
        }
    }

    stbi_image_free(img);
}

int extract_label_from_filename(const char *filepath) {
    // Le format est ".../dataset/124a.png"
    // On cherche le dernier point
    const char *dot = strrchr(filepath, '.');
    if (!dot || dot == filepath) return -1;

    // Le caractère juste avant le point est la lettre
    char letter_char = *(dot - 1);
    
    // Conversion en minuscule au cas où, puis en index 0-25
    return tolower(letter_char) - 'a';
}

// --- ENTRAINEMENT ---

void rotate_image_double(double *img, double angle_rad) {
    double *temp = malloc(64 * 64 * sizeof(double));
    // Initialize with background color (0.0)
    for(int i=0; i<64*64; i++) temp[i] = 0.0;

    double cx = 31.5; // Center of 64x64
    double cy = 31.5;
    double cos_a = cos(angle_rad);
    double sin_a = sin(angle_rad);

    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            // Inverse mapping to find source pixel
            double dx = x - cx;
            double dy = y - cy;
            
            // Rotation formula (inverse)
            int src_x = (int)(dx * cos_a + dy * sin_a + cx);
            int src_y = (int)(-dx * sin_a + dy * cos_a + cy);

            if (src_x >= 0 && src_x < 64 && src_y >= 0 && src_y < 64) {
                temp[y * 64 + x] = img[src_y * 64 + src_x];
            }
        }
    }
    
    for(int i=0; i<64*64; i++) img[i] = temp[i];
    free(temp);
}

void scale_image_double(double *img, double scale_factor) {
    double *temp = malloc(64 * 64 * sizeof(double));
    // Initialize with background color (0.0)
    for(int i=0; i<64*64; i++) temp[i] = 0.0;

    double cx = 31.5;
    double cy = 31.5;

    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            // Inverse mapping
            double src_x_d = (x - cx) / scale_factor + cx;
            double src_y_d = (y - cy) / scale_factor + cy;
            
            int src_x = (int)src_x_d;
            int src_y = (int)src_y_d;

            if (src_x >= 0 && src_x < 64 && src_y >= 0 && src_y < 64) {
                temp[y * 64 + x] = img[src_y * 64 + src_x];
            }
        }
    }
    for(int i=0; i<64*64; i++) img[i] = temp[i];
    free(temp);
}

void apply_random_transform(double *img) {
    // 1. Noise (Increased slightly to 2%)
    for (int i = 0; i < 64 * 64; i++) {
        if ((double)rand() / RAND_MAX < 0.02) { 
            img[i] = 1.0 - img[i];
        }
    }
    
    // 2. Rotation (+/- 10 degrees)
    double angle = ((double)rand() / RAND_MAX * 0.34) - 0.17;
    if (fabs(angle) > 0.01) {
        rotate_image_double(img, angle);
    }

    // 3. Scale (New: 0.9 to 1.1) - Helps with G vs C size differences
    double scale = 0.9 + ((double)rand() / RAND_MAX * 0.2);
    if (fabs(scale - 1.0) > 0.02) {
        scale_image_double(img, scale);
    }

    // 4. Shift (Increased to +/- 3 pixels)
    int shift_x = (rand() % 7) - 3; 
    int shift_y = (rand() % 7) - 3; 
    
    if (shift_x == 0 && shift_y == 0) return;
    
    double *temp = malloc(64 * 64 * sizeof(double));
    // Initialize with background color (0.0 usually)
    for(int i=0; i<64*64; i++) temp[i] = 0.0;
    
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 64; x++) {
            int nx = x + shift_x;
            int ny = y + shift_y;
            if (nx >= 0 && nx < 64 && ny >= 0 && ny < 64) {
                temp[ny * 64 + nx] = img[y * 64 + x];
            }
        }
    }
    
    for(int i=0; i<64*64; i++) img[i] = temp[i];
    free(temp);
}

void fill_input_and_expect_img(double **inputs, double **expected, int batch_size) {
    for (int i = 0; i < batch_size; i++) {
        char *file = get_random_train_file();
        
        // 1. INPUT
        double *input_vector = malloc(sizeof(double) * 64 * 64);
        image_to_double64(file, input_vector);
        
        // Apply augmentation
        apply_random_transform(input_vector);

        inputs[i] = input_vector;

        // 2. EXPECTED (One-Hot Encoding)
        double *target_vector = calloc(26, sizeof(double)); // calloc met tout à 0
        int label_idx = extract_label_from_filename(file);
        
        if (label_idx >= 0 && label_idx < 26) {
            target_vector[label_idx] = 1.0;
        } else {
            printf("Attention: Label invalide pour %s\n", file);
        }
        expected[i] = target_vector;
    }
}

double evaluate_validation(struct neural_network *n) {
    if (!global_file_list || global_file_list->val_count == 0) return 0.0;
    
    int correct = 0;
    int total = global_file_list->val_count;
    
    // Limit validation to 200 samples to be fast if dataset is huge
    if (total > 200) total = 200;

    for (int i = 0; i < total; i++) {
        char *file = get_val_file(i); // Or random val file
        
        double input[64 * 64];
        image_to_double64(file, input);
        // NO AUGMENTATION ON VALIDATION
        
        int predicted = classify(n, input);
        int expected = extract_label_from_filename(file);
        
        if (predicted == expected) correct++;
    }
    
    return (double)correct / total * 100.0;
}

void train_on_image(struct neural_network *n, char *folder,
    double learn_rate, int epoch_numbers,
    int mini_batch_size, int loop_per_mini_batch) {

    // 1. Charger la liste des fichiers UNE SEULE FOIS
    load_file_list(folder);

    if (global_file_list->count == 0) {
        printf("Erreur: Aucune image trouvée.\n");
        return;
    }

    double **inputs = malloc(sizeof(double*) * mini_batch_size);
    double **expect = malloc(sizeof(double*) * mini_batch_size);

    printf("Début de l'entraînement (%d epochs)...\n", epoch_numbers);

    double best_val_accuracy = 0.0;
    int stagnation_count = 0;
    const int PATIENCE_CHECKS = 50; // 50 * 10 = 500 epochs sans amélioration

    for (int epoch = 0; epoch < epoch_numbers; epoch++) {
        
        // Remplir le batch avec de nouvelles images aléatoires
        fill_input_and_expect_img(inputs, expect, mini_batch_size);

        // Apprentissage sur ce batch
        for (int j = 0; j < loop_per_mini_batch; j++) {
            learn_epoch_with_backpropagation(n, inputs, expect, mini_batch_size, learn_rate);
        }

        // Calcul de la précision tous les 10 epochs
        if (epoch % 10 == 0) {
            // Evaluate on VALIDATION set, not training batch
            double val_accuracy = evaluate_validation(n);
            
            printf("Epoch %d/%d - Val Accuracy: %.1f%% (Best: %.1f%%)\n", epoch, epoch_numbers, val_accuracy, best_val_accuracy);
            
            if (val_accuracy > best_val_accuracy) {
                best_val_accuracy = val_accuracy;
                stagnation_count = 0;
                // On sauvegarde le "champion"
                save_network("neural_network/network_best.bin", n);
            } else {
                stagnation_count++;
                
                // LEARNING RATE DECAY
                // Si on stagne depuis 15 checks (150 epochs), on divise le learning rate par 2
                if (stagnation_count % 15 == 0) {
                    learn_rate *= 0.5;
                    printf(">>> Stagnation détectée. Réduction du learning rate à %.6f\n", learn_rate);
                }
            }

            // ARRÊT SI 100% ATTEINT (Sur validation !)
            if (val_accuracy >= 99.5) { // 99.5 allows for 1 error in 200
                printf("\n=== SUCCÈS ===\n");
                printf("Précision de validation > 99.5%% atteinte ! Arrêt immédiat.\n");
                save_network("neural_network/network_best.bin", n); 
                break;
            }

            if (stagnation_count >= PATIENCE_CHECKS) {
                printf("\n=== ARRÊT ANTICIPÉ ===\n");
                printf("La précision n'a pas dépassé %.1f%% depuis %d epochs.\n", best_val_accuracy, PATIENCE_CHECKS * 10);
                printf("Fin de l'entraînement.\n");
                break;
            }

            save_network("neural_network/network_autosave.bin", n); 
        }

        // Nettoyage mémoire du batch
        for (int i = 0; i < mini_batch_size; i++) {
            free(inputs[i]);
            free(expect[i]);
        }
    }
    
    // Sauvegarde finale
    save_network("neural_network/network_final.bin", n);
    printf("Entraînement terminé.\n");

    // Libération optionnelle de la liste globale si fin du programme
    // (Pas strictement nécessaire ici car l'OS le fera, mais propre)
    free(inputs);
    free(expect);
}