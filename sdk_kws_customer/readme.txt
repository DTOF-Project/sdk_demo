1. raspi 文件是之前4b的 能跑通 串口和 iic 的code 
2. third_party中是json 的 相关依赖
3. 同目录的 Makefile 作为参考，需要将 所有的 .c 文件都编译即可
4. mk.sh 简单的 一个编译脚本 在terminal中 运行 ./mk.sh 即可


TODO
1. 需要将raspi中的 依赖能重新定向 然后跑通，其中可能需要依赖到一些 sdk中的 内容
2. 迁移命令：
    - s:



Record
1. 当前的 sdk 的 commit 为 kws - 71aaa4f
2. 编译后可以从串口发送命令，命令末端的"\n"和"\r"会被忽略
    - echo: 向串口发送"echo"字符串
    - readreg: 通过i2c读取从机的寄存器
    - quit: 结束循环
3. 
