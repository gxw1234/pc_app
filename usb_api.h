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

#include "usb_control.h"  // 使用usb_control.h中的device_info_t定义

// 导出的API函数
USB_API int USB_ScanDevice(device_info_t* devices, int max_devices);
USB_API int USB_OpenDevice(const char* target_serial);  // 改回 char*
USB_API int USB_CloseDevice(void);
USB_API int USB_ReadData(unsigned char* data, int length, int* transferred);

#ifdef __cplusplus
}
#endif

#endif // USB_API_H
