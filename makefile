# 编译器设置
CC      := gcc
CFLAGS  := -Wall -Wextra -fPIC -O2
LDFLAGS := -shared

# 目录定义
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin
LIB_NAME := logio
TARGET  := $(BIN_DIR)/lib$(LIB_NAME).so

# 源文件和中间文件
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# 默认目标：创建目录并编译
all: create_dirs $(TARGET)

# 创建必要的目录
create_dirs:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# 生成共享库
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^
	@echo "✅  共享库已生成: $@"

# 编译中间文件（.o 文件）
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理所有生成的文件
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "🗑️   已清理中间文件和结果目录"

.PHONY: all create_dirs clean
