@echo off
color



:: Setup
set SCRIPT_DIR=%~dp0

call settings.bat

if a"%BUILD_PATH_BASE%" NEQ a"" goto :build_start

echo Setting up environment
set BUILD_PATH_BASE=%PATH%



:: Build
:build_start

call :build x86 || goto :fail
echo.
call :build x64 || goto :fail

:okay
echo.
echo Builds successful
goto :eof

:fail
echo.
echo Build failed
color c
exit /b 1
goto :eof




:: Build target
:build
set ARCH=%~1
if a"%ARCH%"==a"x64" goto :build_x64

:build x86
set ARCH=x86
set MINGW=%MINGW_X86_BIN_PATH%
goto :build_start

:build_x64
set ARCH=x64
set MINGW=%MINGW_X64_BIN_PATH%
goto :build_start

:build_start

:: Settings
echo Building for %ARCH%
set PATH=%MINGW%;%BUILD_PATH_BASE%

set EXE_NAME=robot.exe
set DLL_NAME=robot.dll

set DEFINES=-DNDEBUG -DEXE_NAME="%EXE_NAME%" -DDLL_NAME="%DLL_NAME%"
set OPTS=-O3 -Wall -std=gnu++11 -static %DEFINES%
set RES_COMPILER=windres
set COMPILER=g++

set TEMP_DIR=%SCRIPT_DIR%tmp\
set SRC_DIR=%SCRIPT_DIR%src\
set OUTPUT_PATH=%SCRIPT_DIR%builds\gcc\%ARCH%\

echo Building resources
mkdir "%TEMP_DIR%" > NUL 2> NUL
"%RES_COMPILER%" -J rc -O coff %DEFINES% -o ^
	"%TEMP_DIR%Resources.coff" ^
	"%SRC_DIR%Resources.rc" ^
	|| goto :build_error
"%RES_COMPILER%" -J rc -O coff %DEFINES% -o ^
	"%TEMP_DIR%Resources_dll.coff" ^
	"%SRC_DIR%Resources_dll.rc" ^
	|| goto :build_error

echo Building dll
"%COMPILER%" %OPTS% -shared -o "%OUTPUT_PATH%%DLL_NAME%" ^
	"%SRC_DIR%robot_hook.cpp" ^
	"%TEMP_DIR%Resources_dll.coff" ^
	-lUser32 ^
	|| goto :build_error

echo Building exe
"%COMPILER%" %OPTS% -o "%OUTPUT_PATH%%EXE_NAME%" ^
	"%SRC_DIR%robot.cpp" ^
	"%SRC_DIR%SubProcess.cpp" ^
	"%SRC_DIR%Thread.cpp" ^
	"%SRC_DIR%Windows.cpp" ^
	"%SRC_DIR%Window.cpp" ^
	"%SRC_DIR%SoundThread.cpp" ^
	"%SRC_DIR%Environment.cpp" ^
	"%TEMP_DIR%Resources.coff" ^
	-lUser32 -lShell32 -lgdi32 -lShlwapi -lOle32 -lWinmm ^
	|| goto :build_error


echo Built
goto :eof

:build_error
exit /b 1
goto :eof

