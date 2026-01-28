# Makefile
CC = clang
CXX = clang++
STD = c17

ifeq ($(OS),)
OS := $(shell uname -s | tr "[:upper:]" "[:lower:]")
endif

# Build rayforce library first
RAYFORCE_DIR = deps/rayforce
RAYFORCE_LIB = $(RAYFORCE_DIR)/librayforce.a

# ImGui/ImPlot directories
IMGUI_DIR = deps/imgui
IMPLOT_DIR = deps/implot
GLFW_DIR = deps/glfw

# ImGui source files
IMGUI_SRC = $(IMGUI_DIR)/imgui.cpp \
            $(IMGUI_DIR)/imgui_draw.cpp \
            $(IMGUI_DIR)/imgui_tables.cpp \
            $(IMGUI_DIR)/imgui_widgets.cpp \
            $(IMGUI_DIR)/imgui_demo.cpp \
            $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp \
            $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

# ImPlot source files
IMPLOT_SRC = $(IMPLOT_DIR)/implot.cpp \
             $(IMPLOT_DIR)/implot_items.cpp

# ImGui object files
IMGUI_OBJ = $(IMGUI_SRC:.cpp=.o)
IMPLOT_OBJ = $(IMPLOT_SRC:.cpp=.o)

# ImGui includes
IMGUI_INCLUDES = -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I$(IMPLOT_DIR)

# GLFW flags (try pkg-config first, fallback to local)
GLFW_CFLAGS = $(shell pkg-config --cflags glfw3 2>/dev/null || echo "-I$(GLFW_DIR)/include")
GLFW_LIBS = $(shell pkg-config --libs glfw3 2>/dev/null || echo "-lglfw")

# C++ flags for ImGui
CXXFLAGS = -std=c++11 $(IMGUI_INCLUDES) $(GLFW_CFLAGS) -fPIC -Wall -Wextra -g -O0

ifeq ($(OS),linux)
CFLAGS = -include $(RAYFORCE_DIR)/core/def.h -fPIC -Wall -Wextra -std=$(STD) -g -O0 -march=native -fsigned-char -DDEBUG -m64
LIBS = -lm -ldl -lpthread -lGL $(GLFW_LIBS) -lstdc++
endif

ifeq ($(OS),darwin)
CFLAGS = -include $(RAYFORCE_DIR)/core/def.h -fPIC -Wall -Wextra -std=$(STD) -g -O0 -march=native -fsigned-char -DDEBUG -m64
LIBS = -lm -ldl -lpthread -framework OpenGL -framework Cocoa -framework IOKit $(GLFW_LIBS) -lstdc++
endif

INCLUDES = -Iinclude -I$(RAYFORCE_DIR)/core $(IMGUI_INCLUDES) $(GLFW_CFLAGS)

SRC = src/main.c src/queue.c src/widget.c src/context.c src/rayforce_thread.c
OBJ = $(SRC:.c=.o)
TARGET = raygui

default: rayforce_lib $(TARGET)

rayforce_lib:
	$(MAKE) -C $(RAYFORCE_DIR) lib-debug

$(TARGET): $(OBJ) $(IMGUI_OBJ) $(IMPLOT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(RAYFORCE_LIB) $(LIBS)

# C source compilation
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# C++ source compilation for ImGui
$(IMGUI_DIR)/%.o: $(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(IMGUI_DIR)/backends/%.o: $(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(IMPLOT_DIR)/%.o: $(IMPLOT_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(IMGUI_DIR) -c $< -o $@

# Fetch dependencies
deps:
	./deps/fetch_deps.sh

clean:
	rm -f $(OBJ) $(IMGUI_OBJ) $(IMPLOT_OBJ) $(TARGET)
	$(MAKE) -C $(RAYFORCE_DIR) clean

.PHONY: default clean rayforce_lib deps
