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

// Vérifie si un pixel est noir
int is_black(unsigned char* img,int w,int h,int channels,int x,int y){
    if(x<0||x>=w||y<0||y>=h) return 0;
    unsigned char r = img[(y*w+x)*channels+0];
    unsigned char g = img[(y*w+x)*channels+1];
    unsigned char b = img[(y*w+x)*channels+2];
    // CORRECTION : Tolérance légèrement augmentée
    return (r<150 && g<150 && b<150);
}

// Flood-fill pour récupérer une zone
Rect flood_fill(unsigned char* img,int w,int h,int channels,int* visited,int start_x,int start_y){
    Stack stack; stack.data=malloc(1024*sizeof(Point)); stack.size=0; stack.capacity=1024;
    push(&stack,(Point){start_x,start_y}); visited[start_y*w+start_x]=1;
    int x0=start_x,y0=start_y,x1=start_x,y1=start_y;

    while(stack.size>0){
        Point p = pop(&stack);
        // CORRECTION : dx/dy pour 8 directions
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
    Rect r = {x0,y0,x1,y1};
    return r;
}

// Calcul surface
int area(Rect r){ return (r.x1-r.x0+1)*(r.y1-r.y0+1); }

// Centre d'un rectangle
Point center(Rect r){ return (Point){(r.x0+r.x1)/2,(r.y0+r.y1)/2}; }

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

double distance(Point a, Point b){
    return sqrt((a.x-b.x)*(a.x-b.x) + (a.y-b.y)*(a.y-b.y));
}



// Vérifie si deux rectangles (format x0,y0,x1,y1) se chevauchent.

int rect_intersects(Rect r1, Rect r2) {
    return !(r2.x0 > r1.x1 || r2.x1 < r1.x0 || r2.y0 > r1.y1 || r2.y1 < r1.y0);
}


// Vérifie si deux rectangles sont "proches" (se chevauchent si étendus de `dist`).

int rect_is_close(Rect r1, Rect r2, int dist) {
    Rect r1_expanded = { r1.x0 - dist, r1.y0 - dist, r1.x1 + dist, r1.y1 + dist };
    return rect_intersects(r1_expanded, r2);
}


//Retourne l'union de deux rectangles (le plus petit rect qui contient les deux).

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
    int w,h,channels;
    unsigned char* img = stbi_load(argv[1],&w,&h,&channels,3);
    if(!img){ printf("Impossible de charger %s\n",argv[1]); return 1;}
    int* visited = calloc(w*h,sizeof(int));
    Rect zones[10000]; int n_zones=0;
    printf("Etape 1: Détection des blobs...\n");
    for(int y=0;y<h;y++) for(int x=0;x<w;x++)
        if(!visited[y*w+x] && is_black(img,w,h,channels,x,y)) {
            if (n_zones >= 10000) {
                printf("Limite de 10000 zones atteinte!\n");
                break;
            }
            zones[n_zones++] = flood_fill(img,w,h,channels,visited,x,y);
        }

    if(n_zones==0){ printf("Aucune zone détectée.\n"); return 1; }
    printf("... %d blobs détectés.\n", n_zones);

    //Fusionner les blobs en blocs
    printf("Etape 2: Fusion des blobs en blocs...\n");
    int* zone_merged = calloc(n_zones, sizeof(int));
    Rect merged_zones[10000]; int n_merged = 0;
    int merge_distance = 35;

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

        merged_zones[n_merged++] = current_cluster;
    }
    printf("... %d blocs fusionnés obtenus.\n", n_merged);
    free(zone_merged);

    // Identifier la grille et la liste
    printf("Etape 3: Identification des blocs...\n");
    
    // Heuristique: La grille est un grand bloc "carré"
    // On pénalise les blocs trop "larges" ou trop "hauts"
    double max_grid_score = -1.0;
    int grid_index = -1;

    for(int i=0;i<n_merged;i++){
        Rect r = merged_zones[i];
        double a = area(r);
        if (a == 0) continue;
        
        // Ratio (largeur / hauteur)
        double ratio = (double)(r.x1-r.x0+1) / (r.y1-r.y0+1);
        
        // Le score est la surface, pénalisée si le ratio s'éloigne de 1.0 (carré)
        double ratio_penalty = 1.0 / (1.0 + fabs(ratio - 1.0));
        
        // Favoriser les ratios "raisonnables"
        if (ratio < 0.5 || ratio > 2.0) ratio_penalty *= 0.1;

        double score = a * ratio_penalty;

        if(score > max_grid_score){ max_grid_score = score; grid_index = i; }
    }

    if (grid_index == -1) {
        printf("Aucune grille n'a pu être identifiée.\n");
        stbi_image_free(img);
        free(visited);
        return 1;
    }

    Rect grid = merged_zones[grid_index];
    save_rect(img,w,h,3,grid,"grid.png");
    printf("Grille détectée : (%d,%d) -> (%d,%d)\n",grid.x0,grid.y0,grid.x1,grid.y1);

    // L'union de TOUS les autres blocs est la liste de mots
    Rect words = {w,h,0,0}; // Initialiser en "inversé"
    int found_words = 0;

    for(int i=0;i<n_merged;i++){
        if(i == grid_index) continue;  // on ignore la grille

        Rect r = merged_zones[i];
        words = rect_union(words, r); // Fusionner tous les autres blocs
        found_words = 1;
    }

    if(found_words){
        save_rect(img,w,h,3,words,"words.png");
        printf("Liste de mots détectée : (%d,%d) -> (%d,%d)\n",words.x0,words.y0,words.x1,words.y1);
    } else {
        printf("Aucune zone de mots détectée.\n");
    }

    stbi_image_free(img);
    free(visited);
    return 0;
}

