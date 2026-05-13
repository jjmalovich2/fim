CXX = g++
CC = gcc

CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Iinclude -Ivendor/blake3/c -MMD -MP -DBLAKE3_NO_SSE2 -DBLAKE3_NO_SSE41 -DBLAKE3_NO_AVX2 -DBLAKE3_NO_AVX512
CFLAGS = -O3 -Wall -Ivendor/blake3/c -DBLAKE3_NO_SSE2 -DBLAKE3_NO_SSE41 -DBLAKE3_NO_AVX2 -DBLAKE3_NO_AVX512

SRC_DIR = src
BUILD_DIR = build
VENDOR_DIR = vendor/blake3/c

# grab every .cpp file
CXX_SRCS := $(wildcard $(SRC_DIR)/*.cpp)

# grab blake3 src
C_SRCS := $(VENDOR_DIR)/blake3.c \
		  $(VENDOR_DIR)/blake3_dispatch.c \
		  $(VENDOR_DIR)/blake3_portable.c

CXX_OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(CXX_SRCS))
C_OBJS := $(patsubst $(VENDOR_DIR)/%.c, $(BUILD_DIR)/vendor_%.o, $(C_SRCS))
OBJS := $(CXX_OBJS) $(C_OBJS)

DEPS := $(OBJS:.o=.d)

TARGET = fim

all: $(TARGET)

$(TARGET): $(OBJS)
	@echo "Linking $@"
	@$(CXX) $(CXXFLAGS) -o $@ $^
	@echo "Build successful ~ run with ./$(TARGET)"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling C++: $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/vendor_%.o: $(VENDOR_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling C: $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning build directory..."
	@rm -rf $(BUILD_DIR) $(TARGET)

-include $(DEPS)

.PHONY: all clean