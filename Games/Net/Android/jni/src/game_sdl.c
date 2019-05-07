#include <SDL.h>
#include <SDL_image.h>  // required to load transparent texture from PNG
#include <SDL_ttf.h>    // required to use TTF fonts  

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "game.h"
#include "game_io.h"
#include "game_sdl.h"
#include "game_rand.h"


/* **************************************************************** */

#define BACKGROUND "bg.jpg"
#define CROSS_P "CROSS.png"
#define CORNER_P "CORNER.png"
#define SEGMENT_P "SEGMENT.png"
#define LEAF_P "LEAF.png"
#define TEE_P "TEE.png"
#define FONT "Arial.ttf"
#define FONTSIZE 36


/* **************************************************************** */

#define MIN(x,y) ( ((x)>(y))? (y) : (x) )

/* **************************************************************** */
     
struct Env_t {  
  SDL_Texture * background;
  SDL_Texture * cross;
  SDL_Texture * corner;
  SDL_Texture * segment;
  SDL_Texture * leaf;
  SDL_Texture * tee;
  SDL_Texture * text;
  int piece_x, piece_y;
  game g;
}; 

/* **************************************************************** */

bool good_arg(char* str){
  for(int i=0; str[i]!='\0'; i++){
    if(str[i]>'9' || str[i]<'0')
      return false;
  }
  return true;
}

/* **************************************************************** */

#ifdef __ANDROID__
static void copy_asset(char * src, char * dst)
{
    SDL_RWops * file = SDL_RWFromFile(src, "r");
    if(!file) ERROR("[ERROR] SDL_RWFromFile: %s\n", src);
    int size = SDL_RWsize(file);
    PRINT("copy file %s (%d bytes) into %s\n", src, size, dst);
    char* buf = (char*)malloc(size+1);
    if(!buf) ERROR("[ERROR] malloc\n");
    int r = SDL_RWread(file, buf, 1, size);
    PRINT("read %d\n", r);
    if(r != size) ERROR("[ERROR] fail to read all file (%d bytes)\n", r);
    FILE* out = fopen(dst, "w+");
    if(!out) ERROR("[ERROR] fail to create file %s\n", dst);
    int w = fwrite(buf, 1, r, out);
    if(r != w) ERROR("[ERROR] fail to write all file (%d bytes)\n", w);
    fclose(out);
    SDL_RWclose(file);
    free(buf);
}
#endif

/* **************************************************************** */
     
