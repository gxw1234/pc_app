@echo off
echo Building test.exe...

REM Copy required DLLs
copy ..\USB_G_50.dll .
copy ..\libusb-1.0.dll .

REM Compile test program
gcc -o test.exe test.c -L.. -lUSB_G_50
if %errorlevel% neq 0 (
    echo Failed to build test.exe
    exit /b %errorlevel%
)

echo Build completed!
echo Generated: test.exe
