/**
* NODO USUARIO - Sistema de comunicación LoRa
*/

#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>

// ========== CONFIGURACIÓN LORA ==========
#define PAUSE               0
#define FREQUENCY           866.3        // Frecuencia de comunicación en MHz
#define BANDWIDTH           250.0        // Ancho de banda en kHz
#define SPREADING_FACTOR    7            // Factor de dispersión (velocidad vs alcance)
#define TRANSMIT_POWER      14           // Potencia de transmisión en dBm

// ========== VARIABLES GLOBALES ==========
String message;                         // Buffer para mensajes recibidos
volatile bool rxFlag = false;           // Bandera de interrupción para recepción
uint8_t state = 1;                     // Estado actual del sistema (1=principal, 2=envío, 3=registrando)
uint8_t myId = 0;                      // ID asignado por el servidor
bool registered = false;               // Estado de registro en la red

// Array de mensajes predefinidos que puede enviar el usuario
String messages[] = {"Hola", "Como estas", "Bien", "Prueba", "Saludos"};
int msgIndex = 0;                      // Índice del mensaje actual

// ========== DECLARACIÓN DE FUNCIONES ==========
String Start_Rx();                    // Función para recibir datos LoRa
void updateInterface();               // Actualizar interfaz de usuario en pantalla
void Start_Tx(String data);          // Función para transmitir datos LoRa

void setup() {
 // Inicialización del hardware Heltec (radio, pantalla, serial)
 heltec_setup();
 both.println("=== NODO USUARIO===");
 both.println("Radio init");
 
 // Configuración del módulo LoRa
 radio.begin();
 radio.setDio1Action(rx);                                    // Configurar interrupción para recepción
 radio.setFrequency(FREQUENCY);                              // Establecer frecuencia
 radio.setBandwidth(BANDWIDTH);                              // Establecer ancho de banda
 radio.setSpreadingFactor(SPREADING_FACTOR);                 // Establecer factor de dispersión
 radio.setOutputPower(TRANSMIT_POWER);                       // Establecer potencia de salida
 radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);        // Iniciar modo recepción continua
 
 both.println("Usuario listo");
 updateInterface();                                           // Mostrar interfaz inicial
}

void loop() {
  heltec_loop();  // Función obligatoria para el funcionamiento de Heltec
  
  // ========== DETECCIÓN DEL BOTÓN PRG ==========
  static bool lastButtonState = HIGH;     // Estado anterior del botón
  static unsigned long lastButtonTime = 0; // Timestamp del último cambio
  
  bool currentButton = digitalRead(0);     // Leer estado actual del botón PRG (pin 0)
  
  // Detectar flanco descendente con debounce de 500ms
  if (currentButton == LOW && lastButtonState == HIGH && (millis() - lastButtonTime) > 500) {
    
    if (!registered) {
      // CASO 1: Usuario no registrado - Iniciar proceso de registro
      Start_Tx("REG_0101");          // Enviar solicitud de registro al servidor
      delay(1000);
      if (rxFlag) {  // Si se recibió un mensaje de asignación de id (activado por interrupción)
        message = Start_Rx();  // Leer el mensaje recibido
        
        int indexId = message.indexOf('_');
        // Procesar respuesta de registro
        if (message.substring(indexId + 1).toInt() >= 2 && message.indexOf("0101") != -1) {  // Si estamos esperando ID y recibimos un número >= 2
          myId = message.toInt();     // Asignar ID recibido del servidor
          registered = true;          // Marcar como registrado exitosamente
          both.printf("REGISTRADO CON ID: %d\n", myId);
        }
      }
    } else {
      switch (state) {
        case 1: 
          // Formato del mensaje: "MSG_[ID]_[contenido]"
          String msgToSend = "MSG_" + String(myId) + "_" + messages[msgIndex];
          Start_Tx(msgToSend);      // Enviar mensaje
          msgIndex = (msgIndex + 1) % 5;  // Avanzar al siguiente mensaje (circular)
          break;
      }
    }
    updateInterface();          // Actualizar pantalla después del cambio de estado
    lastButtonTime = millis();  // Guardar timestamp para debounce
  }
  if(registered) {
    //Espera a que la base pida los IDS de los usuarios
    if(rxFlag) {
      message = Start_Rx();
      if(message.startsWith("ASK")) {
        int indexId = message.indexOf('_');
        if(indexId != -1 && indexId < message.length() - 1){
          String idStr = message.substring(indexId + 1);
          int requestedId = idStr.toInt();
          if(requestedId != 0 && requestedId == myId) {
              both.println("Reportandose: ID " + String(myId));
              Start_Tx("Usuario" + String(myId) + "reportandose");
          }
        } else {
          both.println("Formato ASK invalido: " + message);
        }
      }
    }
  }
  
  lastButtonState = currentButton;  // Actualizar estado anterior del botón
 
}

/**
* Actualiza la interfaz de usuario en la pantalla OLED según el estado actual
*/
void updateInterface() {
  display.clear();  // Limpiar pantalla
  
  switch (state) {
    case 1:
      // ========== INTERFAZ PRINCIPAL ==========
      if (!registered) {
        // Usuario no registrado
        display.println("=== SIN REGISTRO ===");
        display.println("Presiona PRG");
        display.println("para registrar");
        } else {
          // Usuario registrado
          display.println("=== USUARIO ===");
          display.printf("Mi ID: %d\n", myId);
        }
      break;
  }
  
  display.display();  // Actualizar pantalla física
}

/**
* Función de interrupción para recepción LoRa
* Se ejecuta automáticamente cuando llega un mensaje
*/
void rx() {
 rxFlag = true;  // Activar bandera de mensaje recibido
}

/**
* Función para recibir y procesar datos LoRa
* @return String con los datos recibidos
*/
String Start_Rx() {
 String rxdata;
 rxFlag = false;  // Limpiar bandera de recepción
 
 radio.readData(rxdata);  // Leer datos del buffer de recepción
 
 // Verificar si la recepción fue exitosa
 if (_radiolib_status == RADIOLIB_ERR_NONE) {
   both.printf("RX [%s]\n", rxdata.c_str());  // Mostrar mensaje recibido
 }
 
 radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);  // Reactivar modo recepción
 return rxdata.c_str();
}

/**
* Función para transmitir datos por LoRa
* @param data String con los datos a transmitir
*/
void Start_Tx(String data) {
  both.printf("TX [%s]\n", data.c_str());  // Mostrar mensaje a enviar
  radio.transmit(data.c_str());             // Transmitir datos
  last_tx = millis();                       // Guardar timestamp de transmisión
  radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);  // Volver a modo recepción
  rxFlag = false;
}