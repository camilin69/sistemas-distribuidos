/**
* NODO CHAT - Sistema de chat seguro con cifrado XOR
*/

#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>

// ========== CONFIGURACIÓN LORA ==========
#define FREQUENCY           866.3
#define BANDWIDTH           250.0
#define SPREADING_FACTOR    7
#define TRANSMIT_POWER      14

// ========== VARIABLES GLOBALES ==========
String mensaje_enviado;
String mensaje_recibido;
volatile bool rxFlag = false;
String llave_proteccion = "7777";
int i1, i2;

// ========== CIFRADO XOR MEJORADO ==========
const uint8_t XOR_KEYS[] = {0x2A, 0x4F, 0x31, 0x5C}; // *, O, 1, \ (clave de 4 bytes)

String cifrar_xor(const String& texto) {
  String resultado = texto;
  for (int i = 0; i < resultado.length(); i++) {
    resultado[i] = resultado[i] ^ XOR_KEYS[i % 4];
  }
  return resultado;
}

String descifrar_xor(const String& texto_cifrado) {
  return cifrar_xor(texto_cifrado); // XOR es simétrico
}

// ========== DECLARACIÓN DE FUNCIONES ==========
String Start_Rx();
void Start_Tx(String data);

void setup() {
 heltec_setup();
 
 radio.begin();
 radio.setDio1Action(rx);
 radio.setFrequency(FREQUENCY);
 radio.setBandwidth(BANDWIDTH);
 radio.setSpreadingFactor(SPREADING_FACTOR);
 radio.setOutputPower(TRANSMIT_POWER);
 radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
 
 both.println("Admin Chat Seguro");
}

void loop() {
  heltec_loop();
  
  if(Serial.available()) {
    mensaje_enviado = Serial.readStringUntil('\n');
    mensaje_enviado.trim();
    
    if (mensaje_enviado.length() > 0) {
      String mensaje_descifrado = descifrar_xor(mensaje_enviado);
      
      if(mensaje_descifrado.startsWith(llave_proteccion + "-admin-")) {
        mensaje_enviado = cifrar_xor(mensaje_descifrado);
        Start_Tx(mensaje_enviado);
      }
    }
  } 
  
  if (rxFlag) { 
    mensaje_recibido = Start_Rx();
    mensaje_recibido.trim();
    
    if (mensaje_recibido.length() > 0) {
      String mensaje_descifrado = descifrar_xor(mensaje_recibido);

      if(mensaje_descifrado.startsWith(llave_proteccion + "-")) {
        i1 = mensaje_descifrado.indexOf("-");
        i2 = mensaje_descifrado.indexOf("-", i1 + 1);
        String id_remitente = mensaje_descifrado.substring(i1 + 1, i2);
        String mensaje = mensaje_descifrado.substring(i2 + 1);
        both.println("ID " + id_remitente + " dice: " + mensaje);
        
      } else {
        both.println("Mensaje recibido no válido o corrupto");
      }
    }
  }
  
}

void rx() {
  rxFlag = true;
}

String Start_Rx() {
  String rxdata;
  rxFlag = false;
  
  int status = radio.readData(rxdata);
  if (status == RADIOLIB_ERR_NONE) {
    both.println("Mensaje recibido! \n" + rxdata);
    return rxdata;
  }
  
  radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
  return ""; // Retornar string vacío si no hay datos
}

void Start_Tx(String data) {
  both.println("Transmitiendo mensaje cifrado\n" + data);
  radio.transmit(data.c_str());
  radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
  rxFlag = false;
}