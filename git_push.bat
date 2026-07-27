@echo off
chcp 65001 >nul
cd /d "%~dp0"

echo ==============================
echo  Git Quick Save
echo ==============================

:: 获取当前分支
for /f %%i in ('git branch --show-current 2^>nul') do set "branch=%%i"

:: 获取时间戳
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value 2^>nul') do set "dt=%%I"
set "tag=%dt:~0,4%%dt:~4,2%%dt:~6,2%_%dt:~8,2%%dt:~10,2%%dt:~12,2%"

echo 当前分支: %branch%
echo 提交信息: auto save %tag%
echo.

:: 生成临时脚本
set "script=%TEMP%\git_quick_save.cmd"
>"%script%" (
    echo cd /d "%~dp0"
    echo echo.
    echo echo [1/4] git status
    echo git status
    echo echo.
    echo echo [2/4] git add .
    echo git add .
    echo echo.
    echo echo [3/4] git commit
    echo git commit -m "auto save %tag%"
    echo echo.
    echo echo [4/4] git push
    echo git push
    echo echo.
    echo echo ==============================
    echo echo  完成！
    echo echo ==============================
    echo pause
)

:: 在 git-cmd 新窗口中执行
start "Git Quick Save" git-cmd "%script%"

echo 已打开 Git 窗口，正在执行...
timeout /t 2 >nul
pause