import serial
import time

XOR_KEYS = [0x2A, 0x4F, 0x31, 0x5C]

class Receiver:
    def __init__(self, port, baudrate):
        self.port = port
        self.baudrate = baudrate
        self.ser = None

    def connect(self):
        try:
            self.ser = serial.Serial(port=self.port, baudrate=self.baudrate, timeout=1)
            time.sleep(2)
            return True
        except Exception as e:
            print(f"Connection Error: {e}")
            return False
    
    def cypher_xor(self, text):
        result = ""
        for i, char in enumerate(text):
            result += chr(ord(char) ^ XOR_KEYS[i % 4])
        return result
        
    def decypher_xor(self, text_cyphered):
        return self.cypher_xor(text_cyphered)
    
    def receive_data(self):
        if not self.ser or not self.ser.is_open:
            print("Serial port is not open.")
            return None
        try:
            raw_data = self.ser.readline().decode('utf-8').strip()
            
            return raw_data
        except Exception as e:
            print(f"Data Reception Error: {e}")
            return None
        
    def ask_for_data(self, command):
        if not self.ser or not self.ser.is_open:
            print("Serial port is not open.")
            return None
        try:
            cyphered_command = self.cypher_xor(command)
            self.ser.write((cyphered_command + '\n').encode('utf-8'))
            time.sleep(0.5)
            data = self.receive_data()
            return data
        except Exception as e:
            print(f"Data Request Error: {e}")
            return None

    def close(self):
        if self.ser and self.ser.is_open:
            self.ser.close()