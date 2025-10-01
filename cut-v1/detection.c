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
    return (r<128 && g<128 && b<128);
}

// Flood-fill pour récupérer une zone
Rect flood_fill(unsigned char* img,int w,int h,int channels,int* visited,int start_x,int start_y){
    Stack stack; stack.data=malloc(1024*sizeof(Point)); stack.size=0; stack.capacity=1024;
    push(&stack,(Point){start_x,start_y}); visited[start_y*w+start_x]=1;
    int x0=start_x,y0=start_y,x1=start_x,y1=start_y;

    while(stack.size>0){
        Point p = pop(&stack);
        int dx[4]={-1,1,0,0}, dy[4]={0,0,-1,1};
        for(int i=0;i<4;i++){
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
    int new_w = r.x1 - r.x0 + 1;
    int new_h = r.y1 - r.y0 + 1;
    unsigned char* crop = malloc(new_w*new_h*channels);
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

int main(int argc,char** argv){
    if(argc<2){ printf("Usage: %s image.png\n",argv[0]); return 1;}
    int w,h,channels;
    unsigned char* img = stbi_load(argv[1],&w,&h,&channels,3);
    if(!img){ printf("Impossible de charger %s\n",argv[1]); return 1;}
    int* visited = calloc(w*h,sizeof(int));

    // détecter toutes les zones (lettres)
    Rect zones[10000]; int n_zones=0;
    for(int y=0;y<h;y++) for(int x=0;x<w;x++)
        if(!visited[y*w+x] && is_black(img,w,h,channels,x,y))
            zones[n_zones++] = flood_fill(img,w,h,channels,visited,x,y);

    if(n_zones==0){ printf("Aucune zone détectée.\n"); return 1; }

    // identifier la grille : zone avec la plus grande surface
    int max_area = 0; int grid_index=-1;
    for(int i=0;i<n_zones;i++){
        int a = area(zones[i]);
        if(a>max_area){ max_area=a; grid_index=i; }
    }
    Rect grid = zones[grid_index];
    save_rect(img,w,h,3,grid,"grid.png");
    printf("Grille détectée : (%d,%d) -> (%d,%d)\n",grid.x0,grid.y0,grid.x1,grid.y1);

    // identifier les zones “lettres restantes” pour les mots
    int min_x=w, min_y=h, max_x=0, max_y=0;
    Point center_grid = center(grid);
    double max_dist = grid.y1 - grid.y0; // tolérance : hauteur de la grille

    for(int i=0;i<n_zones;i++){
        if(i==grid_index) continue;  // on ignore la grille

        Rect r = zones[i];

        // Exclure les zones qui chevauchent la grille
        int overlap_x = (r.x0 <= grid.x1) && (r.x1 >= grid.x0);
        int overlap_y = (r.y0 <= grid.y1) && (r.y1 >= grid.y0);
        if(overlap_x && overlap_y) continue;

        Point c = center(r);
        double d = distance(center_grid,c);
        if(d <= 3*max_dist){ // lettres proches de la grille
            if(r.x0<min_x) min_x=r.x0;
            if(r.y0<min_y) min_y=r.y0;
            if(r.x1>max_x) max_x=r.x1;
            if(r.y1>max_y) max_y=r.y1;
        }
    }

    if(max_x>min_x && max_y>min_y){
        Rect words = {min_x,min_y,max_x,max_y};
        save_rect(img,w,h,3,words,"words.png");
        printf("Liste de mots détectée : (%d,%d) -> (%d,%d)\n",words.x0,words.y0,words.x1,words.y1);
    } else {
        printf("Aucune zone de mots détectée.\n");
    }

    stbi_image_free(img);
    free(visited);
    return 0;
}

