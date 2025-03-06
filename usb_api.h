#ifndef USB_API_H
#define USB_API_H

#ifdef _WIN32
    #ifdef USB_EXPORTS
        #define USB_API __declspec(dllexport)
    #else
        #define USB_API __declspec(dllimport)
    #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "usb_control.h"

// 导出的API函数

USB_API int USB_ScanDevice(device_info_t* devices, int max_devices);
USB_API int USB_OpenDevice(const char* target_serial);  
USB_API int USB_CloseDevice(const char* target_serial);
USB_API int USB_ReadData(const char* target_serial, unsigned char* data, int length);

#ifdef __cplusplus
}
#endif

#endif // USB_API_H
