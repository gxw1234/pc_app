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

USB_API int USB_ScanDevice(device_info_t* devices, int max_devices) {
    return usb_control_scan_device(devices, max_devices);
}

USB_API int USB_OpenDevice(const char* target_serial) {
    if (target_serial == NULL) {
        return -1;
    }
    return usb_control_open_device(target_serial);
}

USB_API int USB_CloseDevice(void) {
    return usb_control_close_device();
}

USB_API int USB_ReadData(unsigned char* data, int length, int* transferred) {
    return usb_control_read(data, length, transferred);
}
