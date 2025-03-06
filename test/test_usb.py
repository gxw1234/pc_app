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

    # ===================扫描USB设备==================================
    # max_devices: 最大扫描设备数量
    # DeviceInfo: USB设备信息
    # USB_ScanDevice: 开始扫描
    #
    # Returns:
    #   num_devices: 扫描USB个数
    # ==============================================================
    max_devices = 16
    devices = (DeviceInfo * max_devices)()

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


    # ===================打开USB设备==================================
    # USB_OpenDevice: 打开设备
    # devices[0].serial: 扫描出来的设备号， 如果有已知的设备号可以转换  "xxxxxxxxx".encode()
    # USB_ScanDevice: 开始扫描
    #
    # Returns:
    #   result: 打开的结果
    # ==============================================================

    result = dll.USB_OpenDevice(devices[0].serial)
    print(f'result:{result}')
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
