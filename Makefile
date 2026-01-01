# # ====================================================================
# # KHAI BÁO BIẾN
# # ====================================================================

# CXX = D:\msys64\usr\bin\ccache.exe g++
# CXXFLAGS = -Wall -std=c++17 -I src -I src/libs -I tests
# BUILD_DIR = build

# SRC = $(wildcard src/*.cpp src/*/*.cpp src/*/*/*.cpp tests/*.cpp)
# OBJ = $(SRC:%.cpp=$(BUILD_DIR)/%.o)
# DIRS = $(sort $(dir $(OBJ)))

# TARGET = $(BUILD_DIR)/autolang.exe

# .PHONY: all clean create_dirs
# all: $(TARGET)

# # ====================================================================
# # QUY TẮC XÂY DỰNG
# # ====================================================================

# create_dirs:
# 	@echo Creating directories...
# 	@for %%d in ($(DIRS)) do if not exist "%%d" mkdir "%%d"

# $(TARGET): $(OBJ)
# 	@echo Linking: $@
# 	$(CXX) $(OBJ) -o $@ -mconsole

# $(BUILD_DIR)/%.o: %.cpp | create_dirs
# 	@echo Compiling: $<
# 	$(CXX) $(CXXFLAGS) -c $< -o $@

# clean:
# 	@echo Cleaning...
# 	@if exist $(TARGET) del /Q $(TARGET)
# 	@for %%f in ($(OBJ)) do if exist "%%f" del /Q "%%f"
# 	@for %%d in ($(DIRS)) do if exist "%%d" rmdir /S /Q "%%d"

# build/autolang: tests/main.cpp src/*.cpp
main:
	g++ tests/main.cpp -I src -o build/autolang