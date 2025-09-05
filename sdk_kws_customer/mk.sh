# 判断当前系统架构
# 如果后续这里的 架构不能根据这个判断 再查看一下当前 arch
ARCH=$(uname -m)

# echo "$ARCH"


if [ "$ARCH" = "aarch64" ]; then
    # 64位系统
    LDS_FILE="raspinew/arm64-64.lds"
else
    # 默认 32位系统
    LDS_FILE="raspinew/arm32-32.lds"
fi


sed -i "s#LDFLAGS = -lssl -lcrypto -lgpiod -lm -pthread -Wl,-T,.*s#LDFLAGS = -lssl -lcrypto -lgpiod -lm -pthread -Wl,-T,${LDS_FILE}#" Makefile

make clean && make  2> warning.txt

# mkdir ./Build/bin/script
# cp ./script/update.py ./Build/bin/script/update.py