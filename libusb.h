#ifndef LIBUSB_H
#define LIBUSB_H

#include <stdint.h>

// libusb context
typedef struct libusb_context libusb_context;
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
__declspec(dllimport) int libusb_init(libusb_context **ctx);
__declspec(dllimport) void libusb_exit(libusb_context *ctx);
__declspec(dllimport) ssize_t libusb_get_device_list(libusb_context *ctx, libusb_device ***list);
__declspec(dllimport) void libusb_free_device_list(libusb_device **list, int unref_devices);
__declspec(dllimport) int libusb_get_device_descriptor(libusb_device *dev, struct libusb_device_descriptor *desc);
__declspec(dllimport) int libusb_open(libusb_device *dev, libusb_device_handle **handle);
__declspec(dllimport) void libusb_close(libusb_device_handle *dev_handle);
__declspec(dllimport) int libusb_bulk_transfer(libusb_device_handle *dev_handle,
                        unsigned char endpoint,
                        unsigned char *data,
                        int length,
                        int *actual_length,
                        unsigned int timeout);

#endif // LIBUSB_H
