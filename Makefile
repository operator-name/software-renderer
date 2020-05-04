PROJECT_NAME = software-renderer

# Define the names of key files
SOURCE_FILE = $(PROJECT_NAME).cpp
OBJECT_FILE = $(PROJECT_NAME).o
EXECUTABLE = $(PROJECT_NAME).out
WINDOW_SOURCE = libs/sdw/window.cpp
WINDOW_OBJECT = libs/sdw/window.o

# Build settings
COMPILER = g++ # clang++
COMPILER_OPTIONS = -c -pipe -Wall -std=c++11
DEBUG_OPTIONS = -ggdb -g3
FUSSY_OPTIONS = -Werror -pedantic
SANITIZER_OPTIONS = -O1 -fsanitize=undefined -fno-omit-frame-pointer #-fsanitize=address
SPEEDY_OPTIONS = -Ofast -funsafe-math-optimizations -march=native
LINKER_OPTIONS =

# Set up flags
SDW_COMPILER_FLAGS := -I./libs/sdw
GLM_COMPILER_FLAGS := -I./libs/glm
GLMT_COMPILER_FLAGS := -I./libs/glmt
# If you have a manual install of SDL, you might not have sdl2-config. Compiler flags should be something like: -I/usr/local/include/SDL2 -D_THREAD_SAFE
SDL_COMPILER_FLAGS := $(shell sdl2-config --cflags)
# If you have a manual install of SDL, you might not have sdl2-config. Linker flags should be something like: -L/usr/local/lib -lSDL2
SDL_LINKER_FLAGS := $(shell sdl2-config --libs)
SDW_LINKER_FLAGS := $(WINDOW_OBJECT)

LIBSDL2PP_COMPILER_FLAGS := -L./libs/libSDL2pp/lib -I./libs/libSDL2pp/include/

default: speedy

# Rule to help find errors (when you get a segmentation fault)
# NOTE: Needs the "Address Sanitizer" library to be installed in order to work (might not work on lab machines !)
diagnostic: window
	$(COMPILER) $(COMPILER_OPTIONS) $(FUSSY_OPTIONS) $(SANITIZER_OPTIONS) -o $(OBJECT_FILE) $(SOURCE_FILE) $(SDL_COMPILER_FLAGS) $(SDW_COMPILER_FLAGS) $(GLM_COMPILER_FLAGS) $(GLMT_COMPILER_FLAGS)  $(LIBSDL2PP_COMPILER_FLAGS)
	$(COMPILER) $(LINKER_OPTIONS) $(FUSSY_OPTIONS) $(SANITIZER_OPTIONS) -o $(EXECUTABLE) $(OBJECT_FILE) $(SDW_LINKER_FLAGS) $(SDL_LINKER_FLAGS)
	# ./$(EXECUTABLE)

# Rule to compile and link for production release
production: window
	$(COMPILER) $(COMPILER_OPTIONS) -o $(OBJECT_FILE) $(SOURCE_FILE) $(SDL_COMPILER_FLAGS) $(SDW_COMPILER_FLAGS) $(GLM_COMPILER_FLAGS) $(GLMT_COMPILER_FLAGS)  $(LIBSDL2PP_COMPILER_FLAGS)
	$(COMPILER) $(LINKER_OPTIONS) -o $(EXECUTABLE) $(OBJECT_FILE) $(SDW_LINKER_FLAGS) $(SDL_LINKER_FLAGS)
	# ./$(EXECUTABLE)

# Rule to compile and link for use with a debugger
debug: window
	$(COMPILER) $(COMPILER_OPTIONS) $(DEBUG_OPTIONS) -o $(OBJECT_FILE) $(SOURCE_FILE) $(SDL_COMPILER_FLAGS) $(SDW_COMPILER_FLAGS) $(GLM_COMPILER_FLAGS) $(GLMT_COMPILER_FLAGS)  $(LIBSDL2PP_COMPILER_FLAGS)
	$(COMPILER) $(LINKER_OPTIONS) $(DEBUG_OPTIONS) -o $(EXECUTABLE) $(OBJECT_FILE) $(SDW_LINKER_FLAGS) $(SDL_LINKER_FLAGS)
	# ./$(EXECUTABLE)

# Rule to build for high performance executable
speedy: window
	$(COMPILER) $(COMPILER_OPTIONS) $(SPEEDY_OPTIONS) -o $(OBJECT_FILE) $(SOURCE_FILE) $(SDL_COMPILER_FLAGS) $(SDW_COMPILER_FLAGS) $(GLM_COMPILER_FLAGS) $(GLMT_COMPILER_FLAGS)  $(LIBSDL2PP_COMPILER_FLAGS)
	$(COMPILER) $(LINKER_OPTIONS) $(SPEEDY_OPTIONS) -o $(EXECUTABLE) $(OBJECT_FILE) $(SDW_LINKER_FLAGS) $(SDL_LINKER_FLAGS)
	# ./$(EXECUTABLE)

libs/libSDL2pp/lib/libSDL2pp.a:
	cmake ./extlibs/libSDL2pp/ -DCMAKE_INSTALL_PREFIX=./libs/libSDL2pp -DSDL2PP_WITH_IMAGE=OFF -DSDL2PP_WITH_MIXER=OFF -DSDL2PP_WITH_TTF=OFF -DSDL2PP_ENABLE_LIVE_TESTS=OFF -DSDL2PP_WITH_EXAMPLES=OFF -DSDL2PP_WITH_TESTS=OFF -DSDL2PP_STATIC=ON
	cd ./extlibs/libSDL2pp && make install

.PHONY: window
window: $(WINDOW_OBJECT)

$(WINDOW_OBJECT):
	$(COMPILER) $(COMPILER_OPTIONS) -o $(WINDOW_OBJECT) $(WINDOW_SOURCE) $(SDL_COMPILER_FLAGS) $(GLM_COMPILER_FLAGS) $(GLMT_COMPILER_FLAGS) $(LIBSDL2PP_COMPILER_FLAGS)

run: $(EXECUTABLE)
	./$(EXECUTABLE)

mp4: PPM/frame00000.ppm
	ffmpeg -framerate 30 -i PPM/frame%05d.ppm -c:v libx264 -preset veryslow -crf 17 -tune animation out.mp4

dev:
	clang-format --style="{NamespaceIndentation: All}" -i *.*pp
	clang-format --style="{NamespaceIndentation: All}" -i libs/sdw/sdw/*.h
	clang-format --style="{NamespaceIndentation: All}" -i libs/sdw/*.*pp
	clang-format --style="{NamespaceIndentation: All}" -i libs/glmt/*.*pp
	clear
	make speedy
	make run

# Files to remove during clean
clean:
	rm -f $(WINDOW_OBJECT)
	rm -f $(OBJECT_FILE)
	rm -f $(EXECUTABLE)
	rm -f PPM/frame*.ppm
