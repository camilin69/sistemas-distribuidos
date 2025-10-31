/**
* NODO - Sistema de registro con cifrado XOR mejorado
*/

#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>

// ========== CONFIGURACIÓN LORA ==========
#define FREQUENCY           866.3
#define BANDWIDTH           250.0
#define SPREADING_FACTOR    7
#define TRANSMIT_POWER      14

// ========== VARIABLES GLOBALES ==========
String mensaje;
volatile bool rxFlag = false;
String llave_proteccion_rx = "7777";
String llaves[100];
int llaves_count = 0;

// ========== CIFRADO XOR MEJORADO ==========
const uint8_t XOR_KEYS[] = {0x2A, 0x4F, 0x31, 0x5C}; // *, O, 1, \ (MISMA CLAVE QUE ADMIN)

String cifrar_xor(const String& texto) {
  String resultado = texto;
  for (int i = 0; i < resultado.length(); i++) {
    resultado[i] = resultado[i] ^ XOR_KEYS[i % 4];
  }
  return resultado;
}

String descifrar_xor(const String& texto_cifrado) {
  return cifrar_xor(texto_cifrado);
}

// ========== DECLARACIÓN DE FUNCIONES ==========
String Start_Rx();
void Start_Tx(String data);
bool encontrar_llave(String llave);

void setup() {
 heltec_setup();
 
 radio.begin();
 radio.setDio1Action(rx);
 radio.setFrequency(FREQUENCY);
 radio.setBandwidth(BANDWIDTH);
 radio.setSpreadingFactor(SPREADING_FACTOR);
 radio.setOutputPower(TRANSMIT_POWER);
 radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
 
 both.println("Nodo listo con cifrado XOR mejorado");
}

void loop() {
  heltec_loop();

  if (rxFlag) {
    String mensaje_cifrado = Start_Rx();
    String mensaje_descifrado = descifrar_xor(mensaje_cifrado);
    
    if(mensaje_descifrado.startsWith("registrar-" + llave_proteccion_rx + "-")) {
        int startIndex = String("registrar-" + llave_proteccion_rx + "-").length();
        String llave_recibida = mensaje_descifrado.substring(startIndex);
        both.printf("Llave a registrar: %s\n", llave_recibida.c_str());

        if (!encontrar_llave(llave_recibida)) {
          llaves[llaves_count] = llave_recibida;
          llaves_count++;
          Start_Tx(cifrar_xor("Llave " + llave_recibida + " registrada exitosamente."));
        } else {
          Start_Tx(cifrar_xor("Llave " + llave_recibida + " ya existe, no se registró."));
        }
        
    } else if (mensaje_descifrado.startsWith("recibir-" + llave_proteccion_rx + "-")) {
        int startIndex = String("recibir-" + llave_proteccion_rx + "-").length();
        String llave_recibida = mensaje_descifrado.substring(startIndex);
        both.printf("Llave recibida: %s\n", llave_recibida.c_str());
        
        if(encontrar_llave(llave_recibida)) {
          Start_Tx(cifrar_xor("Llave " + llave_recibida + " encontrada."));
        } else {
          Start_Tx(cifrar_xor("Llave " + llave_recibida + " no encontrada."));
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
    both.printf("Recibiendo cifrado [%s]\n", rxdata.c_str());
  }
  
  radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
  return rxdata;
}

void Start_Tx(String data) {
  both.printf("Tx cifrado [%s]\n", data.c_str());
  radio.transmit(data.c_str());
  radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);
  rxFlag = false;
}

bool encontrar_llave(String llave) {
  for(int i = 0; i < llaves_count; i++) {
    if (llaves[i] == llave) {
      return true;
    }
  }
  return false;
}