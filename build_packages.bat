@echo off

set SCRIPT_DIR=%~dp0

call :build_package robot-gcc-x86 "%SCRIPT_DIR%builds\gcc\x86\"
call :build_package robot-gcc-x64 "%SCRIPT_DIR%builds\gcc\x64\"
call :build_package robot-vc14-x86 "%SCRIPT_DIR%builds\vc14\x86\"
call :build_package robot-vc14-x64 "%SCRIPT_DIR%builds\vc14\x64\"

goto :eof



:build_package

set EXE_PATH=%~2
set PACKAGE_NAME=%~1
set PACKAGE_FULL=%SCRIPT_DIR%packages\%PACKAGE_NAME%.zip
set OPTIONS=-mx=9 -mmt=on -mtc=off -mm=BZip2 -mpass=10
set SEVENZIP_EXE=%SCRIPT_DIR%other\7z

echo Packaging %PACKAGE_NAME%...
del "%PACKAGE_FULL%" > NUL 2> NUL

pushd "%EXE_PATH%"
"%SEVENZIP_EXE%" a -tzip "%PACKAGE_FULL%" %OPTIONS% robot.exe robot.dll > NUL
popd
pushd "%SCRIPT_DIR%builds"
"%SEVENZIP_EXE%" a -tzip "%PACKAGE_FULL%" %OPTIONS% voice.vbs > NUL
popd
pushd "%SCRIPT_DIR%other"
"%SEVENZIP_EXE%" a -tzip "%PACKAGE_FULL%" %OPTIONS% sox.exe zlib1.dll readme.txt > NUL
popd
pushd "%SCRIPT_DIR%docs"
"%SEVENZIP_EXE%" a -tzip "%PACKAGE_FULL%" %OPTIONS% options.txt > NUL
popd
pushd "%SCRIPT_DIR%"
"%SEVENZIP_EXE%" a -tzip "%PACKAGE_FULL%" %OPTIONS% help.bat > NUL
popd

goto :eof


