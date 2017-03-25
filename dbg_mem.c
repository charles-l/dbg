#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <GL/glew.h>
#include <err.h>
#include <assert.h>
#include "dbg.h"

#define BYTEGRID(i, j) (i * (WIN_W / 4) + 5), (j * 25 + 5)

const unsigned int WIN_W = 400;
const unsigned int WIN_H = 25 * 30;
const char *WIN_T = "dbg";
TTF_Font *font;
SDL_Color font_color = {255, 255, 255};

void textbox_draw(SDL_Renderer *renderer, char *str, int x, int y) {
  SDL_Surface *s = TTF_RenderText_Blended(font, str, font_color);
  assert(s != NULL);

  SDL_Texture *m = SDL_CreateTextureFromSurface(renderer, s);
  SDL_Rect r = {x, y, s->w, s->h};
  SDL_SetRenderDrawColor(renderer, 24, 24, 24, 255);
  SDL_RenderFillRect(renderer, &r);
  SDL_RenderCopy(renderer, m, NULL, &r);
  SDL_FreeSurface(s);
  SDL_DestroyTexture(m);
}

int main(int argc, char **argv) {
  { // init
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    // maybe this works?
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 2);
    glEnable(GL_MULTISAMPLE);
  }

  SDL_Window *window = SDL_CreateWindow(
      WIN_T,
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      WIN_W,
      WIN_H,
      SDL_WINDOW_SHOWN);

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  font = TTF_OpenFont("font/SourceCodePro.ttf", 12);
  if(!font) errx(1, "%s", TTF_GetError());

  char buf[80];
  struct mem_file m = mem_file_load(1133);

  int quit = 0;
  while(!quit) {
    SDL_Event e;
    while(SDL_PollEvent(&e) != 0) {
      if(e.type == SDL_QUIT) {
        quit = 1;
      }
    }

    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderClear(renderer);

    off_t start = 0x0;

    mem_file_read(&m, start, buf, 80);
    for(int i = 0; i < 80 / WORD_SIZE; i++) {
      char s[128];
      sprintf(s, "0x%08x", start + (i * WORD_SIZE));
      for(int j = 0; j < WORD_SIZE; j++) {
        sprintf(s, "%s %02hhX", s, buf[(i * WORD_SIZE) + j]);
      }
      textbox_draw(renderer, s, BYTEGRID(0, i));
    }

    SDL_RenderPresent(renderer);
  }

  mem_file_close(&m);

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_Quit();
  return 0;
}
