# Makefile
CC = clang
CXX = g++
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

# GLFW directory
GLFW_DIR = deps/glfw

# ImGui source files (excluding imgui_demo.cpp - not needed for production)
IMGUI_SRC = $(IMGUI_DIR)/imgui.cpp \
            $(IMGUI_DIR)/imgui_draw.cpp \
            $(IMGUI_DIR)/imgui_tables.cpp \
            $(IMGUI_DIR)/imgui_widgets.cpp \
            $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp \
            $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

# ImPlot source files
IMPLOT_SRC = $(IMPLOT_DIR)/implot.cpp \
             $(IMPLOT_DIR)/implot_items.cpp

# GLFW source files - common
GLFW_SRC_COMMON = $(GLFW_DIR)/src/context.c \
                  $(GLFW_DIR)/src/init.c \
                  $(GLFW_DIR)/src/input.c \
                  $(GLFW_DIR)/src/monitor.c \
                  $(GLFW_DIR)/src/platform.c \
                  $(GLFW_DIR)/src/vulkan.c \
                  $(GLFW_DIR)/src/window.c \
                  $(GLFW_DIR)/src/egl_context.c \
                  $(GLFW_DIR)/src/osmesa_context.c \
                  $(GLFW_DIR)/src/null_init.c \
                  $(GLFW_DIR)/src/null_monitor.c \
                  $(GLFW_DIR)/src/null_window.c \
                  $(GLFW_DIR)/src/null_joystick.c

# ImGui object files
IMGUI_OBJ = $(IMGUI_SRC:.cpp=.o)
IMPLOT_OBJ = $(IMPLOT_SRC:.cpp=.o)

# ImGui includes
IMGUI_INCLUDES = -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I$(IMPLOT_DIR)

# GLFW includes
GLFW_INCLUDES = -I$(GLFW_DIR)/include

# Platform-specific settings
ifeq ($(OS),linux)
# GLFW Linux-specific sources (X11)
GLFW_SRC_PLATFORM = $(GLFW_DIR)/src/x11_init.c \
                    $(GLFW_DIR)/src/x11_monitor.c \
                    $(GLFW_DIR)/src/x11_window.c \
                    $(GLFW_DIR)/src/xkb_unicode.c \
                    $(GLFW_DIR)/src/posix_poll.c \
                    $(GLFW_DIR)/src/posix_time.c \
                    $(GLFW_DIR)/src/posix_thread.c \
                    $(GLFW_DIR)/src/posix_module.c \
                    $(GLFW_DIR)/src/glx_context.c \
                    $(GLFW_DIR)/src/linux_joystick.c

GLFW_DEFINES = -D_GLFW_X11
# Note: Use full path for libraries without .so symlinks
GLFW_LIBS = -lX11 -lXrandr -lXinerama -lXcursor -lXi /lib/x86_64-linux-gnu/libXxf86vm.so.1

CFLAGS = -include $(RAYFORCE_DIR)/core/def.h -fPIC -Wall -Wextra -std=$(STD) -g -O0 -march=native -fsigned-char -DDEBUG -m64
LIBS = -lm -ldl -lpthread -lGL $(GLFW_LIBS)
endif

ifeq ($(OS),darwin)
# GLFW macOS-specific sources
GLFW_SRC_PLATFORM = $(GLFW_DIR)/src/cocoa_init.m \
                    $(GLFW_DIR)/src/cocoa_monitor.m \
                    $(GLFW_DIR)/src/cocoa_window.m \
                    $(GLFW_DIR)/src/cocoa_joystick.m \
                    $(GLFW_DIR)/src/cocoa_time.c \
                    $(GLFW_DIR)/src/nsgl_context.m \
                    $(GLFW_DIR)/src/posix_thread.c \
                    $(GLFW_DIR)/src/posix_module.c

GLFW_DEFINES = -D_GLFW_COCOA
GLFW_LIBS =

CFLAGS = -include $(RAYFORCE_DIR)/core/def.h -fPIC -Wall -Wextra -std=$(STD) -g -O0 -march=native -fsigned-char -DDEBUG -m64
LIBS = -lm -ldl -lpthread -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -lstdc++
endif

# GLFW C flags (separate from main CFLAGS to avoid def.h force-include)
# Note: _GNU_SOURCE needed for clock_gettime, CLOCK_MONOTONIC, O_CLOEXEC, etc.
GLFW_CFLAGS = -fPIC -Wall -std=c99 -g -O0 -D_GNU_SOURCE $(GLFW_DEFINES) -I$(GLFW_DIR)/include -I$(GLFW_DIR)/src

# All GLFW sources
GLFW_SRC = $(GLFW_SRC_COMMON) $(GLFW_SRC_PLATFORM)
GLFW_OBJ = $(GLFW_SRC:.c=.o)

# C++ flags for ImGui (includes GLFW headers)
# Note: C++ files should NOT include rayforce/core directly (shadows system string.h)
CXXFLAGS = -std=c++11 $(IMGUI_INCLUDES) $(GLFW_INCLUDES) -fPIC -Wall -Wextra -g -O0

# Includes for C files (can include rayforce core)
INCLUDES_C = -Iinclude -I$(RAYFORCE_DIR)/core $(IMGUI_INCLUDES) $(GLFW_INCLUDES)

# Includes for C++ files (exclude rayforce core to avoid shadowing system headers)
INCLUDES_CXX = -Iinclude $(IMGUI_INCLUDES) $(GLFW_INCLUDES)

# C source files
SRC_C = src/main.c src/queue.c src/widget.c src/context.c src/rayforce_thread.c
OBJ_C = $(SRC_C:.c=.o)

# C++ source files (raygui)
SRC_CXX = src/ui.cpp src/widget_registry.cpp src/grid_renderer.cpp src/chart_renderer.cpp src/repl_renderer.cpp
OBJ_CXX = $(SRC_CXX:.cpp=.o)

TARGET = raygui

default: rayforce_lib $(TARGET)

rayforce_lib:
	$(MAKE) -C $(RAYFORCE_DIR) lib-debug

# Use C++ linker since we have C++ objects
$(TARGET): $(OBJ_C) $(OBJ_CXX) $(IMGUI_OBJ) $(IMPLOT_OBJ) $(GLFW_OBJ)
	$(CXX) -o $@ $^ $(RAYFORCE_LIB) $(LIBS)

# C source compilation for raygui
src/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES_C) -c $< -o $@

# C++ source compilation for raygui
src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES_CXX) -c $< -o $@

# C++ source compilation for ImGui
$(IMGUI_DIR)/%.o: $(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(IMGUI_DIR)/backends/%.o: $(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(IMPLOT_DIR)/%.o: $(IMPLOT_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -I$(IMGUI_DIR) -c $< -o $@

# C source compilation for GLFW
$(GLFW_DIR)/src/%.o: $(GLFW_DIR)/src/%.c
	$(CC) $(GLFW_CFLAGS) -c $< -o $@

# Fetch dependencies
deps:
	./deps/fetch_deps.sh

clean:
	rm -f $(OBJ_C) $(OBJ_CXX) $(IMGUI_OBJ) $(IMPLOT_OBJ) $(GLFW_OBJ) $(TARGET)
	$(MAKE) -C $(RAYFORCE_DIR) clean

.PHONY: default clean rayforce_lib deps
