#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    char** data;
    int width;
    int height;
} Grid;

typedef struct {
    int found;
    int x0, y0;
    int x1, y1;
} SearchResult;

Grid load_grid(const char* filename);
void free_grid(Grid* grid);
char* str_to_upper(const char* str);
SearchResult find_word(const Grid* grid, const char* word);
int check_direction(const Grid* grid, const char* word, int x, int y, int dx, int dy);

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <fichier_grille> <mot>\n", argv[0]);
        return 1;
    }

    const char* grid_filename = argv[1];
    const char* word_to_find = argv[2];

    Grid grid = load_grid(grid_filename);
    if (grid.data == NULL) {
        return 1;
    }

    char* word_upper = str_to_upper(word_to_find);

    SearchResult result = find_word(&grid, word_upper);

    if (result.found) {
        printf("(%d,%d)(%d,%d)\n", result.x0, result.y0, result.x1, result.y1);
    } else {
        printf("Not found\n");
    }

    free_grid(&grid);
    free(word_upper);

    return 0;
}

Grid load_grid(const char* filename) {
    Grid grid = {NULL, 0, 0};
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Erreur: Impossible d'ouvrir le fichier de grille");
        return grid;
    }

    char buffer[1024];
    int capacity = 10;
    
    grid.data = (char**)malloc(capacity * sizeof(char*));
    if (grid.data == NULL) {
        fprintf(stderr, "Erreur d'allocation mémoire (grid.data)\n");
        fclose(file);
        return grid;
    }

    while (fgets(buffer, sizeof(buffer), file)) {
        if (grid.height == capacity) {
            capacity *= 2;
            grid.data = (char**)realloc(grid.data, capacity * sizeof(char*));
            if (grid.data == NULL) {
                fprintf(stderr, "Erreur de ré-allocation mémoire\n");
                grid.height = 0;
                free_grid(&grid); 
                fclose(file);
                return grid;
            }
        }

        buffer[strcspn(buffer, "\n")] = 0;
        
        if (grid.width == 0) {
            grid.width = strlen(buffer);
        }
        
        grid.data[grid.height] = (char*)malloc(grid.width + 1);
        if (grid.data[grid.height] == NULL) {
            fprintf(stderr, "Erreur d'allocation mémoire (ligne)\n");
            free_grid(&grid);
            fclose(file);
            return grid;
        }
        strcpy(grid.data[grid.height], buffer);
        grid.height++;
    }

    fclose(file);
    return grid;
}

void free_grid(Grid* grid) {
    if (grid->data) {
        for (int i = 0; i < grid->height; i++) {
            free(grid->data[i]);
        }
        free(grid->data);
        grid->data = NULL;
    }
}

char* str_to_upper(const char* str) {
    size_t len = strlen(str);
    char* upper_str = (char*)malloc(len + 1);
    if (!upper_str) {
        fprintf(stderr, "Erreur d'allocation (str_to_upper)\n");
        exit(1);
    }
    
    for (size_t i = 0; i < len; i++) {
        upper_str[i] = toupper(str[i]);
    }
    upper_str[len] = '\0';
    return upper_str;
}

SearchResult find_word(const Grid* grid, const char* word) {
    SearchResult result = {0, 0, 0, 0, 0};
    int word_len = strlen(word);

    int dx[] = { 1,  1, 0, -1, -1, -1, 0,  1};
    int dy[] = { 0,  1, 1,  1,  0, -1, -1, -1};

    for (int y = 0; y < grid->height; y++) {
        for (int x = 0; x < grid->width; x++) {
            
            for (int dir = 0; dir < 8; dir++) {
                if (check_direction(grid, word, x, y, dx[dir], dy[dir])) {
                    result.found = 1;
                    result.x0 = x;
                    result.y0 = y;
                    result.x1 = x + dx[dir] * (word_len - 1);
                    result.y1 = y + dy[dir] * (word_len - 1);
                    return result;
                }
            }
        }
    }

    return result;
}

int check_direction(const Grid* grid, const char* word, int x, int y, int dx, int dy) {
    int word_len = strlen(word);

    for (int i = 0; i < word_len; i++) {
        int current_x = x + i * dx;
        int current_y = y + i * dy;

        if (current_x < 0 || current_x >= grid->width ||
            current_y < 0 || current_y >= grid->height) {
            return 0;
        }

        if (grid->data[current_y][current_x] != word[i]) {
            return 0;
        }
    }

    return 1;
}
