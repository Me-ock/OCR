#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

typedef struct { int x0,y0,x1,y1; } Rect;
typedef struct { int x,y; } Point;

// Stack pour flood-fill
typedef struct { Point* data; int size,capacity; } Stack;
void push(Stack* s, Point p){
    if(s->size>=s->capacity){
        s->capacity*=2;
        s->data = realloc(s->data, s->capacity*sizeof(Point));
    }
    s->data[s->size++] = p;
}
Point pop(Stack* s){ return s->data[--s->size]; }

// Calcul surface
int area(Rect r){ return (r.x1-r.x0+1)*(r.y1-r.y0+1); }

// Fonction de comparaison pour trier les zones par surface décroissante (plus grand au plus petit)
int compare_rect_area_desc(const void* a, const void* b) {
    return area(*(Rect*)b) - area(*(Rect*)a);
}

// Vérifie si un pixel est noir (compatible PNG transparent)
int is_black(unsigned char* img, int w, int h, int x, int y){
    if(x<0 || x>=w || y<0 || y>=h) return 0;
    
    // Accès pixel en 4 canaux (RGBA)
    unsigned char* p = img + (y * w + x) * 4;
    unsigned char r = p[0];
    unsigned char g = p[1];
    unsigned char b = p[2];
    unsigned char a = p[3];

    // Si le pixel est transparent (Alpha < 128), ce n'est pas du noir
    if (a < 128) return 0;

    // Sinon, on vérifie la couleur (sombre)
    return (r < 150 && g < 150 && b < 150);
}

// Flood-fill pour récupérer une zone
Rect flood_fill(unsigned char* img, int w, int h, int* visited, int start_x, int start_y){
    Stack stack; stack.data=malloc(1024*sizeof(Point)); stack.size=0; stack.capacity=1024;
    push(&stack,(Point){start_x,start_y}); visited[start_y*w+start_x]=1;
    int x0=start_x,y0=start_y,x1=start_x,y1=start_y;

    while(stack.size>0){
        Point p = pop(&stack);
        int dx[8]={-1, 0, 1,-1, 1,-1, 0, 1};
        int dy[8]={-1,-1,-1, 0, 0, 1, 1, 1};
        
        for(int i=0;i<8;i++){
            int nx=p.x+dx[i], ny=p.y+dy[i];
            // Appel à is_black (plus besoin de passer channels car fixé à 4)
            if(nx>=0 && nx<w && ny>=0 && ny<h && !visited[ny*w+nx] && is_black(img,w,h,nx,ny)){
                push(&stack,(Point){nx,ny});
                visited[ny*w+nx]=1;
                if(nx<x0)x0=nx; if(ny<y0)y0=ny;
                if(nx>x1)x1=nx; if(ny>y1)y1=ny;
            }
        }
    }
    free(stack.data);
    return (Rect){x0,y0,x1,y1};
}

// Sauvegarde rectangle (support PNG avec transparence)
void save_rect(unsigned char* img, int w, int h, Rect r, const char* filename){
    if (r.x1 < r.x0 || r.y1 < r.y0) {
        printf("Avertissement: Rectangle invalide pour %s\n", filename);
        return;
    }
    int new_w = r.x1 - r.x0 + 1;
    int new_h = r.y1 - r.y0 + 1;
    int channels = 4; // Force 4 canaux

    unsigned char* crop = malloc(new_w * new_h * channels);
    if (!crop) { printf("Erreur malloc pour crop\n"); return; }

    for(int y=0;y<new_h;y++) {
        for(int x=0;x<new_w;x++) {
            int src_idx = ((r.y0 + y) * w + (r.x0 + x)) * channels;
            int dst_idx = (y * new_w + x) * channels;
            
            crop[dst_idx + 0] = img[src_idx + 0];
            crop[dst_idx + 1] = img[src_idx + 1];
            crop[dst_idx + 2] = img[src_idx + 2];
            crop[dst_idx + 3] = img[src_idx + 3];
        }
    }
    stbi_write_png(filename, new_w, new_h, channels, crop, new_w * channels);
    free(crop);
}

// Vérifie si deux rectangles sont proches
int rect_is_close(Rect r1, Rect r2, int dist) {
    Rect r1_expanded = { r1.x0 - dist, r1.y0 - dist, r1.x1 + dist, r1.y1 + dist };
    return !(r2.x0 > r1_expanded.x1 || r2.x1 < r1_expanded.x0 || r2.y0 > r1_expanded.y1 || r2.y1 < r1_expanded.y0);
}

