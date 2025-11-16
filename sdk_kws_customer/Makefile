# 编译器设置
# CC = arm-linux-gnueabihf-gcc
CC = gcc
CFLAGS = -Wall -O2 -g -fPIC

# 链接器设置
LDFLAGS = -lssl -lcrypto -lgpiod -lm -pthread -Wl,-T,raspinew/arm64-64.lds \
sdk/src/lib/dtof_ft.a

# 目录设置
BUILD_DIR = Build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin
TARGET = $(BIN_DIR)/my_program  # 定义最终目标文件（可根据需要修改名称）

# cJSON 设置
# CJSON_DIR = third_party/cjson
# CJSON_SRC = $(CJSON_DIR)/cJSON.c
# CJSON_HEADER = $(CJSON_DIR)/cJSON.h
# CJSON_OBJ = $(OBJ_DIR)/cJSON.o  # 简化路径，避免嵌套目录问题

# 源文件和头文件路径
SRC = $(wildcard raspinew/*.c)\
      $(wildcard sdk/**/*.c) \
	  $(wildcard sdk/**/**/*.c)
#       $(CJSON_SRC)

# SRC := $(filter-out raspinew/command_receiver.c raspinew/dtof_frame_control.c raspinew/dtof_interrupt_handler.c, $(SRC))

# INCLUDES = -I sdk -I raspinew -I . -I $(CJSON_DIR) 
INCLUDES = -I sdk -I raspinew -I . -I ./l3_a05_30hz_sdk

# 对象文件
OBJ = $(SRC:%.c=$(OBJ_DIR)/%.o)

# 默认目标
all: $(TARGET)

# 链接目标
$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)
	@echo "编译完成: $@"

# 编译源文件到对象文件
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)  # 创建必要的目录
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
	@echo "编译: $< -> $@"

# 清理目标
clean:
	rm -rf $(BUILD_DIR)
	@echo "清理完成"

# 清理对象文件（保留可执行文件）
clean-obj:
	rm -rf $(OBJ_DIR)
	@echo "清理对象文件完成"

# 安装目标（示例，可根据需要调整）
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
	@echo "安装完成: /usr/local/bin/"

# 调试目标
debug: CFLAGS += -DDEBUG
debug: clean $(TARGET)
	@echo "调试模式编译完成"

# 重建目标
rebuild: clean all
	@echo "重建完成"

.PHONY: all clean clean-obj install debug rebuild
