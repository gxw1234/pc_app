CC = gcc
CFLAGS = -I. -L.
TARGET = usb_control

$(TARGET): main.c
	$(CC) -o $(TARGET).exe main.c $(CFLAGS) -lusb-1.0

clean:
	del $(TARGET).exe