Rect rect_union(Rect r1, Rect r2) {
    Rect r;
    r.x0 = (r1.x0 < r2.x0) ? r1.x0 : r2.x0;
    r.y0 = (r1.y0 < r2.y0) ? r1.y0 : r2.y0;
    r.x1 = (r1.x1 > r2.x1) ? r1.x1 : r2.x1;
    r.y1 = (r1.y1 > r2.y1) ? r1.y1 : r2.y1;
    return r;
}

int main(int argc,char** argv){
    if(argc<2){ printf("Usage: %s image.png\n",argv[0]); return 1;}
    
    int w,h,channels_in_file;
    // MODIFICATION 1 : Force le chargement en 4 canaux (RGBA) pour gérer la transparence PNG
    unsigned char* img = stbi_load(argv[1], &w, &h, &channels_in_file, 4);
    
    if(!img){ printf("Impossible de charger %s\n",argv[1]); return 1;}
    
    int* visited = calloc(w*h,sizeof(int));
    Rect zones[10000]; int n_zones=0;
    
    printf("Etape 1: Détection des blobs...\n");
    for(int y=0;y<h;y++) for(int x=0;x<w;x++) {
        // is_black a été mis à jour pour ignorer les pixels transparents
        if(!visited[y*w+x] && is_black(img,w,h,x,y)) {
            if (n_zones >= 10000) {
                printf("Limite de 10000 zones atteinte!\n");
                break;
            }
            zones[n_zones++] = flood_fill(img,w,h,visited,x,y);
        }
    }

    if(n_zones==0){ printf("Aucune zone détectée.\n"); return 1; }
    printf("... %d blobs détectés.\n", n_zones);

    // --- Fusionner les blobs en blocs ---
    printf("Etape 2: Fusion des blobs en blocs...\n");
    int* zone_merged = calloc(n_zones, sizeof(int));
    Rect merged_zones[10000]; int n_merged = 0;
    int merge_distance = 35; // Distance de fusion (peut être ajustée)

    for (int i = 0; i < n_zones; i++) {
        if (zone_merged[i]) continue;

        Rect current_cluster = zones[i];
        zone_merged[i] = 1;

        int merged_something;
        do {
            merged_something = 0;
            for (int j = i + 1; j < n_zones; j++) {
                if (!zone_merged[j] && rect_is_close(current_cluster, zones[j], merge_distance)) {
                    current_cluster = rect_union(current_cluster, zones[j]);
                    zone_merged[j] = 1;
                    merged_something = 1;
                }
            }
        } while (merged_something);

        // On ne garde que les blocs qui ont une taille significative (> 50px de surface par ex)
        // pour éviter de garder des poussières isolées comme des "mots"
        if (area(current_cluster) > 50) {
            merged_zones[n_merged++] = current_cluster;
        }
    }
    printf("... %d blocs majeurs obtenus après fusion.\n", n_merged);
    free(zone_merged);

    if (n_merged < 1) {
        printf("Erreur : Pas assez de blocs trouvés pour distinguer grille et mots.\n");
        stbi_image_free(img);
        free(visited);
        return 1;
    }

    // --- Identification Grille vs Liste ---
    printf("Etape 3: Identification par surface...\n");
    
    // MODIFICATION 2 : On trie les zones par surface (du plus grand au plus petit)
    qsort(merged_zones, n_merged, sizeof(Rect), compare_rect_area_desc);

    // Hypothèse forte : La Grille est le plus gros bloc, la Liste de mots est le 2ème plus gros.
    Rect grid = merged_zones[0];
    save_rect(img, w, h, grid, "grid.png");
    printf("-> Grille détectée (plus grande zone) : %dx%d px\n", grid.x1-grid.x0, grid.y1-grid.y0);

    if (n_merged >= 2) {
        Rect words = merged_zones[1];
        // Si on a plus de 2 zones, il se peut que la liste de mots soit fragmentée (ex: en 2 colonnes).
        // On peut essayer de fusionner tout ce qui n'est pas la grille.
        for(int k=2; k<n_merged; k++) {
            // Optionnel : on fusionne les autres gros blocs dans "words"
            words = rect_union(words, merged_zones[k]);
        }
        
        save_rect(img, w, h, words, "words.png");
        printf("-> Liste de mots détectée (autres zones fusionnées) : %dx%d px\n", words.x1-words.x0, words.y1-words.y0);
    } else {
        printf("Avertissement : Une seule zone majeure détectée. Vérifiez 'grid.png', elle contient peut-être tout.\n");
    }

    stbi_image_free(img);
    free(visited);
    return 0;
}
