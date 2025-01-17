#include "usb_control.h"

int main() {
    int r;

    // Initialize USB control
    r = usb_control_init();
    if (r < 0) {
        return 1;
    }

    // Open device
    r = usb_control_open();
    if (r < 0) {
        usb_control_exit();
        return 1;
    }

    // Wait for 2 seconds
    printf("Waiting for 2 seconds...\n");
    Sleep(2000);

    // Close device
    r = usb_control_close();
    if (r < 0) {
        usb_control_exit();
        return 1;
    }

    // Cleanup
    printf("Cleaning up...\n");
    usb_control_exit();
    printf("Done!\n");

    return 0;
}
