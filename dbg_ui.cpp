#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <GL/glew.h>
#include <err.h>
#include <assert.h>
#include "dbg.h"
#include "imgui/imgui.h"
#include "imgui_impl_sdl.h"

#include <errno.h>
#define BUF_SIZE 1024

const unsigned int WIN_W = 800;
const unsigned int WIN_H = 600;
const char *WIN_T = "dbg";
TTF_Font *font;
SDL_Color font_color = {255, 255, 255};

typedef struct {
  off_t rstart; // remote start address
  char *buf;
  size_t n;
} mem_view;

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
      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

  SDL_GLContext glcontext = SDL_GL_CreateContext(window);
  ImGui_ImplSdl_Init(window);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  font = TTF_OpenFont("font/SourceCodePro.ttf", 12);
  if(!font) errx(1, "%s", TTF_GetError());

  off_t offset = 0x0;
  char buf[BUF_SIZE];
  pid_t pid = 1133;

  int n;
  struct pmap *pmaps = read_mem_maps(pid, &n);

  bool quit = false;
  while(!quit) {
    SDL_Event e;
    while(SDL_PollEvent(&e) != 0) {
      ImGui_ImplSdl_ProcessEvent(&e);
      if(e.type == SDL_QUIT)
        quit = true;
    }
    ImGui_ImplSdl_NewFrame(window);

    ImGui::Begin("memory map");
    for(int i = 0; i < n; i++) {
      if(ImGui::Button(pmaps[i].mapname)) {
        offset = pmaps[i].begin;
      }

      ImGui::Text("%p %lu %s",
          (void *) pmaps[i].begin,
          pmaps[i].size,
          pmaps[i].perm);
    }
    ImGui::End();

    ImGui::Begin("memory dump");
#ifdef x86 // 32 bit
    ImGui::InputInt("offset", (int *) &offset, 1, 100, ImGuiInputTextFlags_CharsHexadecimal);
#else // 64 bit
    ImGui::InputInt("offset (upper)", (int *) &offset, 1, 100, ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputInt("offset (lower)", (int *) &offset - 4, 1, 100, ImGuiInputTextFlags_CharsHexadecimal);
#endif
    if (read_mem(pid, offset, buf, BUF_SIZE) == -1) {
      ImGui::Text("error: %s", strerror(errno));
    } else {
      for(int i = 0; i < BUF_SIZE / WORD_SIZE; i++) {
        char s[64] = {0};
        sprintf(s, "0x%p", offset + i * WORD_SIZE);

        for(int j = 0; j < WORD_SIZE; j++) {
          sprintf(s, "%s %02hhX", s, buf[i * WORD_SIZE + j]);
        }

        ImGui::Text(s);
      }
    }
    ImGui::End();

    {
      glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
      glClearColor(10, 10, 10, 255);
      glClear(GL_COLOR_BUFFER_BIT);

      glPixelZoom(5, 5);
      glDrawPixels(WORD_SIZE, BUF_SIZE / WORD_SIZE, GL_RGB, GL_UNSIGNED_BYTE, buf);

      ImGui::Render();
      SDL_GL_SwapWindow(window);
    }
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_Quit();
  return 0;
}
