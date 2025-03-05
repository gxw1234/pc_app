#include "usb_control.h"

// Global variables
static HMODULE dll = NULL;
static libusb_context* ctx = NULL;
static libusb_device_handle* dev_handle = NULL;

// Function pointers
static libusb_init_t libusb_init = NULL;
static libusb_exit_t libusb_exit = NULL;
static libusb_get_device_list_t libusb_get_device_list = NULL;
static libusb_free_device_list_t libusb_free_device_list = NULL;
static libusb_get_device_descriptor_t libusb_get_device_descriptor = NULL;
static libusb_open_t libusb_open = NULL;
static libusb_close_t libusb_close = NULL;
static libusb_set_configuration_t libusb_set_configuration = NULL;
static libusb_claim_interface_t libusb_claim_interface = NULL;
static libusb_release_interface_t libusb_release_interface = NULL;
static libusb_bulk_transfer_t libusb_bulk_transfer = NULL;
static libusb_get_string_descriptor_ascii_t libusb_get_string_descriptor_ascii = NULL;

// Helper function to get device string descriptor
static int get_string_descriptor(libusb_device_handle* handle, uint8_t desc_index, unsigned char* data, int length) {
    if (desc_index == 0) return 0;
    
    int r = libusb_get_string_descriptor_ascii(handle, desc_index, data, length);
    if (r < 0) {
        printf(" Failed: %s", libusb_error_name(r));
        return r;
    }
    return r;
}

// Static function declarations
static const char* error_name(int error_code);

const char* libusb_error_name(int error_code) {
    return error_name(error_code);
}

static const char* error_name(int error_code) {
    switch(error_code) {
        case LIBUSB_SUCCESS: return "LIBUSB_SUCCESS";
        case LIBUSB_ERROR_IO: return "LIBUSB_ERROR_IO";
        case LIBUSB_ERROR_INVALID_PARAM: return "LIBUSB_ERROR_INVALID_PARAM";
        case LIBUSB_ERROR_ACCESS: return "LIBUSB_ERROR_ACCESS";
        case LIBUSB_ERROR_NO_DEVICE: return "LIBUSB_ERROR_NO_DEVICE";
        case LIBUSB_ERROR_NOT_FOUND: return "LIBUSB_ERROR_NOT_FOUND";
        case LIBUSB_ERROR_BUSY: return "LIBUSB_ERROR_BUSY";
        case LIBUSB_ERROR_TIMEOUT: return "LIBUSB_ERROR_TIMEOUT";
        case LIBUSB_ERROR_OVERFLOW: return "LIBUSB_ERROR_OVERFLOW";
        case LIBUSB_ERROR_PIPE: return "LIBUSB_ERROR_PIPE";
        case LIBUSB_ERROR_INTERRUPTED: return "LIBUSB_ERROR_INTERRUPTED";
        case LIBUSB_ERROR_NO_MEM: return "LIBUSB_ERROR_NO_MEM";
        case LIBUSB_ERROR_NOT_SUPPORTED: return "LIBUSB_ERROR_NOT_SUPPORTED";
        case LIBUSB_ERROR_OTHER: return "LIBUSB_ERROR_OTHER";
        default: return "Unknown error";
    }
}

//初始化
int usb_control_init(void) {
    // Load DLL
    dll = LoadLibraryA(LIBUSB_DLL_PATH);
    if (!dll) {
        printf("Failed to load %s\n", LIBUSB_DLL_PATH);
        return -1;
    }
    printf("Loaded %s\n", LIBUSB_DLL_PATH);

    // Get function pointers
    libusb_init = (libusb_init_t)GetProcAddress(dll, "libusb_init");
    libusb_exit = (libusb_exit_t)GetProcAddress(dll, "libusb_exit");
    libusb_get_device_list = (libusb_get_device_list_t)GetProcAddress(dll, "libusb_get_device_list");
    libusb_free_device_list = (libusb_free_device_list_t)GetProcAddress(dll, "libusb_free_device_list");
    libusb_get_device_descriptor = (libusb_get_device_descriptor_t)GetProcAddress(dll, "libusb_get_device_descriptor");
    libusb_open = (libusb_open_t)GetProcAddress(dll, "libusb_open");
    libusb_close = (libusb_close_t)GetProcAddress(dll, "libusb_close");
    libusb_set_configuration = (libusb_set_configuration_t)GetProcAddress(dll, "libusb_set_configuration");
    libusb_claim_interface = (libusb_claim_interface_t)GetProcAddress(dll, "libusb_claim_interface");
    libusb_release_interface = (libusb_release_interface_t)GetProcAddress(dll, "libusb_release_interface");
    libusb_bulk_transfer = (libusb_bulk_transfer_t)GetProcAddress(dll, "libusb_bulk_transfer");
    libusb_get_string_descriptor_ascii = (libusb_get_string_descriptor_ascii_t)GetProcAddress(dll, "libusb_get_string_descriptor_ascii");

    if (!libusb_init || !libusb_exit || !libusb_get_device_list || !libusb_free_device_list || 
        !libusb_get_device_descriptor || !libusb_open || !libusb_close || !libusb_set_configuration ||
        !libusb_claim_interface || !libusb_release_interface || !libusb_bulk_transfer ||
        !libusb_get_string_descriptor_ascii) {
        printf("Failed to get function pointers\n");
        FreeLibrary(dll);
        return -1;
    }
    printf("Got function pointers\n");

    // Initialize libusb
    int r = libusb_init(&ctx);
    if (r < 0) {
        printf("Init Error: %s\n", libusb_error_name(r));
        FreeLibrary(dll);
        return r;
    }
    printf("libusb initialized successfully\n");

    return 0;
}



