import serial
import threading
import time
import sys

class LoraChat:
    def __init__(self, puerto, id, baudrate=115200):
        self.puerto = puerto
        self.id = id
        self.baudrate = baudrate
        self.llave_proteccion = "7777"
        self.ser = None
        self.running = False
        self.receive_thread = None
        
        # ========== CIFRADO XOR MEJORADO ==========
        self.XOR_KEYS = [0x2A, 0x4F, 0x31, 0x5C]  # *, O, 1, \ (MISMA CLAVE QUE ARDUINO)
        
    def cifrar_xor(self, texto):
        resultado = ""
        for i, char in enumerate(texto):
            resultado += chr(ord(char) ^ self.XOR_KEYS[i % 4])
        return resultado
    
    def descifrar_xor(self, texto_cifrado):
        return self.cifrar_xor(texto_cifrado)
    
    def connect(self):
        try:
            self.ser = serial.Serial(port=self.puerto, baudrate=self.baudrate, timeout=1)
            print(f"Conectado al puerto {self.puerto}")
            time.sleep(2)
            print(f"Asignando ID {self.id} al nodo...")
            self.ser.write((self.id + "\n").encode('utf-8'))
            time.sleep(1)
            return True
        except Exception as e:
            print(f"Error al conectar: {e}")
            return False
    
    def receive_messages(self):
        print("Hilo de recepción iniciado...")
        while self.running:
            try:
                if self.ser and self.ser.in_waiting > 0:
                    linea = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    
                    if linea.startswith("ID "):
                        print(f"\n{linea}")
                    
            except Exception as e:
                if self.running:
                    print(f"\nError en recepción: {e}")
            time.sleep(0.1)
    
    def send_messages(self):
        print("🌰 Chat seguro entre nodos LoRa 🌰")
        print("🔒 Comunicación cifrada con XOR")
        print("🐣 Escribe 'salir' para terminar")
        print("=" * 50)
        
        while self.running:
            try:
                mensaje = input("🏁 Ingrese mensaje: ")
                
                if mensaje.lower() == 'salir':
                    self.stop()
                    break
                
                if mensaje.strip():
                    # Crear y cifrar mensaje
                    mensaje_completo = self.llave_proteccion + "-" + self.id + "-" + mensaje
                    mensaje_cifrado = self.cifrar_xor(mensaje_completo)
                    
                    # Enviar mensaje cifrado
                    self.ser.write((mensaje_cifrado + "\n").encode('utf-8'))
                    print("✓ Mensaje cifrado enviado")
                    
            except KeyboardInterrupt:
                print("\n⏹️ Interrupción recibida, saliendo...")
                self.stop()
                break
            except Exception as e:
                print(f"😨 Error al enviar: {e}")
    
    def start(self):
        if not self.connect():
            return
        
        self.running = True
        
        # Iniciar hilo de recepción
        self.receive_thread = threading.Thread(target=self.receive_messages, daemon=True)
        self.receive_thread.start()
        
        # Esperar a que el Arduino procese el ID
        time.sleep(2)
        
        # Usar el hilo principal para enviar mensajes
        self.send_messages()
    
    def stop(self):
        self.running = False
        if self.ser:
            self.ser.close()
        print("Chat terminado")

def main():
    if len(sys.argv) > 2:
        puerto = sys.argv[1]
        id = sys.argv[2] 
    else:
        puerto = input("🔌 Ingresa el puerto de tu nodo (ej: COM3, COM4, COM5): ")
        id = input("🆔 Ingresa el ID de tu nodo (ej: 1, 2, 3): ")
    
    chat = LoraChat(puerto, id)
    
    try:
        chat.start()
    except KeyboardInterrupt:
        print("\nPrograma interrumpido por el usuario")
    finally:
        chat.stop()

if __name__ == "__main__":
    main()