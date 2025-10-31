import serial
import time

puerto = "COM4"
baudrate = 115200
llave_proteccion = "6969"
ser = serial.Serial(port=puerto, baudrate=baudrate, timeout=2)

# ========== CIFRADO XOR MEJORADO ==========
XOR_KEYS = [0x2A, 0x4F, 0x31, 0x5C]  # *, O, 1, \ (MISMA CLAVE QUE ARDUINO)

def cifrar_xor(texto):
    resultado = ""
    for i, char in enumerate(texto):
        resultado += chr(ord(char) ^ XOR_KEYS[i % 4])
    return resultado

def descifrar_xor(texto_cifrado):
    return cifrar_xor(texto_cifrado)  # XOR es simétrico

aux = 1
llave = ""

while aux > 0:
    try:
        ser.reset_input_buffer()
        time.sleep(0.1)
        # Solicitar estado
        ser.write("estado\n".encode('utf-8'))
        time.sleep(0.5)
        
        state_line = ""
        if ser.in_waiting > 0:
            state_line = ser.readline().decode('utf-8', errors='ignore').strip()
        if not state_line:
            print("No se recibió estado.")
            break
            
        print("\n1. Para refrescar estado\n0. Para salir")
        
        if state_line == "1":
            llave = input("📒 Registrando. Ingrese llave para registrar: \n")
        elif state_line == "2":
            llave = input("📡 Recibiendo. Ingrese llave para verificar registro:\n")
                
        if len(llave) > 6:
            mensaje_plano = llave_proteccion + "-" + llave
            mensaje_cifrado = cifrar_xor(mensaje_plano)
            
            print(f"Mensaje plano: {mensaje_plano}")
            print(f"Mensaje cifrado: {mensaje_cifrado}")
            
            ser.write((mensaje_cifrado + "\n").encode('utf-8'))
            
            print("Esperando respuesta...")
            time.sleep(2)
            
            # Leer respuesta
            if ser.in_waiting > 0:
                all_lines = ser.readlines()
                for line in all_lines:
                    print("Línea recibida:", line.decode('utf-8', errors='ignore').strip())
            
            llave = ""
            
        elif llave == '0':
            aux = 0
            print("Saliendo...")
        elif llave == '1':
            print("Refrescando Estado...")
            
    except Exception as e:
        print(f"❌ Error: {e}")
        break

ser.close()