Env * init(SDL_Window* win, SDL_Renderer* ren, int argc, char* argv[])
{  

  Env * env = malloc(sizeof(struct Env_t));
  int w, h;
  SDL_GetWindowSize(win, &w, &h);
  //Ouverture du game
  #ifdef __ANDROID__
    const char * dir = SDL_AndroidGetInternalStoragePath();
    /* const char * dir = SDL_AndroidGetExternalStoragePath(); */
    char filename[1024];
    env->g = random_game_ext(5,5,false,true);
  #else
    char gamepath[80]="";                                 
    switch (argc)
    {
      case 1:
        strcat(gamepath,"default.txt");
        env->g = load_game(gamepath);
        break;
      case 2:
        strcat(gamepath,argv[1]);
        env->g = load_game(gamepath);
        break;
      case 3:
        if(good_arg(argv[1]) && good_arg(argv[2]))
          env->g = random_game_ext(strtol(argv[1],NULL,10), strtol(argv[2],NULL,10), false, true);
        else {
          fprintf(stderr,"Usage : Wrong argument ! Follow the correct format for random game !\n");   fprintf(stderr,"Usage : ./net_sdl <width> <height> [S|N] [3|4]\n");
          exit(EXIT_FAILURE);
        }
        break;
      case 4:
        if(good_arg(argv[1]) && good_arg(argv[2]) && (argv[3][0]=='S' || argv[3][0]=='N'))
          env->g = random_game_ext(strtol(argv[1],NULL,10), strtol(argv[2],NULL,10), argv[3][0]=='S'?true:false, true);
        else {
          fprintf(stderr,"Usage : Wrong argument ! Follow the correct format for random game !\n");   fprintf(stderr,"Usage : ./net_sdl <width> <height> [S|N] [3|4]\n");
          exit(EXIT_FAILURE);
        }
        break;
      case 5:
        if(good_arg(argv[1]) && good_arg(argv[2]) && (argv[3][0]=='S' || argv[3][0]=='N') && (argv[4][0]=='4' || argv[4][0]=='3'))
          env->g = random_game_ext(strtol(argv[1],NULL,10), strtol(argv[2],NULL,10), argv[3][0]=='S', argv[4][0]=='4');
        else {
          fprintf(stderr,"Usage : Wrong argument ! Follow the correct format for random game !\n");   fprintf(stderr,"Usage : ./net_sdl <width> <height> [S|N] [3|4]\n");
          exit(EXIT_FAILURE);
        }
        break;
      default:
        fprintf(stderr,"Usage : Wrong argument ! Follow the correct format for random game !\n");   fprintf(stderr,"Usage : ./net_sdl <width> <height> [S|N] [3|4]\n");
        exit(EXIT_FAILURE);
    }
  #endif
  int gw = game_width( (cgame) env->g);                                  
  int gh = game_height( (cgame) env->g);                              

  //Init positions
  int cell1 = h/(gh+2);
  int cell2 = w/(gw+2);
  int min = MIN(cell1,cell2);
  int cell_size = min%2==0?min:(min-1);
  env->piece_x = cell_size + (w - (cell_size*(gw+1)))/2;
  env->piece_y = cell_size + (h - (cell_size*(gh+1)))/2;


  //Init all textures
  env->background = IMG_LoadTexture(ren, BACKGROUND);
  if(!env->background) ERROR("IMG_LoadTexture: %s\n", BACKGROUND);

  env->cross = IMG_LoadTexture(ren, CROSS_P);
  if(!env->cross) ERROR("IMG_LoadTexture: %s\n", CROSS_P);

  env->corner = IMG_LoadTexture(ren, CORNER_P);
  if(!env->corner) ERROR("IMG_LoadTexture: %s\n", CORNER_P);

  env->segment = IMG_LoadTexture(ren, SEGMENT_P);
  if(!env->segment) ERROR("IMG_LoadTexture: %s\n", SEGMENT_P);

  env->leaf = IMG_LoadTexture(ren, LEAF_P);
  if(!env->leaf) ERROR("IMG_LoadTexture: %s\n", LEAF_P);

  env->tee = IMG_LoadTexture(ren, TEE_P);
  if(!env->tee) ERROR("IMG_LoadTexture: %s\n", TEE_P);
  /* init text texture using Arial font */
  SDL_Color color = { 0, 0, 255, 255 }; /* blue color in RGBA */
  TTF_Font * font = TTF_OpenFont(FONT, FONTSIZE);
  if(!font) ERROR("TTF_OpenFont: %s\n", FONT);
  TTF_SetFontStyle(font, TTF_STYLE_BOLD); // TTF_STYLE_ITALIC | TTF_STYLE_NORMAL
  SDL_Surface * surf = TTF_RenderText_Blended(font, "Congratulations!", color); // blended rendering for ultra nice text
  env->text = SDL_CreateTextureFromSurface(ren, surf);
  SDL_FreeSurface(surf);
  TTF_CloseFont(font);
  return env;
}
     
/* **************************************************************** */
     
