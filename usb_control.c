#include "usb_control.h"

// Global variables
static HMODULE libusb_dll = NULL;
static libusb_context* ctx = NULL;
static device_instance_t device_instances[MAX_DEVICES];

// Function pointers
static libusb_init_t fn_init;
static libusb_exit_t fn_exit;
static libusb_get_device_list_t fn_get_device_list;
static libusb_free_device_list_t fn_free_device_list;
static libusb_get_device_descriptor_t fn_get_device_descriptor;
static libusb_open_t fn_open;
static libusb_close_t fn_close;
static libusb_set_configuration_t fn_set_configuration;
static libusb_claim_interface_t fn_claim_interface;
static libusb_release_interface_t fn_release_interface;
static libusb_bulk_transfer_t fn_bulk_transfer;
static libusb_get_string_descriptor_ascii_t fn_get_string_descriptor_ascii;

// Helper function to get device string descriptor
static int get_string_descriptor(libusb_device_handle* handle, uint8_t desc_index, char* data, int length) {
    if (desc_index == 0) {
        data[0] = '\0';  // 确保空描述符返回空字符串
        return 0;
    }
    int r = fn_get_string_descriptor_ascii(handle, desc_index, (unsigned char*)data, length);
    if (r < 0) {
        printf(" Failed: error code %d", r);
        data[0] = '\0';  // 错误时返回空字符串
        return r;
    }
    data[r] = '\0';  // 确保字符串正确终止
    return r;
}

// Initialize device instances
static void init_device_instances() {
    for (int i = 0; i < MAX_DEVICES; i++) {
        device_instances[i].handle = NULL;
        device_instances[i].serial[0] = '\0';
        device_instances[i].in_use = 0;
    }
}

// Find device instance by serial number
static device_instance_t* find_device_instance(const char* target_serial) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (device_instances[i].in_use && strcmp(device_instances[i].serial, target_serial) == 0) {
            return &device_instances[i];
        }
    }
    return NULL;
}

// Get free device instance slot
static device_instance_t* get_free_instance() {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (!device_instances[i].in_use) {
            return &device_instances[i];
        }
    }
    return NULL;
}

int usb_control_init(void) {
    // Load DLL
    libusb_dll = LoadLibraryA(LIBUSB_DLL_PATH);
    if (!libusb_dll) {
        printf("Failed to load %s\n", LIBUSB_DLL_PATH);
        return -1;
    }

    // Get function pointers
    fn_init = (libusb_init_t)GetProcAddress(libusb_dll, "libusb_init");
    fn_exit = (libusb_exit_t)GetProcAddress(libusb_dll, "libusb_exit");
    fn_get_device_list = (libusb_get_device_list_t)GetProcAddress(libusb_dll, "libusb_get_device_list");
    fn_free_device_list = (libusb_free_device_list_t)GetProcAddress(libusb_dll, "libusb_free_device_list");
    fn_get_device_descriptor = (libusb_get_device_descriptor_t)GetProcAddress(libusb_dll, "libusb_get_device_descriptor");
    fn_open = (libusb_open_t)GetProcAddress(libusb_dll, "libusb_open");
    fn_close = (libusb_close_t)GetProcAddress(libusb_dll, "libusb_close");
    fn_set_configuration = (libusb_set_configuration_t)GetProcAddress(libusb_dll, "libusb_set_configuration");
    fn_claim_interface = (libusb_claim_interface_t)GetProcAddress(libusb_dll, "libusb_claim_interface");
    fn_release_interface = (libusb_release_interface_t)GetProcAddress(libusb_dll, "libusb_release_interface");
    fn_bulk_transfer = (libusb_bulk_transfer_t)GetProcAddress(libusb_dll, "libusb_bulk_transfer");
    fn_get_string_descriptor_ascii = (libusb_get_string_descriptor_ascii_t)GetProcAddress(libusb_dll, "libusb_get_string_descriptor_ascii");

    // Initialize libusb
    int r = fn_init(&ctx);
    if (r < 0) {
        printf("Failed to initialize libusb: error code %d\n", r);
        FreeLibrary(libusb_dll);
        return r;
    }

    // Initialize device instances
    init_device_instances();
    return 0;
}

void usb_control_exit(void) {
    // Close all open devices
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (device_instances[i].in_use) {
            fn_release_interface(device_instances[i].handle, 0);
            fn_close(device_instances[i].handle);
            device_instances[i].in_use = 0;
        }
    }

    // Clean up libusb
    if (ctx) {
        fn_exit(ctx);
        ctx = NULL;
    }

    // Unload DLL
    if (libusb_dll) {
        FreeLibrary(libusb_dll);
        libusb_dll = NULL;
    }
}

