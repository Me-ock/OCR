#include "../include/neural_net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

// Définitions
#define INPUT_SIZE 1024
#define MAX_WORDS 20    
#define MAX_WORD_LEN 15 
#define INPUT_FILE "decoupage/decoupage_output.txt" 
#define GRID_OUTPUT_FILE "grid.txt"
#define WORDS_OUTPUT_FILE "words.txt"
#define INITIAL_GRID_SIZE 10 // Taille d'allocation initiale

// Structure pour stocker l'info d'une seule lettre
typedef struct {
    char character;
    int x; // Colonne (0-indexée)
    int y; // Ligne (0-indexée)
} GridLetter;

// Fonction utilitaire pour libérer la mémoire de la grille 2D
void free_grid_matrix(char **grid, int rows) {
    if (!grid) return;
    for (int i = 0; i < rows; i++) {
        if (grid[i]) free(grid[i]);
    }
    free(grid);
}

int main(void) {
    // 1. Charger le réseau
    NeuralNetwork *nn = load_network("trained_network.bin");
    if (!nn) return 1;

    // --- Variables pour la reconstruction de la GRILLE (dynamique) ---
    GridLetter *letters = NULL;
    int num_letters = 0;
    int max_x = -1, max_y = -1; // Pour déterminer la taille réelle (max_x+1, max_y+1)
    
    // --- Structures pour la liste de MOTS (statique) ---
    char words[MAX_WORDS][MAX_WORD_LEN + 1];
    for(int i=0; i<MAX_WORDS; i++) {
        memset(words[i], '\0', MAX_WORD_LEN + 1);
    }
    
    // 2. Lecture du fichier d'entrée et Reconnaissance
    FILE *f = fopen(INPUT_FILE, "r");
    if (!f) {
        perror("❌ Erreur fichier decoupage_output.txt (Lancez bridge.py avant)");
        free_network(nn);
        return 1;
    }

    char type[10];
    int val1, val2; // x/col ou wordIdx/pos
    float pixels[INPUT_SIZE];
    int capacity = INITIAL_GRID_SIZE;
    letters = malloc(capacity * sizeof(GridLetter));

    printf("🧠 Démarrage de la reconnaissance et de la reconstruction...\n");

    while (fscanf(f, " %s %d %d", type, &val1, &val2) == 3) {
        // Lire les 1024 pixels
        for (int i = 0; i < INPUT_SIZE; i++) {
            if (fscanf(f, "%f", &pixels[i]) != 1) break;
        }
        
        // Prédire la lettre
        char predicted = predict_character(nn, pixels);

        if (strcmp(type, "GRID") == 0) {
            // val1 = x (col), val2 = y (row) - Déjà 0-indexé par Python
            
            // 💡 Allocation Dynamique: Augmenter la capacité si nécessaire
            if (num_letters >= capacity) {
                capacity *= 2;
                letters = realloc(letters, capacity * sizeof(GridLetter));
            }
            
            // Stockage de la lettre reconnue
            letters[num_letters].character = predicted;
            letters[num_letters].x = val1;
            letters[num_letters].y = val2;
            
            // 💡 Détermination de la taille maximale de la grille
            if (val1 > max_x) max_x = val1;
            if (val2 > max_y) max_y = val2;
            num_letters++;

        } 
        else if (strcmp(type, "WORD") == 0) {
            // Reconstruction de la liste de mots (val1 = wordIndex, val2 = position)
            if (val1 < MAX_WORDS && val2 < MAX_WORD_LEN)
                words[val1][val2] = predicted;
        }
    }
    fclose(f);

    // --- 3. Construction de la Matrice 2D (Grille) ---
    int final_width = 8;
    int final_height = 8;
    
    if (final_width > 0 && final_height > 0) {
        // Allocation de la grille 2D à la taille exacte
        char **final_grid = (char **)malloc(final_height * sizeof(char *));
        for (int i = 0; i < final_height; i++) {
            final_grid[i] = (char *)malloc((final_width + 1) * sizeof(char));
            memset(final_grid[i], ' ', final_width); 
            final_grid[i][final_width] = '\0'; // Caractère de fin de chaîne
        }

        // Remplissage de la grille avec les prédictions
        for (int i = 0; i < num_letters; i++) {
            if (letters[i].y < final_height && letters[i].x < final_width) {
                 final_grid[letters[i].y][letters[i].x] = letters[i].character;
            }
        }

        // --- 4. Écriture des fichiers TXT pour le Solver ---
        
        // Fichier Grille (grid.txt)
        FILE *fg = fopen(GRID_OUTPUT_FILE, "w");
        if (fg) {
            printf("\n--- Grille Reconstruite (%dx%d) ---\n", final_width, final_height);
            for (int i = 0; i < final_height; i++) {
                fprintf(fg, "%s\n", final_grid[i]);
                printf("%s\n", final_grid[i]);
            }
            fclose(fg);
        }

        // Libération de la grille allouée
        free_grid_matrix(final_grid, final_height);
    }
    
    // Fichier Mots (words.txt)
    FILE *fw = fopen(WORDS_OUTPUT_FILE, "w");
    if (fw) {
        printf("\n--- Mots Reconnus ---\n");
        for (int i = 0; i < MAX_WORDS; i++) {
            if (strlen(words[i]) > 0) {
                fprintf(fw, "%s\n", words[i]);
                printf("%s\n", words[i]);
            }
        }
        fclose(fw);
    }

    printf("\n✅ Sorties '%s' et '%s' prêtes pour le solver.\n", GRID_OUTPUT_FILE, WORDS_OUTPUT_FILE);

    // --- 5. Nettoyage final ---
    free(letters);
    free_network(nn);
    return 0;
}
