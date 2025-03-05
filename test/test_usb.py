import ctypes
from ctypes import *
import time

class DeviceInfo(Structure):
    _fields_ = [
        ("serial", c_char * 256),
        ("manufacturer", c_char * 256),
        ("product", c_char * 256)
    ]

def main():
    try:
        dll = CDLL("../USB_G_50.dll")
        print("Successfully loaded USB_G_50.dll")
    except Exception as e:
        print(f"Failed to load DLL: {e}")
        return

    max_devices = 16
    devices = (DeviceInfo * max_devices)()
    dll.USB_ScanDevice.argtypes = [POINTER(DeviceInfo), c_int]
    dll.USB_ScanDevice.restype = c_int
    num_devices = dll.USB_ScanDevice(devices, max_devices)

    if num_devices <= 0:
        print(f"No devices found (error code: {num_devices})")
        return
    print(f"Found {num_devices} device(s)")

    for i in range(num_devices):
        print(f"\nDevice {i + 1}:")
        print(f"  Serial: {devices[i].serial.decode('utf-8')}")
        print(f"  Manufacturer: {devices[i].manufacturer.decode('utf-8')}")
        print(f"  Product: {devices[i].product.decode('utf-8')}")

    # 打开第一个设备
    dll.USB_OpenDevice.argtypes = [c_char_p]
    dll.USB_OpenDevice.restype = c_int
    result = dll.USB_OpenDevice(devices[0].serial)
    # result = dll.USB_OpenDevice("345E34593133".encode())  # 使用 encode() 转换字符串
    if result < 0:
        print(f"Failed to open device (error code: {result})")
        return

    print("\nDevice opened successfully")

    try:
        buffer_size = 64
        data_buffer = (c_ubyte * buffer_size)()
        bytes_transferred = c_int(0)
        dll.USB_ReadData.argtypes = [POINTER(c_ubyte), c_int, POINTER(c_int)]
        dll.USB_ReadData.restype = c_int

        for i in range(1):
            result = dll.USB_ReadData(data_buffer, buffer_size, byref(bytes_transferred))
            if result < 0:
                print(f"Failed to read data (error code: {result})")
                break

            if bytes_transferred.value > 0:
                print(f"\nReceived {bytes_transferred.value} bytes:")
                hex_data = ' '.join([f'{b:02X}' for b in data_buffer[:bytes_transferred.value]])
                print(f"Data: {hex_data}")

            time.sleep(0.1)  # 短暂延时，避免 CPU 占用过高

    except KeyboardInterrupt:
        print("\nTest stopped by user")
    finally:
        dll.USB_CloseDevice.argtypes = []
        dll.USB_CloseDevice.restype = c_int
        result = dll.USB_CloseDevice()
        if result < 0:
            print(f"Failed to close device (error code: {result})")
        else:
            print("Device closed successfully")

if __name__ == "__main__":
    main()
