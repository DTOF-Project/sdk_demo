@echo off
setlocal

set "OUTPUT_PATH=%~dp0..\application\inc"

echo %OUTPUT_PATH%

set "CURRENT_DIR=%~dp0"

cd /d %CURRENT_DIR%

for /f %%i in ('git rev-parse  HEAD') do set GIT_HASH=%%i

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
