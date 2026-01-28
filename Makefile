# Makefile
CC = clang
STD = c17

ifeq ($(OS),)
OS := $(shell uname -s | tr "[:upper:]" "[:lower:]")
endif

# Build rayforce library first
RAYFORCE_DIR = deps/rayforce
RAYFORCE_LIB = $(RAYFORCE_DIR)/librayforce.a

ifeq ($(OS),linux)
CFLAGS = -include $(RAYFORCE_DIR)/core/def.h -fPIC -Wall -Wextra -std=$(STD) -g -O0 -march=native -fsigned-char -DDEBUG -m64
LIBS = -lm -ldl -lpthread -lGL -lglfw
endif

ifeq ($(OS),darwin)
CFLAGS = -include $(RAYFORCE_DIR)/core/def.h -fPIC -Wall -Wextra -std=$(STD) -g -O0 -march=native -fsigned-char -DDEBUG -m64
LIBS = -lm -ldl -lpthread -framework OpenGL -framework Cocoa -framework IOKit -lglfw
endif

INCLUDES = -Iinclude -I$(RAYFORCE_DIR)/core -Ideps/imgui -Ideps/implot -Ideps/glfw/include

SRC = src/main.c src/queue.c src/widget.c src/context.c
OBJ = $(SRC:.c=.o)
TARGET = raygui

default: rayforce_lib $(TARGET)

rayforce_lib:
	$(MAKE) -C $(RAYFORCE_DIR) lib-debug

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(RAYFORCE_LIB) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
	$(MAKE) -C $(RAYFORCE_DIR) clean

.PHONY: default clean rayforce_lib
