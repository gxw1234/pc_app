#include "usb_control.h"

// Global variables
static HMODULE dll = NULL;
static libusb_context* ctx = NULL;
static libusb_device_handle* handle = NULL;
static libusb_device** devs = NULL;

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

void usb_control_exit(void) {
    if (handle) {
        libusb_release_interface(handle, 0);
        libusb_close(handle);
        handle = NULL;
    }
    
    if (devs) {
        libusb_free_device_list(devs, 1);
        devs = NULL;
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

int usb_control_open(void) {
    int r;
    
    // Get device list
    ssize_t cnt = libusb_get_device_list(ctx, &devs);
    if (cnt < 0) {
        printf("Get Device Error: %s\n", libusb_error_name((int)cnt));
        return (int)cnt;
    }
    printf("Found %d devices\n", (int)cnt);

    // Find our device
    libusb_device* dev = NULL;
    struct libusb_device_descriptor desc;
    int i = 0;
    int found = 0;
    libusb_device_handle* temp_handle = NULL;

    while ((dev = devs[i++]) != NULL) {
        r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0) {
            printf("Failed to get device descriptor: %s\n", libusb_error_name(r));
            continue;
        }

        printf("Device %d: VID=0x%04x, PID=0x%04x", i, desc.idVendor, desc.idProduct);
        
        // Try to open the device to get string descriptors
        r = libusb_open(dev, &temp_handle);
        if (r == 0 && temp_handle != NULL) {
            unsigned char string[256];
            
            printf("\n  Descriptor indexes: iManufacturer=%d, iProduct=%d, iSerialNumber=%d", 
                   desc.iManufacturer, desc.iProduct, desc.iSerialNumber);

            // Set configuration
            r = libusb_set_configuration(temp_handle, 1);
            if (r < 0) {
                printf("\n  Failed to set configuration: %s", libusb_error_name(r));
            }

            // Claim interface
            r = libusb_claim_interface(temp_handle, 0);
            if (r < 0) {
                printf("\n  Failed to claim interface: %s", libusb_error_name(r));
            }
            
            // Get manufacturer string
            if (desc.iManufacturer > 0) {
                printf("\n  Trying to get manufacturer string...");
                r = libusb_get_string_descriptor_ascii(temp_handle, desc.iManufacturer, string, sizeof(string));
                if (r > 0) {
                    printf(" Success: %s", string);
                    printf(", Manufacturer: %s", string);
                } else {
                    printf(" Failed: %s", libusb_error_name(r));
                }
            }
            
            // Get product string
            if (desc.iProduct > 0) {
                printf("\n  Trying to get product string...");
                r = libusb_get_string_descriptor_ascii(temp_handle, desc.iProduct, string, sizeof(string));
                if (r > 0) {
                    printf(" Success: %s", string);
                    printf(", Product: %s", string);
                } else {
                    printf(" Failed: %s", libusb_error_name(r));
                }
            }
            
            // Get serial number
            if (desc.iSerialNumber > 0) {
                printf("\n  Trying to get serial number...");
                r = libusb_get_string_descriptor_ascii(temp_handle, desc.iSerialNumber, string, sizeof(string));
                if (r > 0) {
                    printf(" Success: %s", string);
                    printf(", S/N: %s", string);
                } else {
                    printf(" Failed: %s", libusb_error_name(r));
                }
            }
            
            libusb_release_interface(temp_handle, 0);
            libusb_close(temp_handle);
        } else {
            printf(" (Cannot open device: %s)", libusb_error_name(r));
        }
        printf("\n");
        
        if (desc.idVendor == VENDOR_ID && desc.idProduct == PRODUCT_ID) {
            found = 1;
            printf("Found target device!\n");
            break;
        }
    }

    if (!found) {
        printf("Device not found\n");
        return LIBUSB_ERROR_NOT_FOUND;
    }

    // Open the device
    r = libusb_open(dev, &handle);
    if (r < 0) {
        printf("Error opening device: %s\n", libusb_error_name(r));
        return r;
    }
    printf("Device opened successfully\n");

    // Set configuration
    r = libusb_set_configuration(handle, 1);
    if (r < 0) {
        printf("Error setting configuration: %s\n", libusb_error_name(r));
        libusb_close(handle);
        handle = NULL;
        return r;
    }
    printf("Configuration set successfully\n");

    // Claim interface
    r = libusb_claim_interface(handle, 0);
    if (r < 0) {
        printf("Error claiming interface: %s\n", libusb_error_name(r));
        libusb_close(handle);
        handle = NULL;
        return r;
    }
    printf("Interface claimed successfully\n");

    // Send open command
    unsigned char data = 1;
    int transferred = 0;
    printf("Sending open command...\n");
    
    r = libusb_bulk_transfer(handle, 0x01, &data, 1, &transferred, 1000);
    if (r == 0) {
        printf("Device opened successfully (transferred %d bytes)\n", transferred);
    } else {
        printf("Error in bulk transfer: %s\n", libusb_error_name(r));
        libusb_release_interface(handle, 0);
        libusb_close(handle);
        handle = NULL;
        return r;
    }

    return 0;
}

int usb_control_close(void) {
    if (!handle) {
        return LIBUSB_ERROR_NOT_FOUND;
    }

    // Send close command
    unsigned char data = 0;
    int transferred = 0;
    printf("Sending close command...\n");
    
    int r = libusb_bulk_transfer(handle, 0x01, &data, 1, &transferred, 1000);
    if (r == 0) {
        printf("Device closed successfully (transferred %d bytes)\n", transferred);
    } else {
        printf("Error in bulk transfer: %s\n", libusb_error_name(r));
    }

    libusb_release_interface(handle, 0);
    libusb_close(handle);
    handle = NULL;

    return r;
}
