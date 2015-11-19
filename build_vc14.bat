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
set ARCH_UPPER=X86
set INCLUDE=
set LIB=
set LIBPATH=
set PATH=%BUILD_PATH_BASE%
call "%VC14_X86_SETUP%"
goto :build_start

:build_x64
set ARCH=x64
set ARCH_UPPER=X64
set INCLUDE=
set LIB=
set LIBPATH=
set PATH=%BUILD_PATH_BASE%
call "%VC14_X64_SETUP%"
goto :build_start

:build_start

:: Settings
echo Building for %ARCH%

set EXE_NAME=robot.exe
set DLL_NAME=robot.dll

set DEFINES=/DNDEBUG /DEXE_NAME="%EXE_NAME%" /DDLL_NAME="%DLL_NAME%"
set DEFINES2=/DEFINE:NDEBUG /DEFINE:EXE_NAME="%EXE_NAME%" /DEFINE:DLL_NAME="%DLL_NAME%"
set OPTS=/EHsc /O2 /W3 %DEFINES%
set RES_COMPILER=windres
set COMPILER=g++

set TEMP_DIR=%SCRIPT_DIR%tmp\
set SRC_DIR=%SCRIPT_DIR%src\
set OUTPUT_PATH=%SCRIPT_DIR%builds\vc14\%ARCH%\

echo Building resources
mkdir "%TEMP_DIR%" > NUL 2> NUL
"%VC_RC%" /NOLOGO /fo ^
	"%TEMP_DIR%Resources.res" ^
	"%SCRIPT_DIR%src\Resources.rc" ^
	|| goto :build_error
cvtres /NOLOGO /MACHINE:%ARCH_UPPER% %DEFINES2% ^
	/OUT:"%TEMP_DIR%Resources.coff" ^
	"%TEMP_DIR%Resources.res" ^
	|| goto :build_error

"%VC_RC%" /NOLOGO /fo ^
	"%TEMP_DIR%Resources_dll.res" ^
	"%SCRIPT_DIR%src\Resources_dll.rc" ^
	|| goto :build_error
cvtres /NOLOGO /MACHINE:%ARCH_UPPER% %DEFINES2% ^
	/OUT:"%TEMP_DIR%Resources_dll.coff" ^
	"%TEMP_DIR%Resources_dll.res" ^
	|| goto :build_error

echo Building dll
cl /nologo %OPTS% /c ^
	/Fo:"%TEMP_DIR%robot_hook.obj" ^
	"%SRC_DIR%robot_hook.cpp" ^
	|| goto :build_error

link /NOLOGO /MACHINE:%ARCH_UPPER% /DLL ^
	/IMPLIB:"%TEMP_DIR%ignore.lib" ^
	/OUT:"%OUTPUT_PATH%%DLL_NAME%" ^
	"%TEMP_DIR%robot_hook.obj" ^
	"%TEMP_DIR%Resources_dll.coff" ^
	User32.lib ^
	|| goto :build_error

echo Building exe
set SOURCES=robot.cpp SubProcess.cpp Thread.cpp Windows.cpp Window.cpp SoundThread.cpp Environment.cpp
set OBJECTS=
for %%f in (%SOURCES%) do call :build_obj "%%f"

link /NOLOGO /MACHINE:%ARCH_UPPER% ^
	/IMPLIB:"%TEMP_DIR%ignore.lib" ^
	/OUT:"%OUTPUT_PATH%%EXE_NAME%" ^
	%OBJECTS% ^
	"%TEMP_DIR%Resources.coff" ^
	User32.lib Shell32.lib gdi32.lib Shlwapi.lib Ole32.lib Winmm.lib ^
	|| goto :build_error



echo Built
goto :eof

:build_error
exit /b 1
goto :eof



:build_obj

set INPUT=%~1

cl /nologo %OPTS% /c ^
	/Fo:"%TEMP_DIR%%~n1.obj" ^
	"%SRC_DIR%%~1" ^
	|| goto exit /b 1

set OBJECTS=%OBJECTS% "%TEMP_DIR%%~n1.obj"

goto :eof
