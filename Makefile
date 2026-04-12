CXX = clang++
LAUNCHER = sccache
# Thêm -MMD -MP để tự động theo dõi các file header (.h)
CXXFLAGS = -O2 -pipe -std=c++17 -I src -Wall -Wextra -MMD -MP -Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare -Wno-reorder

# 1. Hàm đệ quy thuần Make (Tuyệt chiêu để không phụ thuộc vào lệnh OS)
rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# 2. Tự động quét toàn bộ file .cpp trong src và các thư mục con
# SRC = $(call rwildcard,src,*.cpp) tests/main.cpp
SRC = tests/main.cpp

# 3. Tạo danh sách file object và dependency
OBJ = $(patsubst %.cpp,build/%.o,$(SRC))
DEPS = $(OBJ:.o=.d)

# Thêm đuôi .exe cho chuẩn Windows CMD
TARGET = build/autolang.exe

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJ)
	@if not exist "build" mkdir "build"
	$(CXX) $(OBJ) -o $(TARGET)

run: all
	$(TARGET)

# 4. Rule biên dịch: Tạo thư mục con tương ứng trong build/ trước khi compile
build/%.o: %.cpp
	@if not exist "$(subst /,\,$(dir $@))" mkdir "$(subst /,\,$(dir $@))"
	$(LAUNCHER) $(CXX) $(CXXFLAGS) -c $< -o $@

# Include các file .d để Make biết khi nào header thay đổi thì cần build lại
-include $(DEPS)

clean:
	@if exist build rmdir /s /q build