int usb_control_scan_device(device_info_t* devices, int max_devices) {
    libusb_device** devs;
    ssize_t cnt = fn_get_device_list(ctx, &devs);
    if (cnt < 0) {
        printf("Failed to get device list: error code %d\n", (int)cnt);
        return (int)cnt;
    }

    int found = 0;
    for (ssize_t i = 0; i < cnt && found < max_devices; i++) {
        struct libusb_device_descriptor desc;
        int r = fn_get_device_descriptor(devs[i], &desc);
        if (r < 0) {
            printf("Failed to get device descriptor: error code %d\n", r);
            continue;
        }

        if (desc.idVendor == VENDOR_ID && desc.idProduct == PRODUCT_ID) {
            libusb_device_handle* handle;
            r = fn_open(devs[i], &handle);
            if (r < 0) {
                printf("Failed to open device: error code %d\n", r);
                continue;
            }

            // Get serial number
            r = get_string_descriptor(handle, desc.iSerialNumber, devices[found].serial, sizeof(devices[found].serial));
            if (r < 0) {
                fn_close(handle);
                continue;
            }

            // Get manufacturer
            r = get_string_descriptor(handle, desc.iManufacturer, devices[found].manufacturer, sizeof(devices[found].manufacturer));
            if (r < 0) {
                fn_close(handle);
                continue;
            }

            // Get product
            r = get_string_descriptor(handle, desc.iProduct, devices[found].product, sizeof(devices[found].product));
            if (r < 0) {
                fn_close(handle);
                continue;
            }

            fn_close(handle);
            found++;
        }
    }

    fn_free_device_list(devs, 1);
    return found;
}

int usb_control_open_device(const char* target_serial) {
    if (!target_serial) {
        return LIBUSB_ERROR_INVALID_PARAM;
    }

    // Check if device is already open
    if (find_device_instance(target_serial)) {
        return LIBUSB_ERROR_BUSY;
    }

    // Get a free instance slot
    device_instance_t* instance = get_free_instance();
    if (!instance) {
        return LIBUSB_ERROR_NO_MEM;
    }

    // Scan for the device
    libusb_device** devs;
    ssize_t cnt = fn_get_device_list(ctx, &devs);
    if (cnt < 0) {
        return (int)cnt;
    }

    int found = 0;
    for (ssize_t i = 0; i < cnt; i++) {
        struct libusb_device_descriptor desc;
        int r = fn_get_device_descriptor(devs[i], &desc);
        if (r < 0) continue;

        if (desc.idVendor == VENDOR_ID && desc.idProduct == PRODUCT_ID) {
            libusb_device_handle* handle;
            r = fn_open(devs[i], &handle);
            if (r < 0) continue;

            char serial[MAX_STR_LENGTH];
            r = get_string_descriptor(handle, desc.iSerialNumber, serial, sizeof(serial));
            if (r < 0) {
                fn_close(handle);
                continue;
            }

            if (strcmp(serial, target_serial) == 0) {
                // Found the target device
                r = fn_set_configuration(handle, 1);
                if (r < 0) {
                    fn_close(handle);
                    fn_free_device_list(devs, 1);
                    return r;
                }

                r = fn_claim_interface(handle, 0);
                if (r < 0) {
                    fn_close(handle);
                    fn_free_device_list(devs, 1);
                    return r;
                }

                // Initialize the instance
                instance->handle = handle;
                strncpy(instance->serial, target_serial, sizeof(instance->serial) - 1);
                instance->in_use = 1;
                found = 1;
                break;
            }
            fn_close(handle);
        }
    }

    fn_free_device_list(devs, 1);
    return found ? 0 : LIBUSB_ERROR_NO_DEVICE;
}

int usb_control_close_device(const char* target_serial) {
    if (!target_serial) {
        return LIBUSB_ERROR_INVALID_PARAM;
    }

    device_instance_t* instance = find_device_instance(target_serial);
    if (!instance) {
        return LIBUSB_ERROR_NO_DEVICE;
    }

    fn_release_interface(instance->handle, 0);
    fn_close(instance->handle);
    instance->handle = NULL;
    instance->serial[0] = '\0';
    instance->in_use = 0;

    return 0;
}

int usb_control_read(const char* target_serial, unsigned char* data, int length, int* transferred) {
    if (!target_serial || !data || !transferred) {
        return LIBUSB_ERROR_INVALID_PARAM;
    }

    device_instance_t* instance = find_device_instance(target_serial);
    if (!instance) {
        return LIBUSB_ERROR_NO_DEVICE;
    }

    return fn_bulk_transfer(instance->handle, 0x81, data, length, transferred, 1000);  // 1秒超时
}
