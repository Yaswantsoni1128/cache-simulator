@echo off
REM Build script for Windows (PowerShell or cmd)
if not exist src (echo src folder not found & exit /b 1)
if not exist include (echo include folder not found & exit /b 1)
set COMPILER=g++
%COMPILER% -std=c++17 src\*.cpp -Iinclude -O2 -o cache_simulator.exe
if %ERRORLEVEL% neq 0 (
  echo Build failed
  exit /b %ERRORLEVEL%
)
echo Built cache_simulator.exe
