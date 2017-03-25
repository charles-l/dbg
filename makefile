OBJ = dbg_ui.o imgui_impl_sdl.o imgui/imgui.o imgui/imgui_draw.o
LDFLAGS = -lSDL2 -lSDL2_ttf -lGL
CFLAGS =

%.o: %.cpp
	g++ $(CFLAGS) -c $< -o $@

all: $(OBJ)
	g++ -o dbg $(OBJ) $(LDFLAGS)
