1. raspi 文件是之前4b的 能跑通 串口和 iic 的code 
2. third_party中是json 的 相关依赖
3. 同目录的 Makefile 作为参考，需要将 所有的 .c 文件都编译即可
4. mk.sh 简单的 一个编译脚本 在terminal中 运行 ./mk.sh 即可


TODO
1. 需要将raspi中的 依赖能重新定向 然后跑通，其中可能需要依赖到一些 sdk中的 内容（已完成）
2. 


Record
1. 当前的 sdk 的 commit 为 kws - 71aaa4f
2. 编译后可以从串口发送命令，命令末端的"\n"和"\r"会被忽略
    - "echo": 复读串口发送的echo
    - "s": 启动并开始测距（串口无输出）
    - "d": 启动并开始测距（DEBUG模式，输出每一帧的数据，不会自动停止）
    - "e": 启动并开始测距（DEBUG模式 + 启动frame_cnt, 前50帧跳过， 到达200帧自动停止）
    - "t": 停止测距
    - "c,<value:int>", 设置b偏移为 <value>
    - "rb,<reg_addr:int>,<reg_num:int>", 批量读地址为 <reg_addr>的寄存器中，长度为 <reg_num>的值
    - "w,<reg_addr:int>,<reg_value:int>" 向 <reg_addr>寄存器写 <reg_value>
    - "p": 输出chip uuid, distance offset, xtalk data
    - "cal": 输出 distance_offset
    - "clear": 清除flash（已弃用，仅在stm32平台上有用）
    - "x": 写入串扰数据并原样输出ram
    - "v": 输出版本信息
    - "regtest": 测试寄存器读写api
    - "filetest": 测试文件读写api（存取 distance offset 和 xtalk data ）
    - "u,<version_name>": 更新当前程序，（退出c程序，调用python脚本）
    - "q": 退出命令循环
3. 注意：必须在执行c程序的目录下放置路径为 "script/update.py" 的py脚本文件，否则"u,<version_name>"命令将报错
4. 修改"script/update.py"中的 HOST 和 SAVE_PATH 变量来修改下载url和保存路径，可以使用{version_name}字符串填入"u,<version_name>"命令的version_name参数
