import serial
import requests
import time
url = "https://jsonplaceholder.typicode.com/posts"
puerto = "COM5"
baudrate = 115200
llave_proteccion = "7777"
ser = serial.Serial(port=puerto, baudrate=baudrate, timeout=2)
enviar = ""
state = ""

XOR_KEYS = [0x2A, 0x4F, 0x31, 0x5C]
def cifrar_xor(texto):
    resultado = ""
    for i, char in enumerate(texto):
        resultado += chr(ord(char) ^ XOR_KEYS[i % 4])
    return resultado
    
def descifrar_xor(texto_cifrado):
    return cifrar_xor(texto_cifrado)


response = requests.get(url)
data = response.json()


    
for d in data:
    linea = ser.readline().decode('utf-8').strip()
    print(f"Recibido: {linea}")

    if (llave_proteccion + "-state-1") in linea:
        enviar = "T:" + d["title"]
        state = "1"
    elif (llave_proteccion + "-state-2") in linea:
        enviar = "B:" + d["body"]
        state = "2"
    elif state == "1":
        enviar = "T:" + d["title"]
    elif state == "2":
        enviar = "B:" + d["body"]

    ser.write((cifrar_xor(llave_proteccion + "-" + enviar) + "\n").encode('utf-8'))
    time.sleep(0.5)


