# ============================================================
#  LogIO Makefile  (通用版 – 支持 Linux / Termux / Android)
# ============================================================

# 编译器与工具
CC      := gcc
AR      := ar
CFLAGS  := -Wall -Wextra -fPIC -O2 -pthread
LDFLAGS := -shared -pthread
ARFLAGS := rcs

# 安全加固（可选，若编译器支持）
CFLAGS  += -fstack-protector-strong -fstack-clash-protection -fno-strict-aliasing
CFLAGS  += -Wstack-usage=1024 -fno-optimize-sibling-calls

# 目录定义
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin
INC_DIR := include

# 库名称
LIB_NAME   := logio
TARGET_SO  := $(BIN_DIR)/lib$(LIB_NAME).so
TARGET_A   := $(BIN_DIR)/lib$(LIB_NAME).a

# 源文件与对象文件
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

# 测试程序
TEST_SRC    := test_logio.c
TEST_TARGET := $(BIN_DIR)/test_logio

# 安装路径（可通过命令行覆盖，例如：make install PREFIX=/usr）
PREFIX       ?= /usr/local
INSTALL_LIB  := $(PREFIX)/lib
INSTALL_INC  := $(PREFIX)/include

# 在 Termux 环境可自动适配，也可直接 make PREFIX=/data/data/com.termux/files/usr
# 默认目标：生成共享库与静态库
all: create_dirs $(TARGET_SO) $(TARGET_A)

# 创建输出目录
create_dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# 生成共享库
$(TARGET_SO): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "✅ 共享库已生成: $@"

# 生成静态库
$(TARGET_A): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^
	@echo "✅ 静态库已生成: $@"

# 编译对象文件
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# 编译并运行测试
test: $(TARGET_SO) $(TEST_TARGET)

$(TEST_TARGET): $(TEST_SRC) $(TARGET_SO)
	$(CC) $(CFLAGS) -I$(INC_DIR) -o $@ $(TEST_SRC) -L$(BIN_DIR) -l$(LIB_NAME) -Wl,-rpath,$(BIN_DIR)
	@echo "✅ 测试程序已生成: $@"

run-test: test
	@echo "🧪 运行测试程序..."
	@mkdir -p logs
	@$(TEST_TARGET)

# 安装库和头文件
install: $(TARGET_SO) $(TARGET_A)
	@echo "正在安装共享库与静态库到 $(INSTALL_LIB)..."
	@mkdir -p $(INSTALL_LIB)
	@cp $(TARGET_SO) $(INSTALL_LIB)/
	@cp $(TARGET_A) $(INSTALL_LIB)/
	@echo "正在安装头文件到 $(INSTALL_INC)..."
	@mkdir -p $(INSTALL_INC)
	@cp $(INC_DIR)/logio.h $(INSTALL_INC)/
	@echo "✅ 安装完成"

# 卸载
uninstall:
	@echo "正在卸载..."
	@rm -f $(INSTALL_LIB)/lib$(LIB_NAME).so
	@rm -f $(INSTALL_LIB)/lib$(LIB_NAME).a
	@rm -f $(INSTALL_INC)/logio.h
	@echo "✅ 卸载完成"

# 清理编译产物
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "🗑️ 已清理中间文件和结果目录"

.PHONY: all create_dirs clean install uninstall test run-test