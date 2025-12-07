#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>

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

// Fonction de tri pour les mots (Haut -> Bas)
int compare_rect_y(const void* a, const void* b) {
    return ((Rect*)a)->y0 - ((Rect*)b)->y0;
}

// Fonction de tri pour les lettres (Gauche -> Droite)
int compare_rect_x(const void* a, const void* b) {
    return ((Rect*)a)->x0 - ((Rect*)b)->x0;
}

// Vérifie si un pixel est noir ET opaque
// MODIFICATION PNG : On gère maintenant 4 canaux (RGBA)
int is_black(unsigned char* img, int w, int h, int x, int y){
    if(x<0 || x>=w || y<0 || y>=h) return 0;
    
    // Accès pixel avec 4 canaux
    unsigned char* p = img + (y * w + x) * 4; 
    unsigned char r = p[0];
    unsigned char g = p[1];
    unsigned char b = p[2];
    unsigned char a = p[3]; // Canal Alpha (Transparence)

    // Si le pixel est transparent (ou presque), ce n'est pas du noir
    if (a < 100) return 0;

    // Si le pixel est opaque, on vérifie s'il est sombre
    return (r < 150 && g < 150 && b < 150);
}

// Flood-fill pour récupérer une zone (8-directions)
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
            // On appelle is_black sans l'argument channels car fixé à 4
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

// Calcul surface
int area(Rect r){ return (r.x1-r.x0+1)*(r.y1-r.y0+1); }

// Sauvegarde rectangle en PNG
void save_rect(unsigned char* img, int w, int h, Rect r, const char* filename){
    if (r.x1 < r.x0 || r.y1 < r.y0) return;
    
    int new_w = r.x1 - r.x0 + 1;
    int new_h = r.y1 - r.y0 + 1;
    int channels = 4; // Force 4 canaux pour PNG

    unsigned char* crop = malloc(new_w * new_h * channels);
    if (!crop) { printf("Erreur malloc pour crop\n"); return; }

    for(int y=0;y<new_h;y++) {
        for(int x=0;x<new_w;x++) {
            // Copie des 4 canaux (RGBA)
            int src_idx = ((r.y0 + y) * w + (r.x0 + x)) * channels;
            int dst_idx = (y * new_w + x) * channels;
            
            crop[dst_idx + 0] = img[src_idx + 0]; // R
            crop[dst_idx + 1] = img[src_idx + 1]; // G
            crop[dst_idx + 2] = img[src_idx + 2]; // B
            crop[dst_idx + 3] = img[src_idx + 3]; // A
        }
    }
    
    // Sauvegarde en PNG (gère la transparence)
    stbi_write_png(filename, new_w, new_h, channels, crop, new_w * channels);
    free(crop);
}

Rect rect_union(Rect r1, Rect r2) {
    Rect r;
    r.x0 = (r1.x0 < r2.x0) ? r1.x0 : r2.x0;
    r.y0 = (r1.y0 < r2.y0) ? r1.y0 : r2.y0;
    r.x1 = (r1.x1 > r2.x1) ? r1.x1 : r2.x1;
    r.y1 = (r1.y1 > r2.y1) ? r1.y1 : r2.y1;
    return r;
}

int should_merge_as_word(Rect r1, Rect r2, double avg_h) {
    int h_gap = 0;
    if (r2.x0 > r1.x1) h_gap = r2.x0 - r1.x1;
    else if (r1.x0 > r2.x1) h_gap = r1.x0 - r2.x1;
    else h_gap = 0; 

    int overlap_y1 = (r1.y1 < r2.y1) ? r1.y1 : r2.y1;
    int overlap_y0 = (r1.y0 > r2.y0) ? r1.y0 : r2.y0;
    int v_overlap = overlap_y1 - overlap_y0;

    int horizontal_threshold = (int)(avg_h * 1.5); 
    int vertical_threshold = (int)(avg_h * 0.25); 

    int is_horizontally_close = (h_gap <= horizontal_threshold);
    int is_vertically_aligned = (v_overlap >= vertical_threshold);
    
    return (is_horizontally_close && is_vertically_aligned);
}

// Vérifie si une lettre est contenue (géométriquement) dans le mot
int is_letter_in_word(Rect letter, Rect word) {
    int cx = (letter.x0 + letter.x1) / 2;
    int cy = (letter.y0 + letter.y1) / 2;
    return (cx >= word.x0 && cx <= word.x1 && cy >= word.y0 && cy <= word.y1);
}

