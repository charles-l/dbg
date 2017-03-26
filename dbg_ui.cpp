#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <err.h>
#include <assert.h>
#include "dbg.h"
#include "imgui/imgui.h"
#include "imgui_impl_sdl.h"

#include <capstone/capstone.h>
#include <errno.h>
#define BUF_SIZE 1024

const unsigned int WIN_W = 800;
const unsigned int WIN_H = 600;
const char *WIN_T = "dbg";

typedef struct {
  off_t rstart; // remote start address
  char *buf;
  size_t n;
} mem_view;

int main(int argc, char **argv) {
  { // init
    SDL_Init(SDL_INIT_VIDEO);
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
    for(int i = 0; i < n && pmaps[i].begin != 0; i++) {
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

    ImGui::Begin("disassembler");
    {
      // TODO: make this not suck
      csh handle;
      cs_insn *insn;
      size_t n;

      if(cs_open(CS_ARCH_X86, CS_MODE_64, &handle) == CS_ERR_OK) {
        n = cs_disasm(handle, (uint8_t *) buf, BUF_SIZE, offset, 0, &insn);
        if(n) {
          for (int i = 0; i < n; i++) {
            ImGui::Text("0x%" PRIx64 ":\t%s\t\t%s\n", insn[i].address, insn[i].mnemonic, insn[i].op_str);
          }
          cs_free(insn, n);
        }
      }
      cs_close(&handle);
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