//清理和释放资源
void usb_control_exit(void) {
    if (dev_handle) {
        libusb_release_interface(dev_handle, 0);
        libusb_close(dev_handle);
        dev_handle = NULL;
    }
    
    if (ctx) {
        libusb_exit(ctx);
        ctx = NULL;
    }
    
    if (dll) {
        FreeLibrary(dll);
        dll = NULL;
    }
}


/* 获取USB vad  0x1733 pad 0xAABB 设备序列号 */
int USB_ScanDevice(device_info_t* devices, int max_devices) {
    if (devices == NULL || max_devices <= 0) {
        return LIBUSB_ERROR_INVALID_PARAM;
    }

    libusb_device** devs;
    libusb_device* dev;
    struct libusb_device_descriptor desc;
    ssize_t cnt;
    int r, i = 0;
    int found = 0;
    
    // Get device list
    cnt = libusb_get_device_list(ctx, &devs);
    if (cnt < 0) {
        printf("Get Device Error: %s\n", libusb_error_name((int)cnt));
        return (int)cnt;
    }
    printf("Found %d USB devices\n", (int)cnt);
    
    // Find our devices by VID/PID
    while ((dev = devs[i++]) != NULL && found < max_devices) {
        r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            printf("Failed to get device descriptor: %s\n", libusb_error_name(r));
            continue;
        }

        // Check if this is our target device by VID/PID
        if (desc.idVendor == VENDOR_ID && desc.idProduct == PRODUCT_ID) {
            libusb_device_handle* handle;
            r = libusb_open(dev, &handle);
            if (r < 0) {
                printf("Device %d: VID=0x%04x, PID=0x%04x (Cannot open device: %s)\n", 
                       found + 1, desc.idVendor, desc.idProduct, libusb_error_name(r));
                continue;
            }

            device_info_t* dev_info = &devices[found];
            memset(dev_info, 0, sizeof(device_info_t));

            // Get manufacturer string
            r = get_string_descriptor(handle, desc.iManufacturer, (unsigned char*)dev_info->manufacturer, sizeof(dev_info->manufacturer));
            
            // Get product string
            r = get_string_descriptor(handle, desc.iProduct, (unsigned char*)dev_info->product, sizeof(dev_info->product));
            
            // Get serial number
            r = get_string_descriptor(handle, desc.iSerialNumber, (unsigned char*)dev_info->serial, sizeof(dev_info->serial));
            
            libusb_close(handle);
            
            if (r < 0) {
                continue;
            }

            // printf("Device %d:\n", found + 1);
            // printf("  VID/PID: 0x%04x/0x%04x\n", desc.idVendor, desc.idProduct);
            // printf("  Manufacturer: %s\n", dev_info->manufacturer);
            // printf("  Product: %s\n", dev_info->product);
            // printf("  S/N: %s\n", dev_info->serial);
            // printf("\n");

            found++;
        }
    }

    // Free device list
    libusb_free_device_list(devs, 1);
    
    if (found == 0) {
        printf("No matching devices found\n");
        return LIBUSB_ERROR_NOT_FOUND;
    }
    
    printf("Found %d matching device(s)\n", found);
    return found;
}

/* 打开SN */
int USB_OpenDevice(const char* target_serial) {
    libusb_device** devs;
    libusb_device* dev;
    struct libusb_device_descriptor desc;
    ssize_t cnt;
    int r, i = 0;
    unsigned char string[MAX_STR_LENGTH];
    int found = 0;
    
    // Get device list
    cnt = libusb_get_device_list(ctx, &devs);
    if (cnt < 0) {
        printf("Get Device Error: %s\n", libusb_error_name((int)cnt));
        return (int)cnt;
    }
    
    // Find our device by VID/PID and serial
    while ((dev = devs[i++]) != NULL) {
        r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            continue;
        }

        // Check if this is our target device by VID/PID
        if (desc.idVendor == VENDOR_ID && desc.idProduct == PRODUCT_ID) {
            libusb_device_handle* handle;
            r = libusb_open(dev, &handle);
            if (r < 0) {
                continue;
            }

            // Get serial number
            memset(string, 0, sizeof(string));
            r = get_string_descriptor(handle, desc.iSerialNumber, string, sizeof(string));
            if (r < 0) {
                libusb_close(handle);
                continue;
            }

            // Check if this is our target device
            if (target_serial == NULL || strcmp((char*)string, target_serial) == 0) {
                printf("Opening device with S/N: %s\n", string);

                // Set configuration
                r = libusb_set_configuration(handle, 1);
                if (r < 0) {
                    printf("Failed to set configuration: %s\n", libusb_error_name(r));
                    libusb_close(handle);
                    libusb_free_device_list(devs, 1);
                    return r;
                }

                // Claim interface
                r = libusb_claim_interface(handle, 0);
                if (r < 0) {
                    printf("Failed to claim interface: %s\n", libusb_error_name(r));
                    libusb_close(handle);
                    libusb_free_device_list(devs, 1);
                    return r;
                }

                // Save handle and send open command
                dev_handle = handle;
                printf("Sending open command...\n");
                unsigned char data = 0x01;
                int transferred;
                r = libusb_bulk_transfer(dev_handle, 0x01, &data, 1, &transferred, 0);
                if (r < 0) {
                    printf("Failed to send open command: %s\n", libusb_error_name(r));
                    USB_CloseDevice();
                    libusb_free_device_list(devs, 1);
                    return r;
                }
                printf("Device opened successfully (transferred %d bytes)\n", transferred);

                // Free device list and return success
                libusb_free_device_list(devs, 1);
                return 0;
            }

            // Not our target device, close it
            libusb_close(handle);
            found++;
        }
    }

    // Free device list
    libusb_free_device_list(devs, 1);
    printf("Target device not found\n");
    return LIBUSB_ERROR_NOT_FOUND;
}

