# Many Pictures - Makefile / Many Pictures - Makefile
# Pure C implementation of advanced image viewer

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -Isrc $(shell pkg-config --cflags x11 cairo)
LDFLAGS = -lm -lpthread $(shell pkg-config --libs x11 cairo)
DEBUG_FLAGS = -g -DDEBUG
RELEASE_FLAGS = -O3 -DNDEBUG

# Directories / 디렉토리
SRC_DIR = src
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Target / 타겟
TARGET = $(BIN_DIR)/manypictures

# Source files / 소스 파일
CORE_SOURCES = \
	$(SRC_DIR)/core/memory.c \
	$(SRC_DIR)/core/image.c \
	$(SRC_DIR)/core/fast_io.c

FORMAT_SOURCES = \
	$(SRC_DIR)/formats/bmp.c \
	$(SRC_DIR)/formats/png.c \
	$(SRC_DIR)/formats/jpeg.c \
	$(SRC_DIR)/formats/gif.c \
	$(SRC_DIR)/formats/tiff.c \
	$(SRC_DIR)/formats/webp.c

CODEC_SOURCES = \
	$(SRC_DIR)/codecs/deflate.c \
	$(SRC_DIR)/codecs/jpeg.c

EXIF_SOURCES = \
	$(SRC_DIR)/exif/exif.c

OPERATION_SOURCES = \
	$(SRC_DIR)/operations/color_ops.c \
	$(SRC_DIR)/operations/edit_ops.c

GUI_SOURCES = \
	$(SRC_DIR)/gui/gui.c

MAIN_SOURCES = \
	$(SRC_DIR)/main.c

ALL_SOURCES = $(CORE_SOURCES) $(FORMAT_SOURCES) $(CODEC_SOURCES) $(EXIF_SOURCES) $(OPERATION_SOURCES) $(GUI_SOURCES) $(MAIN_SOURCES)

# Object files / 오브젝트 파일
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(ALL_SOURCES))

# Header dependencies / 헤더 의존성
HEADERS = \
	$(SRC_DIR)/core/types.h \
	$(SRC_DIR)/core/memory.h \
	$(SRC_DIR)/core/image.h \
	$(SRC_DIR)/core/fast_io.h \
	$(SRC_DIR)/codecs/deflate.h \
	$(SRC_DIR)/codecs/jpeg.h \
	$(SRC_DIR)/exif/exif.h \
	$(SRC_DIR)/operations/color_ops.h \
	$(SRC_DIR)/operations/edit_ops.h \
	$(SRC_DIR)/gui/gui.h

# Default target / 기본 타겟
all: $(TARGET)

# Create directories / 디렉토리 생성
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)/core
	@mkdir -p $(OBJ_DIR)/formats
	@mkdir -p $(OBJ_DIR)/codecs
	@mkdir -p $(OBJ_DIR)/operations
	@mkdir -p $(OBJ_DIR)/exif
	@mkdir -p $(OBJ_DIR)/gui
	@mkdir -p $(OBJ_DIR)/video
	@mkdir -p $(OBJ_DIR)/utils

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# Compile object files / 오브젝트 파일 컴파일
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(OBJ_DIR)
	@echo "Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Link executable / 실행 파일 링크
$(TARGET): $(OBJECTS) | $(BIN_DIR)
	@echo "Linking $(TARGET)..."
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)
	@echo "Build complete: $(TARGET)"

# Debug build / 디버그 빌드
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGET)

# Release build / 릴리스 빌드
release: CFLAGS += $(RELEASE_FLAGS)
release: clean $(TARGET)

# Clean build artifacts / 빌드 산출물 정리
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)

# Install target / 설치 타겟
install: $(TARGET)
	@echo "Installing Many Pictures..."
	@install -m 755 $(TARGET) /usr/local/bin/manypictures
	@echo "Installation complete"

# Uninstall / 제거
uninstall:
	@echo "Uninstalling Many Pictures..."
	@rm -f /usr/local/bin/manypictures
	@echo "Uninstall complete"

# Run / 실행
run: $(TARGET)
	@$(TARGET)

# Run with test image / 테스트 이미지로 실행
test: $(TARGET)
	@echo "Running tests..."
	@$(TARGET) --help

# Generate documentation / 문서 생성
docs:
	@echo "Generating documentation..."
	@doxygen Doxyfile 2>/dev/null || echo "Doxygen not installed"

# Code statistics / 코드 통계
stats:
	@echo "Code Statistics:"
	@echo "================"
	@find $(SRC_DIR) -name "*.c" -o -name "*.h" | xargs wc -l | tail -1
	@echo ""
	@echo "Files by type:"
	@echo "  C sources:   $$(find $(SRC_DIR) -name "*.c" | wc -l)"
	@echo "  C headers:   $$(find $(SRC_DIR) -name "*.h" | wc -l)"

# Check for memory leaks (requires valgrind) / 메모리 누수 확인 (valgrind 필요)
memcheck: $(TARGET)
	@valgrind --leak-check=full --show-leak-kinds=all $(TARGET) test.bmp

# Format code (requires clang-format) / 코드 포맷팅 (clang-format 필요)
format:
	@find $(SRC_DIR) -name "*.c" -o -name "*.h" | xargs clang-format -i

# Static analysis (requires cppcheck) / 정적 분석 (cppcheck 필요)
analyze:
	@cppcheck --enable=all --suppress=missingIncludeSystem $(SRC_DIR)

# Help / 도움말
help:
	@echo "Many Pictures - Build System"
	@echo "============================"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build the project (default)"
	@echo "  debug      - Build with debug symbols"
	@echo "  release    - Build optimized release version"
	@echo "  clean      - Remove build artifacts"
	@echo "  install    - Install to /usr/local/bin"
	@echo "  uninstall  - Remove from /usr/local/bin"
	@echo "  run        - Build and run the application"
	@echo "  test       - Run basic tests"
	@echo "  docs       - Generate documentation"
	@echo "  stats      - Show code statistics"
	@echo "  memcheck   - Check for memory leaks"
	@echo "  format     - Format source code"
	@echo "  analyze    - Run static analysis"
	@echo "  help       - Show this help message"

.PHONY: all debug release clean install uninstall run test docs stats memcheck format analyze help
