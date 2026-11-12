CXX ?= g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -I.deps/imgui -I.deps/imgui/backends -I.deps/SDL/include
IMGUI := .deps/imgui
SOURCES := src/main.cpp $(IMGUI)/imgui.cpp $(IMGUI)/imgui_draw.cpp $(IMGUI)/imgui_tables.cpp $(IMGUI)/imgui_widgets.cpp $(IMGUI)/backends/imgui_impl_sdl2.cpp $(IMGUI)/backends/imgui_impl_sdlrenderer2.cpp
SDL_LIB ?= -l:libSDL2-2.0.so.0
build/replay-viewer: $(SOURCES)
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(SOURCES) $(SDL_LIB) -o $@
.PHONY: test
test: build/replay-viewer
	./build/replay-viewer --self-test
	SDL_VIDEODRIVER=dummy ./build/replay-viewer --smoke-test data/sample.replay