/* 关闭设备 */
int usb_control_open(const char* target_serial) {
    libusb_device** devs;
    libusb_device* dev;
    struct libusb_device_descriptor desc;
    ssize_t cnt;
    int r, i = 0;
    unsigned char string[MAX_STR_LENGTH];
    
    // Get device list
    cnt = libusb_get_device_list(ctx, &devs);
    if (cnt < 0) {
        printf("Get Device Error: %s\n", libusb_error_name((int)cnt));
        return (int)cnt;
    }
    printf("Found %d devices\n", (int)cnt);
    
    // Find our device
    while ((dev = devs[i++]) != NULL) {
        r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            printf("Failed to get device descriptor: %s\n", libusb_error_name(r));
            continue;
        }

        // Check if this is our target device by VID/PID
        if (desc.idVendor == VENDOR_ID && desc.idProduct == PRODUCT_ID) {
            libusb_device_handle* handle;
            r = libusb_open(dev, &handle);
            if (r < 0) {
                printf(" (Cannot open device: %s)\n", libusb_error_name(r));
                continue;
            }

            // Set configuration and claim interface
            r = libusb_set_configuration(handle, 1);
            if (r < 0) {
                printf("\n  Failed to set configuration: %s", libusb_error_name(r));
                libusb_close(handle);
                continue;
            }

            r = libusb_claim_interface(handle, 0);
            if (r < 0) {
                printf("\n  Failed to claim interface: %s", libusb_error_name(r));
                libusb_close(handle);
                continue;
            }

            // Get serial number
            memset(string, 0, sizeof(string));
            r = get_string_descriptor(handle, desc.iSerialNumber, string, sizeof(string));
            if (r < 0) {
                libusb_release_interface(handle, 0);
                libusb_close(handle);
                continue;
            }

            printf(", S/N: %s", string);

            // Check if this is our target device by serial number
            if (target_serial == NULL || strcmp((char*)string, target_serial) == 0) {
                printf("\nFound target device!\n");
                dev_handle = handle;  // Save the handle
                break;
            }

            // Not our target device, close it
            libusb_release_interface(handle, 0);
            libusb_close(handle);
        }
        printf("\n");
    }

    // Free device list
    libusb_free_device_list(devs, 1);

    if (dev_handle == NULL) {
        printf("Target device not found\n");
        return LIBUSB_ERROR_NOT_FOUND;
    }

    // Send open command
    printf("Sending open command...\n");
    unsigned char data = 0x01;
    int transferred;
    r = libusb_bulk_transfer(dev_handle, 0x01, &data, 1, &transferred, 0);
    if (r < 0) {
        printf("Failed to send open command: %s\n", libusb_error_name(r));
        USB_CloseDevice();
        return r;
    }
    printf("Device opened successfully (transferred %d bytes)\n", transferred);

    return 0;
}

/* 从设备读取数据 */
int usb_control_read(unsigned char* data, int length, int* transferred) {
    if (!dev_handle) {
        return LIBUSB_ERROR_NO_DEVICE;
    }

    return libusb_bulk_transfer(dev_handle, 0x81, data, length, transferred, 1000);  // 1秒超时
}

/* 关闭设备 */
int USB_CloseDevice(void) {
    if (!dev_handle) {
        return LIBUSB_ERROR_NOT_FOUND;
    }

    // Send close command
    unsigned char data = 0;
    int transferred;
    printf("Sending close command...\n");
    
    int r = libusb_bulk_transfer(dev_handle, 0x01, &data, 1, &transferred, 0);
    if (r == 0) {
        printf("Device closed successfully (transferred %d bytes)\n", transferred);
    } else {
        printf("Error in bulk transfer: %s\n", libusb_error_name(r));
    }

    libusb_release_interface(dev_handle, 0);
    libusb_close(dev_handle);
    dev_handle = NULL;

    return r;
}
