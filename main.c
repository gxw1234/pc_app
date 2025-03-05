#include "usb_control.h"

int main(int argc, char* argv[]) {
    int r;
    device_info_t devices[MAX_DEVICES];
    int selected_device = 0;  // 默认选择第一个设备

    // Initialize USB control
    r = usb_control_init();
    if (r < 0) {
        return r;
    }

    // Get all matching devices
    r = USB_ScanDevice(devices, MAX_DEVICES);
    if (r < 0) {
        printf("Failed to get device list: %s\n", libusb_error_name(r));
        usb_control_exit();
        return r;
    }
    int num_devices = r;

    // Display device list
    printf("\nFound %d device(s):\n", num_devices);

    for (int i = 0; i < num_devices; i++) {
        printf("Device %d:\n", i + 1);
        printf("  Manufacturer: %s\n", devices[i].manufacturer);
        printf("  Product: %s\n", devices[i].product);
        printf("  S/N: %s\n", devices[i].serial);
        printf("\n");
    }

    // Check command line arguments
    if (argc > 1) {
        selected_device = atoi(argv[1]) - 1;  // Convert from 1-based to 0-based index
        if (selected_device < 0 || selected_device >= num_devices) {
            printf("Invalid device number. Please select 1-%d\n", num_devices);
            usb_control_exit();
            return -1;
        }
    }

    // Open selected device
    printf("Opening device %d (S/N: %s)\n", 
           selected_device + 1, devices[selected_device].serial);

           
    r = USB_OpenDevice(devices[selected_device].serial);
    if (r < 0) {
        usb_control_exit();
        return r;
    }

    // Read data for 2 seconds
    printf("Reading data for 2 seconds...\n");
    DWORD start_time = GetTickCount();
    unsigned char data[64];
    int transferred;
    
    while (GetTickCount() - start_time < 2000) {
        r = usb_control_read(data, sizeof(data), &transferred);
        if (r == 0 && transferred > 0) {
            printf("Received %d bytes: ", transferred);
            for (int i = 0; i < transferred && i < 16; i++) {  // 最多显示16字节
                printf("%02X ", data[i]);
            }
            if (transferred > 16) printf("...");
            printf("\n");
        }
        else if (r < 0 && r != LIBUSB_ERROR_TIMEOUT) {
            printf("Read error: %s\n", libusb_error_name(r));
            break;
        }
    }

    // Wait for 2 seconds
    printf("Waiting for 2 seconds...\n");
    Sleep(2000);

    // Close device
    r = USB_CloseDevice();
    if (r < 0) {
        usb_control_exit();
        return r;
    }

    // Cleanup
    printf("Cleaning up...\n");
    usb_control_exit();
    printf("Done!\n");

    return 0;
}
