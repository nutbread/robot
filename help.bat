@echo off
cls
set SCRIPT_DIR=%~dp0
"%SCRIPT_DIR%robot.exe" --help
pause > NUL 2> NUL