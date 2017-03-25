#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <GL/glew.h>
#include <err.h>
#include <assert.h>
#include "dbg.h"
#include "imgui/imgui.h"
#include "imgui_impl_sdl.h"

#include <errno.h>

const unsigned int WIN_W = 800;
const unsigned int WIN_H = 600;
const char *WIN_T = "dbg";
TTF_Font *font;
SDL_Color font_color = {255, 255, 255};

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

  SDL_GLContext glcontext = SDL_GL_CreateContext(window);
  ImGui_ImplSdl_Init(window);
  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  font = TTF_OpenFont("font/SourceCodePro.ttf", 12);
  if(!font) errx(1, "%s", TTF_GetError());

  off_t start = 0x0;
  char offset_buf[11] = {0};

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

    ImGui::Begin("Memory map");
    for(int i = 0; i < n; i++) {
      ImGui::Text("%p %lu %s %s",
          (char *) pmaps[i].begin,
          pmaps[i].size,
          pmaps[i].perm,
          pmaps[i].mapname);
    }
    ImGui::End();


    ImGui::InputText("Offset", offset_buf, 11);

    if(sscanf(offset_buf, "0x%x", &start) == -1) {
      strcpy(offset_buf, "0x0");
      start = 0;
    }

    ImGui::InputInt("pid", &pid);
    ImGui::Text("%i\n", start);

    char buf[80];

    if (read_mem(pid, start, buf, 80) == -1) {
      ImGui::Text("error: %s", strerror(errno));
    } else {
      for(int i = 0; i < 80 / WORD_SIZE; i++) {
        char s[64] = {0};
        sprintf(s, "0x%08x", start + i * WORD_SIZE);

        for(int j = 0; j < WORD_SIZE; j++) {
          sprintf(s, "%s %02hhX", s, buf[i * WORD_SIZE + j]);
        }

        ImGui::Text(s);
      }
    }

    {
      glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
      glClearColor(10, 10, 10, 255);
      glClear(GL_COLOR_BUFFER_BIT);

      glPixelZoom(20, 20);
      glDrawPixels(8, 10, GL_RGB, GL_UNSIGNED_BYTE, buf);

      ImGui::Render();
      SDL_GL_SwapWindow(window);
    }
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_Quit();
  return 0;
}
