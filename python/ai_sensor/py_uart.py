import serial
import time

class UartAdapter:
    def __init__(self, port, baudrate=115200):
        self.ser = serial.Serial(port, baudrate, timeout=0.1)
        
    def write(self, data: bytes):
        self.ser.write(data)
        
    def read_all(self) -> bytes:
        return self.ser.read(self.ser.in_waiting or 1)
        
    def get_time_ms(self):
        return int(time.time() * 1000)
    