from machine import UART
import time

class UartAdapter:
    def __init__(self, port, baudrate=115200, tx=17, rx=16):
        self.uart = UART(port, baudrate=baudrate, tx=tx, rx=rx)
        
    def write(self, data: bytes):
        self.uart.write(data)
        
    def read_all(self) -> bytes:
        if self.uart.any():
            return self.uart.read()
        return b''

    def get_time_ms(self):
        return time.ticks_ms()
    