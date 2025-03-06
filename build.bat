@echo off
echo Building USB_G_50.dll...

REM Compile source files
gcc -c usb_control.c -o usb_control.o -DUSB_EXPORTS -I.
if %errorlevel% neq 0 (
    echo Failed to compile usb_control.c
    exit /b %errorlevel%
)

gcc -c usb_api.c -o usb_api.o -DUSB_EXPORTS -I.
if %errorlevel% neq 0 (
    echo Failed to compile usb_api.c
    exit /b %errorlevel%
)

REM Generate DLL
gcc -shared -o USB_G_50.dll usb_control.o usb_api.o -L. -lusb-1.0
if %errorlevel% neq 0 (
    echo Failed to create DLL
    exit /b %errorlevel%
)

REM Clean intermediate files
del usb_control.o
del usb_api.o

echo Build completed!
echo Generated: USB_G_50.dll
