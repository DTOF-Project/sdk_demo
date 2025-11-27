@echo off
setlocal

:: 设置目标输出路径
set "OUTPUT_PATH=%~dp0"

echo %OUTPUT_PATH%

:: 获取当前目录
set "CURRENT_DIR=%~dp0"

:: 切换到工程目录
cd /d %CURRENT_DIR%

:: 获取 Git 提交哈希（短格式）
for /f %%i in ('git rev-parse  HEAD') do set GIT_HASH=%%i

:: 输出到 .h 文件（重定向，注意不要用 echo 本身输出）
(
echo // This file is auto-generated. Do not edit manually.
echo #ifndef GIT_HASH_H
echo #define GIT_HASH_H
echo.
echo #define GIT_COMMIT_HASH "%GIT_HASH%"
echo.
echo #endif // GIT_HASH_H
) > "%OUTPUT_PATH%\git_hash.h"

endlocal
