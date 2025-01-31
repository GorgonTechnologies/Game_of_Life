# Compiler
CC = gcc

# Compiler/Linker flags
CFLAGS_PRE = -std=c11 -Wall -g -I./src $(shell pkg-config --cflags cglm sdl3)
CFLAGS = -std=c11 -Wall -g -fsanitize=address -I./src $(shell pkg-config --cflags cglm sdl3)
LDFLAGS = $(shell pkg-config --libs cglm sdl3) -fsanitize=address -lGL 

# Executables
TARGET = program
PREPROCESSOR = preprocessor

# Source and build folders
SRC_DIR = src
BUILD_DIR = build

# Resource preprocessing
SRC_RESOURCES = $(BUILD_DIR)/resources.c
RESOURCES = $(wildcard $(SRC_DIR)/resources/*.vert $(SRC_DIR)/resources/*.frag)

# Default rule
all: $(BUILD_DIR)/$(TARGET)

# Ensure the build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build preprocessor
$(BUILD_DIR)/$(PREPROCESSOR): $(SRC_DIR)/preprocessor.c
	$(CC) $(CFLAGS_PRE) -o $@ $<

# Generate embedded resources file
$(SRC_RESOURCES): $(RESOURCES) $(BUILD_DIR)/$(PREPROCESSOR) | $(BUILD_DIR)
	$(BUILD_DIR)/$(PREPROCESSOR)

# Source files and object files
SRCS = $(wildcard $(SRC_DIR)/*.c $(BUILD_DIR)/*.c) $(SRC_RESOURCES)
# Exclude preprocessor class from build 
SRCS := $(filter-out $(SRC_DIR)/preprocessor.c, $(SRCS))

OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Link object files to create the executable
$(BUILD_DIR)/$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compile source files to object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(BUILD_DIR)/$(TARGET)
	./$(BUILD_DIR)/$(TARGET)

debug: $(BUILD_DIR)/$(TARGET)
	gdb ./$(BUILD_DIR)/$(TARGET)

run_pre: $(BUILD_DIR)/$(PREPROCESSOR)
	./$(BUILD_DIR)/$(PREPROCESSOR)

debug_pre: $(BUILD_DIR)/$(PREPROCESSOR)
	gdb ./$(BUILD_DIR)/$(PREPROCESSOR)

clean:
	rm -rf $(BUILD_DIR) $(SRC_RESOURCES)

# Phony targets
.PHONY: all clean
