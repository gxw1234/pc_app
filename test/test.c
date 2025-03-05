#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "../usb_api.h"

int main() {
    int result;
    device_info_t devices[16];
    int num_devices;
    unsigned char data[64];
    int bytes_transferred;

    // 扫描设备
    num_devices = USB_ScanDevice(devices, 16);
    if (num_devices <= 0) {
        printf("No devices found (error code: %d)\n", num_devices);
        return -1;
    }
    printf("Found %d device(s)\n", num_devices);

    // 打印设备信息
    for (int i = 0; i < num_devices; i++) {
        printf("\nDevice %d:\n", i + 1);
        printf("  Serial: %s\n", devices[i].serial);
        printf("  Manufacturer: %s\n", devices[i].manufacturer);
        printf("  Product: %s\n", devices[i].product);
    }

    // 打开第一个设备
    result = USB_OpenDevice(devices[0].serial);
    if (result < 0) {
        printf("Failed to open device (error code: %d)\n", result);
        return -1;
    }

    printf("\nDevice opened successfully\n");

    // 读取数据
    for (int i = 0; i < 10; i++) {
        result = USB_ReadData(data, sizeof(data), &bytes_transferred);
        if (result < 0) {
            printf("Failed to read data (error code: %d)\n", result);
            break;
        }

        if (bytes_transferred > 0) {
            printf("\nReceived %d bytes:\n", bytes_transferred);
            printf("Data: ");
            for (int j = 0; j < bytes_transferred; j++) {
                printf("%02X ", data[j]);
            }
            printf("\n");
        }

        Sleep(100);  // 短暂延时，避免 CPU 占用过高
    }

    // 关闭设备
    result = USB_CloseDevice();
    if (result < 0) {
        printf("Failed to close device (error code: %d)\n", result);
        return -1;
    }

    printf("Device closed successfully\n");
    return 0;
}
