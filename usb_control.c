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
static libusb_interrupt_transfer_t fn_interrupt_transfer;
static libusb_get_string_descriptor_ascii_t fn_get_string_descriptor_ascii;

// Ring buffer operations
static void ring_buffer_init(ring_buffer_t* rb) {
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

static int ring_buffer_write(ring_buffer_t* rb, const unsigned char* data, int length) {
    int i;
    for (i = 0; i < length && rb->count < RING_BUFFER_SIZE; i++) {
        rb->buffer[rb->head] = data[i];
        rb->head = (rb->head + 1) % RING_BUFFER_SIZE;
        rb->count++;
    }
    return i;  // 返回实际写入的字节数
}

static int ring_buffer_read(ring_buffer_t* rb, unsigned char* data, int max_length) {
    int i;
    for (i = 0; i < max_length && rb->count > 0; i++) {
        data[i] = rb->buffer[rb->tail];
        rb->tail = (rb->tail + 1) % RING_BUFFER_SIZE;
        rb->count--;
    }
    return i;  // 返回实际读取的字节数
}

// Initialize device instances
static void init_device_instances() {
    for (int i = 0; i < MAX_DEVICES; i++) {
        device_instances[i].handle = NULL;
        device_instances[i].serial[0] = '\0';
        device_instances[i].in_use = 0;
        device_instances[i].rx_thread = NULL;
        device_instances[i].rx_running = 0;
        ring_buffer_init(&device_instances[i].rx_buffer);
    }
}

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

// USB接收线程函数
static DWORD WINAPI rx_thread_func(LPVOID param) {
    device_instance_t* dev = (device_instance_t*)param;
    unsigned char temp_buffer[USB_PACKET_SIZE];
    int transferred;

    while (dev->rx_running) {
        // 使用中断传输而不是批量传输
        int result = fn_interrupt_transfer(dev->handle, 0x81, temp_buffer, USB_PACKET_SIZE, &transferred, 100);
        if (result == 0 && transferred > 0) {
            ring_buffer_write(&dev->rx_buffer, temp_buffer, transferred);
        }
    }
    return 0;
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
    fn_interrupt_transfer = (libusb_interrupt_transfer_t)GetProcAddress(libusb_dll, "libusb_interrupt_transfer");
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
    if (!target_serial) return LIBUSB_ERROR_INVALID_PARAM;

    // 检查设备是否已经打开
    device_instance_t* instance = find_device_instance(target_serial);
    if (instance && instance->handle) {
        return LIBUSB_ERROR_BUSY;  // 设备已经打开
    }

    // 获取设备列表
    libusb_device** device_list;
    ssize_t device_count = fn_get_device_list(NULL, &device_list);
    if (device_count < 0) {
        return (int)device_count;
    }

    int result = LIBUSB_ERROR_NOT_FOUND;
    libusb_device_handle* handle = NULL;
    struct libusb_device_descriptor desc;
    unsigned char string_desc[256];

    // 遍历设备列表
    for (ssize_t i = 0; i < device_count; i++) {
        // 获取设备描述符
        result = fn_get_device_descriptor(device_list[i], &desc);
        if (result < 0) continue;

        // 检查 VID/PID
        if (desc.idVendor != VENDOR_ID || desc.idProduct != PRODUCT_ID) continue;

        // 打开设备
        result = fn_open(device_list[i], &handle);
        if (result < 0) continue;

        // 获取序列号
        result = fn_get_string_descriptor_ascii(handle, desc.iSerialNumber, string_desc, sizeof(string_desc));
        if (result < 0) {
            fn_close(handle);
            continue;
        }

        // 比较序列号
        if (strcmp((char*)string_desc, target_serial) == 0) {
            // 找到目标设备
            result = fn_set_configuration(handle, 1);
            if (result < 0) {
                fn_close(handle);
                break;
            }

            result = fn_claim_interface(handle, 0);
            if (result < 0) {
                fn_close(handle);
                break;
            }

            // 保存设备实例
            instance = get_free_instance();
            if (!instance) {
                fn_close(handle);
                result = LIBUSB_ERROR_NO_MEM;
                break;
            }

            instance->handle = handle;
            strncpy(instance->serial, target_serial, MAX_STR_LENGTH - 1);
            instance->serial[MAX_STR_LENGTH - 1] = '\0';
            instance->in_use = 1;
            ring_buffer_init(&instance->rx_buffer);

            // 立即启动接收线程
            result = usb_control_start_rx(target_serial);
            if (result < 0) {
                fn_close(handle);
                instance->handle = NULL;
                instance->in_use = 0;
                break;
            }

            result = LIBUSB_SUCCESS;
            break;
        }

        fn_close(handle);
    }

    fn_free_device_list(device_list, 1);
    return result;
}

int usb_control_close_device(const char* target_serial) {
    device_instance_t* dev = find_device_instance(target_serial);
    if (!dev || !dev->in_use) return LIBUSB_ERROR_NO_DEVICE;

    // 停止接收线程
    if (dev->rx_thread) {
        dev->rx_running = 0;
        WaitForSingleObject(dev->rx_thread, INFINITE);
        CloseHandle(dev->rx_thread);
        dev->rx_thread = NULL;
    }

    fn_release_interface(dev->handle, 0);
    fn_close(dev->handle);
    dev->handle = NULL;
    dev->in_use = 0;

    return LIBUSB_SUCCESS;
}

int usb_control_read(const char* target_serial, unsigned char* data, int length, int* transferred) {
    if (!target_serial || !data || !transferred) {
        return LIBUSB_ERROR_INVALID_PARAM;
    }

    device_instance_t* instance = find_device_instance(target_serial);
    if (!instance) {
        return LIBUSB_ERROR_NO_DEVICE;
    }

    // 从环形缓冲区读取数据
    *transferred = ring_buffer_read(&instance->rx_buffer, data, length);
    return LIBUSB_SUCCESS;
}

int usb_control_start_rx(const char* target_serial) {
    device_instance_t* dev = find_device_instance(target_serial);
    if (!dev || !dev->handle) return LIBUSB_ERROR_NO_DEVICE;

    if (dev->rx_thread) return LIBUSB_ERROR_BUSY;  // 已经在运行

    dev->rx_running = 1;
    dev->rx_thread = CreateThread(NULL, 0, rx_thread_func, dev, 0, NULL);
    if (!dev->rx_thread) {
        dev->rx_running = 0;
        return LIBUSB_ERROR_OTHER;
    }

    return LIBUSB_SUCCESS;
}

int usb_control_stop_rx(const char* target_serial) {
    device_instance_t* dev = find_device_instance(target_serial);
    if (!dev || !dev->handle) return LIBUSB_ERROR_NO_DEVICE;

    if (!dev->rx_thread) return LIBUSB_SUCCESS;  // 已经停止

    dev->rx_running = 0;
    WaitForSingleObject(dev->rx_thread, INFINITE);
    CloseHandle(dev->rx_thread);
    dev->rx_thread = NULL;

    return LIBUSB_SUCCESS;
}
