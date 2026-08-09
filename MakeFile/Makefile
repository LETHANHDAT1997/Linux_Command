# Compiler
CC := gcc

# Architecture selection
ARCH ?= x86_64

# Directories
SRC_DIR := src
OBJ_DIR := $(SRC_DIR)/obj
INC_DIR := include

# Target name
TARGET := lab3

# Source files
SRC := $(wildcard $(SRC_DIR)/*.c)

# Object files
OBJ := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))

# Header dependencies
DEPS := $(wildcard $(INC_DIR)/*.h)

# Library selection
ifeq ($(ARCH),aarch64)
LIB_DIR := lib_aarch64
LIBRARY := $(LIB_DIR)/lib_socfun.a
else
LIB_DIR := lib
LIBRARY := $(LIB_DIR)/libsocfun.a
endif

# Compile flags
CFLAGS := -Wall -Wextra -I$(INC_DIR)

# Default target
all: $(TARGET)

# Link target
$(TARGET): $(OBJ)
	$(CC) $^ $(LIBRARY) -o $@

# Compile source -> object
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(DEPS)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
.PHONY: all clean

clean:
	rm -f $(OBJ_DIR)/*.o $(TARGET)