void render(SDL_Window* win, SDL_Renderer* ren, Env * env)
{
  SDL_Rect rect;

  int w, h;
  SDL_GetWindowSize(win, &w, &h);


  SDL_RenderCopy(ren, env->background, NULL, NULL);

  int gw = game_width( (cgame) env->g);                                  
  int gh = game_height( (cgame) env->g);
  int cell1 = h/(gh+2);
  int cell2 = w/(gw+2);
  int min = MIN(cell1,cell2);
  int cell_size = min%2==0?min:(min-1);
  env->piece_x = cell_size + (w - (cell_size*(gw+1)))/2;
  env->piece_y = cell_size + (h - (cell_size*(gh+1)))/2;
  rect.w = cell_size;
  rect.h = cell_size;

  //Boucle pour afficher toutes les cases
  SDL_Point* p = (SDL_Point*) malloc(sizeof(SDL_Point));
  for(int j = 0; j<gh; j++){     
   for(int i = 0; i<gw; i++){
     rect.x = env->piece_x+(i*cell_size) - rect.w/2; rect.y = env->piece_y+(j*cell_size) - rect.h/2; 
     p->x = cell_size/2;
     p->y = cell_size/2;
     switch(get_piece((cgame)env->g,i,gh-j-1)){
        case CORNER:
         SDL_RenderCopyEx(ren, env->corner, NULL, &rect, 90*get_current_dir(env->g, i, gh-j-1), (const SDL_Point *)p ,SDL_FLIP_NONE);
         break;
        case LEAF:
         SDL_RenderCopyEx(ren, env->leaf, NULL, &rect, 90*get_current_dir(env->g, i, gh-j-1), (const SDL_Point *)p,SDL_FLIP_NONE);
         break;
        case SEGMENT:
         SDL_RenderCopyEx(ren, env->segment, NULL, &rect, 90*get_current_dir(env->g, i, gh-j-1), (const SDL_Point *)p,SDL_FLIP_NONE);
         break;
        case TEE:
         SDL_RenderCopyEx(ren, env->tee, NULL, &rect, 90*get_current_dir(env->g, i, gh-j-1), (const SDL_Point *)p,SDL_FLIP_NONE);
         break;
        case CROSS:
         SDL_RenderCopyEx(ren, env->cross, NULL, &rect, 90*get_current_dir(env->g, i, gh-j-1), (const SDL_Point *)p,SDL_FLIP_NONE);
         break;
        default:
          fprintf(stderr,"Error : WTF is that game ?!\n");
          exit(EXIT_FAILURE);
      }
    }
  }

  // Render win text
  if(is_game_over((cgame)env->g)){
    SDL_QueryTexture(env->text, NULL, NULL, &rect.w, &rect.h);
    rect.x = w/2 - rect.w/2; rect.y = FONTSIZE - rect.h/2; 
    SDL_RenderCopy(ren, env->text, NULL, &rect);
  }

}
/* **************************************************************** */
     
bool process(SDL_Window* win, SDL_Renderer* ren, Env * env, SDL_Event * e)
{     
  int w, h;
  SDL_GetWindowSize(win, &w, &h);
  if (e->type == SDL_QUIT) {
    return true;
  }
  
  #ifdef __ANDROID__
    if(!is_game_over((cgame)env->g)){
      if (e->type == SDL_FINGERDOWN){
        SDL_Point mouse;
        mouse.x = e->tfinger.x * w;
        mouse.y = e->tfinger.y * h;
        double gw = game_width( (cgame) env->g);                                  
        double gh = game_height( (cgame) env->g);  
        int cell1 = h/(gh+2);
        int cell2 = w/(gw+2);
        int min = MIN(cell1,cell2);
        double cell_size = min%2==0?min:(min-1);
        int i = (int)floor(((double)mouse.x - (double)env->piece_x + cell_size/2.0)/cell_size);
        int j = gh -1 - (int)floor(((double)mouse.y - (double)env->piece_y + cell_size/2.0)/cell_size);
        if(i<gw && i>=0 && j<gh && j>=0)
          rotate_piece_one(env->g,i,j);
      }
    }else{
      return true;
    }
  #else
    if(!is_game_over((cgame)env->g)){
      if (e->type == SDL_MOUSEBUTTONDOWN){
        SDL_Point mouse;
        SDL_GetMouseState(&mouse.x, &mouse.y);
        double gw = game_width( (cgame) env->g);                                  
        double gh = game_height( (cgame) env->g);  
        int cell1 = h/(gh+2);
        int cell2 = w/(gw+2);
        int min = MIN(cell1,cell2);
        double cell_size = min%2==0?min:(min-1);
        int i = (int)floor(((double)mouse.x - (double)env->piece_x + cell_size/2.0)/cell_size);
        int j = gh -1 - (int)floor(((double)mouse.y - (double)env->piece_y + cell_size/2.0)/cell_size);
        if(i<gw && i>=0 && j<gh && j>=0)
          rotate_piece_one(env->g,i,j);  
      }
    }
    else {
      return true;
    }
  #endif
  
  return false; 
}

/* **************************************************************** */

void clean(SDL_Window* win, SDL_Renderer* ren, Env * env)
{
  SDL_DestroyTexture(env->background);
  SDL_DestroyTexture(env->cross);
  SDL_DestroyTexture(env->leaf);
  SDL_DestroyTexture(env->tee);
  SDL_DestroyTexture(env->corner);
  SDL_DestroyTexture(env->segment);
  delete_game(env->g);

  free(env);
}
     
/* **************************************************************** */

