#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

typedef struct { int x0,y0,x1,y1; } Rect;
typedef struct { int x,y; } Point;
typedef struct { Point* data; int size,capacity; } Stack;

void push(Stack* s, Point p){
    if(s->size>=s->capacity){
        s->capacity*=2;
        s->data = realloc(s->data, s->capacity*sizeof(Point));
    }
    s->data[s->size++] = p;
}

Point pop(Stack* s){ return s->data[--s->size]; }

int is_black(unsigned char* img, int w, int h, int x, int y){
    if(x<0 || x>=w || y<0 || y>=h) return 0;
    unsigned char* p = img + (y * w + x) * 4;
    if (p[3] < 100) return 0;
    return (p[0] < 150 && p[1] < 150 && p[2] < 150);
}

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

int area(Rect r){ return (r.x1-r.x0+1)*(r.y1-r.y0+1); }

void save_subimage(unsigned char* img, int w, int h, Rect r, const char* filename){
    if (r.x1 < r.x0 || r.y1 < r.y0) return;
    int new_w = r.x1 - r.x0 + 1;
    int new_h = r.y1 - r.y0 + 1;
    unsigned char* crop = malloc(new_w * new_h * 4);
    if (!crop) return;
    for(int y=0;y<new_h;y++) {
        for(int x=0;x<new_w;x++) {
            int src = ((r.y0 + y) * w + (r.x0 + x)) * 4;
            int dst = (y * new_w + x) * 4;
            memcpy(&crop[dst], &img[src], 4);
        }
    }
    stbi_write_png(filename, new_w, new_h, 4, crop, new_w * 4);
    free(crop);
}

int compare_reading_order(const void* a, const void* b) {
    Rect* r1 = (Rect*)a;
    Rect* r2 = (Rect*)b;
    int cy1 = (r1->y0 + r1->y1) / 2;
    int cy2 = (r2->y0 + r2->y1) / 2;
    int h = r1->y1 - r1->y0;
    if (abs(cy1 - cy2) < (h / 2)) return r1->x0 - r2->x0;
    return r1->y0 - r2->y0;
}

int main(int argc, char **argv) {
    if (argc < 2) return 1;

    #ifdef _WIN32
        system("mkdir letters");
    #else
        mkdir("letters", 0777);
    #endif

    int w, h, c;
    unsigned char *img = stbi_load(argv[1], &w, &h, &c, 4);
    if (!img) return 1;

    int* visited = calloc(w * h, sizeof(int));
    Rect letters[5000];
    int count = 0;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (!visited[y * w + x] && is_black(img, w, h, x, y)) {
                if (count >= 5000) break;
                Rect r = flood_fill(img, w, h, visited, x, y);
                if (area(r) > 15) letters[count++] = r;
            }
        }
    }

    if (count == 0) {
        stbi_image_free(img);
        free(visited);
        return 0;
    }

    qsort(letters, count, sizeof(Rect), compare_reading_order);

    int row = 0;
    int col = 0;
    int current_line_y = letters[0].y0;

    for (int i = 0; i < count; i++) {
        int h_letter = letters[i].y1 - letters[i].y0;
        if (letters[i].y0 > current_line_y + (h_letter / 2)) {
            row++;
            col = 0;
            current_line_y = letters[i].y0;
        }
        char out_name[512];
        sprintf(out_name, "letters/letter_%02d_%02d.png", row, col);
        save_subimage(img, w, h, letters[i], out_name);
        col++;
    }

    stbi_image_free(img);
    free(visited);
    return 0;
}
