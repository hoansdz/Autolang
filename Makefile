# ==========================================
# AUTOLANG COMPILER - CROSS-PLATFORM MAKEFILE
# ==========================================

CXX = g++
LAUNCHER = sccache

# 1. Đường dẫn Include chỉ cần trỏ vào thư mục header của CPR (không cần trỏ sâu vào curl nữa)
CXXFLAGS = -O2 -pipe -std=c++17 -DNOMINMAX -DCURL_STATICLIB -I src \
           -I src/third_party/curl/include \
           -I src/third_party \
           -Wall -Wextra -MMD -MP \
           -Wno-unused-parameter -Wno-unused-variable -Wno-switch \
           -Wno-sign-compare -Wno-reorder

# 2. Linker Libraries: Nạp trực tiếp file tĩnh của CPR, Curl và các thư viện mạng Windows
LIBS = src/third_party/libs/libcurl.a -lws2_32 -lwldap32 -lcrypt32 -ladvapi32 -lbcrypt

# Thuật toán đệ quy thuần Make để quét file trong thư mục con
rwildcard = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# Tự động nhận diện Hệ Điều Hành (Cross-platform Magic)
ifeq ($(OS),Windows_NT)
	# Lệnh chuẩn của Windows CMD
	FIX_PATH = $(subst /,\,$1)
	MKDIR = if not exist "$(call FIX_PATH,$1)" mkdir "$(call FIX_PATH,$1)"
	RMDIR = if exist "$(call FIX_PATH,$1)" rmdir /s /q "$(call FIX_PATH,$1)"
	TARGET = build/autolang.exe
	RUN_CMD = $(call FIX_PATH,$(TARGET))
else
	# Lệnh chuẩn của Linux / macOS
	FIX_PATH = $1
	MKDIR = mkdir -p $1
	RMDIR = rm -rf $1
	TARGET = build/autolang
	RUN_CMD = ./$(TARGET)
endif

# 3. CHỈ quét source code của Autolang (KHÔNG quét mã nguồn của CPR nữa)
# Sau này bạn có thêm file vào src/ thì mở comment dòng dưới:
# SRC = $(call rwildcard,src,*.cpp) tests/main.cpp
SRC = tests/main.cpp

OBJ = $(patsubst %.cpp,build/%.o,$(SRC))
DEPS = $(OBJ:.o=.d)

.PHONY: all run clean

# Target mặc định khi gõ `mingw32-make`
all: $(TARGET)

# 4. Bước Linker: Ghép các file .o thành file thực thi kèm theo thư viện tĩnh
$(TARGET): $(OBJ)
	@echo [LINKING ALL TOGETHER] $@
	$(LAUNCHER) $(CXX) $(OBJ) $(LIBS) -o $(TARGET)

# Target chạy test ngay sau khi build
run: all
	@echo [RUNNING] $(TARGET)
	@$(RUN_CMD)

# 5. Bước Compile: Dịch từng file .cpp sang .o (Tự động tạo thư mục con chứa .o)
build/%.o: %.cpp
	@$(call MKDIR,$(dir $@))
	@echo [COMPILING] $<
	$(LAUNCHER) $(CXX) $(CXXFLAGS) -c $< -o $@

# Nhúng file .d để Make biết khi nào bạn sửa file .h / .hpp thì phải build lại .cpp
-include $(DEPS)

# Dọn dẹp rác
clean:
	@echo [CLEANING] build directory...
	@$(call RMDIR,build)