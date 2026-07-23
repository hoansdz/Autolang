# ==========================================
# AUTOLANG COMPILER - CROSS-PLATFORM MAKEFILE
# ==========================================

CXX = g++
LAUNCHER = sccache

# 1. Include paths
# Only point to the CPR and Curl header directories.
CXXFLAGS = -O2 -pipe -std=c++17 -DNOMINMAX -DCURL_STATICLIB -I src \
           -I src/third_party/curl/include \
           -I src/third_party \
           -Wall -Wextra -MMD -MP \
           -Wno-unused-parameter -Wno-unused-variable -Wno-switch \
           -Wno-sign-compare -Wno-reorder

# 2. Link libraries
# Link the static Curl library and the required Windows networking libraries.
LIBS = src/third_party/libs/libcurl.a -lws2_32 -lwldap32 -lcrypt32 -ladvapi32 -lbcrypt

# Recursive Make function to find source files in subdirectories.
rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# Detect the target operating system.
ifeq ($(OS),Windows_NT)
	# Windows CMD commands
	FIX_PATH = $(subst /,\,$1)
	MKDIR = if not exist "$(call FIX_PATH,$1)" mkdir "$(call FIX_PATH,$1)"
	RMDIR = if exist "$(call FIX_PATH,$1)" rmdir /s /q "$(call FIX_PATH,$1)"
	TARGET = build/autolang.exe
	RUN_CMD = $(call FIX_PATH,$(TARGET))
else
	# Linux / macOS commands
	FIX_PATH = $1
	MKDIR = mkdir -p $1
	RMDIR = rm -rf $1
	TARGET = build/autolang
	RUN_CMD = ./$(TARGET)
endif

# 3. Source files
# Only compile Autolang sources.
# Uncomment the line below when additional source files are added.
# SRC = $(call rwildcard,src,*.cpp) tests/main.cpp
SRC = tests/main.cpp

OBJ = $(patsubst %.cpp,build/%.o,$(SRC))
DEPS = $(OBJ:.o=.d)

.PHONY: all run clean

# Default target.
all: $(TARGET)

# 4. Link object files into the final executable.
$(TARGET): $(OBJ)
	@echo [LINKING] $@
	$(LAUNCHER) $(CXX) $(OBJ) $(LIBS) -o $(TARGET)

# Build and run.
run: all
	@echo [RUNNING] $(TARGET)
	@$(RUN_CMD)

# 5. Compile each source file into an object file.
# Automatically create the output directory if needed.
build/%.o: %.cpp
	@$(call MKDIR,$(dir $@))
	@echo [COMPILING] $<
	$(LAUNCHER) $(CXX) $(CXXFLAGS) -c $< -o $@

# Include dependency files so changes to headers trigger recompilation.
-include $(DEPS)

# Remove all build artifacts.
clean:
	@echo [CLEANING] build directory...
	@$(call RMDIR,build)