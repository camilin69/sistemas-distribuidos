import serial
puerto = "COM5"
baudrate = 115200
llave_proteccion = "6969"
ser = serial.Serial(port=puerto, baudrate=baudrate, timeout=2)

while True:
    if ser.in_waiting > 0:
        linea = ser.readline().decode('utf-8', errors='ignore').strip()
        print(f"Recibido en nodo {linea}")