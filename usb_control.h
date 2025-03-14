#ifndef USB_CONTROL_H
#define USB_CONTROL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <windows.h>

// DLL path
#define LIBUSB_DLL_PATH "libs/windows/x86_64/libusb-1.0.dll"

// Device IDs
#define VENDOR_ID  0x1733
#define PRODUCT_ID 0xAABB

// Max string length
#define MAX_STR_LENGTH 256
#define MAX_DEVICES 16  // 最大设备数量

// USB types
typedef void* libusb_context;
typedef struct libusb_device libusb_device;
typedef struct libusb_device_handle libusb_device_handle;

// Device descriptor
struct libusb_device_descriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
};

// Device info structure
typedef struct {
    char serial[MAX_STR_LENGTH];
    char manufacturer[MAX_STR_LENGTH];
    char product[MAX_STR_LENGTH];
} device_info_t;

// Function types
typedef int (*libusb_init_t)(libusb_context**);
typedef void (*libusb_exit_t)(libusb_context*);
typedef ssize_t (*libusb_get_device_list_t)(libusb_context*, libusb_device***);
typedef void (*libusb_free_device_list_t)(libusb_device**, int);
typedef int (*libusb_get_device_descriptor_t)(libusb_device*, struct libusb_device_descriptor*);
typedef int (*libusb_open_t)(libusb_device*, libusb_device_handle**);
typedef void (*libusb_close_t)(libusb_device_handle*);
typedef int (*libusb_set_configuration_t)(libusb_device_handle*, int);
typedef int (*libusb_claim_interface_t)(libusb_device_handle*, int);
typedef int (*libusb_release_interface_t)(libusb_device_handle*, int);
typedef int (*libusb_bulk_transfer_t)(libusb_device_handle*, unsigned char, unsigned char*, int, int*, unsigned int);
typedef int (*libusb_get_string_descriptor_ascii_t)(libusb_device_handle*, uint8_t, unsigned char*, int);

// Error codes
#define LIBUSB_SUCCESS             0
#define LIBUSB_ERROR_IO          -1
#define LIBUSB_ERROR_INVALID_PARAM -2
#define LIBUSB_ERROR_ACCESS      -3
#define LIBUSB_ERROR_NO_DEVICE   -4
#define LIBUSB_ERROR_NOT_FOUND   -5
#define LIBUSB_ERROR_BUSY        -6
#define LIBUSB_ERROR_TIMEOUT     -7
#define LIBUSB_ERROR_OVERFLOW    -8
#define LIBUSB_ERROR_PIPE        -9
#define LIBUSB_ERROR_INTERRUPTED -10
#define LIBUSB_ERROR_NO_MEM      -11
#define LIBUSB_ERROR_NOT_SUPPORTED -12
#define LIBUSB_ERROR_OTHER       -99

// Function declarations
extern const char* libusb_error_name(int error_code);
int usb_control_init(void);
void usb_control_exit(void);
int USB_ScanDevice(device_info_t* devices, int max_devices);  // 返回找到的设备数量
int USB_OpenDevice(const char* target_serial);  // 如果target_serial为NULL，打开第一个设备
int USB_CloseDevice(void);
int usb_control_read(unsigned char* data, int length, int* transferred);

#endif // USB_CONTROL_H