int main(int argc,char** argv){
    if(argc<2){ printf("Usage: %s image_liste_mots.png\n",argv[0]); return 1;}

    // Création des dossiers
    #ifdef _WIN32
        system("mkdir words");
        system("mkdir words_letters");
    #else
        mkdir("words", 0777);
        mkdir("words_letters", 0777);
    #endif

    int w,h,channels_in_file;
    // MODIFICATION PNG : On FORCE le chargement en 4 canaux (RGBA)
    // Cela permet à stbi de mettre Alpha à 255 si l'image n'a pas de transparence,
    // ou de lire la transparence si elle existe.
    unsigned char* img = stbi_load(argv[1], &w, &h, &channels_in_file, 4);
    
    if(!img){ printf("Impossible de charger %s\n",argv[1]); return 1;}
    
    int* visited = calloc(w*h,sizeof(int));
    Rect zones[10000]; int n_zones=0;
    double total_height = 0;

    // --- ETAPE 1: Détecter tous les blobs (lettres) ---
    printf("Etape 1: Détection des blobs (lettres)...\n");
    for(int y=0;y<h;y++) for(int x=0;x<w;x++) {
        // is_black gère maintenant l'Alpha
        if(!visited[y*w+x] && is_black(img,w,h,x,y)) {
            if (n_zones >= 10000) break;
            Rect r = flood_fill(img,w,h,visited,x,y);
            if (area(r) > 10) { 
                zones[n_zones++] = r;
                total_height += (r.y1 - r.y0 + 1);
            }
        }
    }

    if(n_zones==0){ printf("Aucune lettre détectée.\n"); return 1; }
    double avg_h = (n_zones > 0) ? (total_height / n_zones) : 20.0;
    printf("... %d blobs (lettres) détectés. Hauteur moyenne: %.2f px\n", n_zones, avg_h);

    // --- ETAPE 2: Fusionner les lettres en mots ---
    printf("Etape 2: Fusion des lettres en mots...\n");
    int* zone_merged = calloc(n_zones, sizeof(int));
    Rect merged_zones[10000]; int n_merged = 0;

    for (int i = 0; i < n_zones; i++) {
        if (zone_merged[i]) continue;
        Rect current_cluster = zones[i];
        zone_merged[i] = 1;
        int merged_something;
        do {
            merged_something = 0;
            for (int j = 0; j < n_zones; j++) { 
                if (i != j && !zone_merged[j] && 
                    should_merge_as_word(current_cluster, zones[j], avg_h)) 
                {
                    current_cluster = rect_union(current_cluster, zones[j]);
                    zone_merged[j] = 1;
                    merged_something = 1;
                }
            }
        } while (merged_something);
        merged_zones[n_merged++] = current_cluster;
    }
    printf("... %d mots fusionnés obtenus.\n", n_merged);
    free(zone_merged);

    // --- ETAPE 2 BIS: Trier les mots de haut en bas ---
    qsort(merged_zones, n_merged, sizeof(Rect), compare_rect_y);

    // --- ETAPE 3: Sauvegarder chaque mot ET ses lettres ---
    printf("Etape 3: Sauvegarde et découpage secondaire...\n");
    int saved_count = 0;
    for(int i=0; i<n_merged; i++) {
        if (area(merged_zones[i]) < (avg_h * avg_h * 0.5)) {
             continue;
        }

        // 1. Sauvegarde du MOT entier (format .png)
        char filename[100];
        sprintf(filename, "words/word_%03d.png", saved_count);
        save_rect(img, w, h, merged_zones[i], filename);

        // 2. Retrouver les lettres contenues dans ce mot
        Rect word_letters[100]; 
        int n_wl = 0;

        for(int k=0; k<n_zones; k++) {
            if(is_letter_in_word(zones[k], merged_zones[i])) {
                if(n_wl < 100) word_letters[n_wl++] = zones[k];
            }
        }

        // 3. Trier les lettres de gauche à droite
        qsort(word_letters, n_wl, sizeof(Rect), compare_rect_x);

        // 4. Sauvegarder les lettres individuelles
        for(int l=0; l<n_wl; l++) {
            char l_filename[100];
            // Format: word_ID_lettre_ID.png
            sprintf(l_filename, "words_letters/word_%03d_let_%02d.png", saved_count, l);
            save_rect(img, w, h, word_letters[l], l_filename);
        }

        saved_count++;
    }
    printf("... %d mots sauvegardés et découpés dans 'words_letters/'.\n", saved_count);

    stbi_image_free(img);
    free(visited);
    return 0;
}
