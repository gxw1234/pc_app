#define USB_API_EXPORTS
#include "usb_api.h"
#include "usb_control.h"
#include <string.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            // DLL被加载时初始化USB控制
            return (usb_control_init() >= 0);
        case DLL_PROCESS_DETACH:
            // DLL被卸载时清理
            usb_control_exit();
            break;
    }
    return TRUE;
}

__declspec(dllexport) int USB_ScanDevice(device_info_t* devices, int max_devices) {
    return usb_control_scan_device(devices, max_devices);
}

__declspec(dllexport) int USB_OpenDevice(const char* target_serial) {
    return usb_control_open_device(target_serial);
}

__declspec(dllexport) int USB_CloseDevice(const char* target_serial) {
    return usb_control_close_device(target_serial);
}

__declspec(dllexport) int USB_ReadData(const char* target_serial, unsigned char* data, int length) {
    int transferred = 0;
    int result = usb_control_read(target_serial, data, length, &transferred);
    return (result >= 0) ? transferred : result;  // 如果成功返回读取的字节数，失败返回错误码
}
