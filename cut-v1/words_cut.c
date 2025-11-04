#include <stdio.h>
#include <stdlib.h>
#include <math.h>
// AJOUT : Nécessaire pour mkdir (systèmes POSIX comme Linux/macOS)
#include <sys/stat.h>
#include <sys/types.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// --- Structures (identiques au script précédent) ---

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

// Vérifie si un pixel est noir
int is_black(unsigned char* img,int w,int h,int channels,int x,int y){
    if(x<0||x>=w||y<0||y>=h) return 0;
    unsigned char r = img[(y*w+x)*channels+0];
    unsigned char g = img[(y*w+x)*channels+1];
    unsigned char b = img[(y*w+x)*channels+2];
    return (r<150 && g<150 && b<150);
}

// Flood-fill pour récupérer une zone (8-directions)
Rect flood_fill(unsigned char* img,int w,int h,int channels,int* visited,int start_x,int start_y){
    Stack stack; stack.data=malloc(1024*sizeof(Point)); stack.size=0; stack.capacity=1024;
    push(&stack,(Point){start_x,start_y}); visited[start_y*w+start_x]=1;
    int x0=start_x,y0=start_y,x1=start_x,y1=start_y;

    while(stack.size>0){
        Point p = pop(&stack);
        int dx[8]={-1, 0, 1,-1, 1,-1, 0, 1};
        int dy[8]={-1,-1,-1, 0, 0, 1, 1, 1};
        
        for(int i=0;i<8;i++){
            int nx=p.x+dx[i], ny=p.y+dy[i];
            if(nx>=0 && nx<w && ny>=0 && ny<h && !visited[ny*w+nx] && is_black(img,w,h,channels,nx,ny)){
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

// Sauvegarde rectangle
void save_rect(unsigned char* img,int w,int h,int channels,Rect r,const char* filename){
    if (r.x1 < r.x0 || r.y1 < r.y0) {
        printf("Avertissement: Tentative de sauvegarde d'un rectangle invalide pour %s\n", filename);
        return;
    }
    int new_w = r.x1 - r.x0 + 1;
    int new_h = r.y1 - r.y0 + 1;
    unsigned char* crop = malloc(new_w*new_h*channels);
    if (!crop) { printf("Erreur malloc pour crop\n"); return; }

    for(int y=0;y<new_h;y++)
        for(int x=0;x<new_w;x++)
            for(int c=0;c<channels;c++)
                crop[(y*new_w+x)*channels+c] = img[((r.y0+y)*w+(r.x0+x))*channels+c];
    stbi_write_png(filename,new_w,new_h,channels,crop,new_w*channels);
    free(crop);
}

// Retourne l'union de deux rectangles
Rect rect_union(Rect r1, Rect r2) {
    Rect r;
    r.x0 = (r1.x0 < r2.x0) ? r1.x0 : r2.x0;
    r.y0 = (r1.y0 < r2.y0) ? r1.y0 : r2.y0;
    r.x1 = (r1.x1 > r2.x1) ? r1.x1 : r2.x1;
    r.y1 = (r1.y1 > r2.y1) ? r1.y1 : r2.y1;
    return r;
}

// --- NOUVELLE FONCTION DE FUSION ---

/**
 * @brief Vérifie si deux blobs (lettres) font partie du même mot.
 * Ils sont fusionnés s'ils sont proches horizontalement ET alignés verticalement.
 */
int should_merge_as_word(Rect r1, Rect r2, double avg_h) {
    // 1. Calculer l'écart horizontal
    int h_gap = 0;
    if (r2.x0 > r1.x1) h_gap = r2.x0 - r1.x1;
    else if (r1.x0 > r2.x1) h_gap = r1.x0 - r2.x1;
    else h_gap = 0; // Chevauchement

    // 2. Calculer le chevauchement vertical
    int overlap_y1 = (r1.y1 < r2.y1) ? r1.y1 : r2.y1;
    int overlap_y0 = (r1.y0 > r2.y0) ? r1.y0 : r2.y0;
    int v_overlap = overlap_y1 - overlap_y0;

    // --- RÈGLES DE FUSION ---
    int horizontal_threshold = (int)(avg_h * 1.5); 
    int vertical_threshold = (int)(avg_h * 0.25); 

    int is_horizontally_close = (h_gap <= horizontal_threshold);
    int is_vertically_aligned = (v_overlap >= vertical_threshold);
    
    return (is_horizontally_close && is_vertically_aligned);
}


// --- MAIN ---

int main(int argc,char** argv){
    if(argc<2){ printf("Usage: %s image_liste_mots.png\n",argv[0]); return 1;}

    // --- MODIFICATION : Créer le dossier "words" ---
    // Le mode 0777 donne les permissions complètes.
    // La fonction ne fait rien si le dossier existe déjà (sur Linux/macOS).
    mkdir("words", 0777);
    // --- Fin de la modification ---

    int w,h,channels;
    unsigned char* img = stbi_load(argv[1],&w,&h,&channels,3);
    if(!img){ printf("Impossible de charger %s\n",argv[1]); return 1;}
    int* visited = calloc(w*h,sizeof(int));

    Rect zones[10000]; int n_zones=0;
    double total_height = 0;

    // --- ETAPE 1: Détecter tous les blobs (lettres) ---
    printf("Etape 1: Détection des blobs (lettres)...\n");
    for(int y=0;y<h;y++) for(int x=0;x<w;x++) {
        if(!visited[y*w+x] && is_black(img,w,h,channels,x,y)) {
            if (n_zones >= 10000) break;
            Rect r = flood_fill(img,w,h,channels,visited,x,y);
            if (area(r) > 20) { 
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

    // --- ETAPE 3: Sauvegarder chaque mot ---
    printf("Etape 3: Sauvegarde des mots dans le dossier 'words/'...\n");
    int saved_count = 0;
    for(int i=0; i<n_merged; i++) {
        if (area(merged_zones[i]) < (avg_h * avg_h * 0.5)) {
             continue;
        }

        char filename[100];
        // --- MODIFICATION : Ajouter le préfixe "words/" au chemin ---
        sprintf(filename, "words/word_%03d.png", saved_count + 1);
        // --- Fin de la modification ---
        
        save_rect(img, w, h, channels, merged_zones[i], filename);
        saved_count++;
    }
    printf("... %d mots sauvegardés.\n", saved_count);

    stbi_image_free(img);
    free(visited);
    return 0;
}

