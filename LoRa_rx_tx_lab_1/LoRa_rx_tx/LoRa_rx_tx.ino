/**
* NODO BASE - Sistema central de comunicación LoRa con 2 interfaces
* Interfaz 1: Registrar nuevos usuarios y recibir mensajes de usuarios
* Interfaz 2: Pedir IDS a los usuarios registrados
*/

#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>

// ========== CONFIGURACIÓN LORA ==========
#define PAUSE               300          // Intervalo de heartbeat en segundos (0 = desactivado)
#define FREQUENCY           866.3        // Frecuencia de comunicación en MHz
#define BANDWIDTH           250.0        // Ancho de banda en kHz
#define SPREADING_FACTOR    7            // Factor de dispersión (velocidad vs alcance)
#define TRANSMIT_POWER      14           // Potencia de transmisión en dBm

// ========== VARIABLES GLOBALES ==========
String message;                         // Buffer para mensajes recibidos
volatile bool rxFlag = false;           // Bandera de interrupción para recepción
long counter = 0;                       // Contador para mensajes heartbeat
uint64_t last_tx = 0;                  // Timestamp de última transmisión
uint8_t state = 1;                     // Estado actual de la interfaz (1, 2 o 3)
int nodeCount = 0;                     // Número total de usuarios registrados
uint8_t nextId = 2;                    // Próximo ID a asignar (comienza en 2, el 1 es para la base)

// ========== ARRAYS PARA GESTIÓN DE USUARIOS ==========
uint8_t userIds[10];                   // IDs de usuarios registrados (máximo 10)
String lastMessage[10];                // Último mensaje de cada usuario

// ========== VARIABLES DE CONTROL DE PANTALLA ==========
String currentDisplayMsg = "";         // Mensaje actual a mostrar temporalmente
unsigned long msgDisplayTime = 0;      // Timestamp de cuando se mostró el mensaje

// ========== DECLARACIÓN DE FUNCIONES ==========
String Start_Rx();                    // Función para recibir datos LoRa
void updateInterface();               // Actualizar interfaz según estado actual
void Start_Tx(String data);          // Función para transmitir datos LoRa
void addUser(uint8_t id);            // Añadir nuevo usuario al registro
void updateUserMessage(uint8_t id, String msg);  // Actualizar último mensaje de usuario

void setup() {
 // Inicialización del hardware Heltec (radio, pantalla, serial)
 heltec_setup();
 both.println("=== NODO BASE===");
 both.println("Radio init");
 
 // Configuración del módulo LoRa
 radio.begin();
 radio.setDio1Action(rx);                                    // Configurar interrupción para recepción
 radio.setFrequency(FREQUENCY);                              // Establecer frecuencia
 radio.setBandwidth(BANDWIDTH);                              // Establecer ancho de banda
 radio.setSpreadingFactor(SPREADING_FACTOR);                 // Establecer factor de dispersión
 radio.setOutputPower(TRANSMIT_POWER);                       // Establecer potencia de salida
 radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);        // Iniciar modo recepción continua
 
 both.println("Base lista");
 
 updateInterface();
}

void loop() {
 heltec_loop();  // Función obligatoria para el funcionamiento de Heltec
 
  // ========== DETECCIÓN DEL BOTÓN PRG PARA CAMBIAR INTERFACES ==========
  static bool lastButtonState = HIGH;     // Estado anterior del botón
  static unsigned long lastButtonTime = 0; // Timestamp del último cambio
  
  bool currentButton = digitalRead(0);     // Leer estado actual del botón PRG (pin 0)
 
  // Detectar flanco descendente con debounce de 300ms
  if (currentButton == LOW && lastButtonState == HIGH && (millis() - lastButtonTime) > 300) {
    state++;                              // Avanzar al siguiente estado
    if (state > 2) state = 1;            
    both.printf("*** CAMBIANDO A INTERFAZ %d ***\n", state);
    lastButtonTime = millis();            // Guardar timestamp para debounce
    updateInterface();                    // Actualizar interfaz inmediatamente
  }
  lastButtonState = currentButton;        // Actualizar estado anterior del botón
 
  // ========== PROCESAMIENTO DE MENSAJES RECIBIDOS ==========
  if (rxFlag && state == 1) {  // Si se recibió un mensaje (activado por interrupción)
    message = Start_Rx();  // Leer el mensaje recibido
    
    // ========== PROCESAR SOLICITUD DE REGISTRO ==========
    if (message == "REG_0101") {
      addUser(nextId);                    // Añadir usuario a la lista
      Start_Tx(String(nextId) + "_0101");          // Responder con el ID asignado
      
      // Preparar mensaje temporal para mostrar en pantalla
      currentDisplayMsg = "NUEVO ID:" + String(nextId);
      msgDisplayTime = millis();
      nextId++;                          // Incrementar ID para próximo usuario      
    } 
    // ========== PROCESAR MENSAJES DE USUARIOS ==========
    else if (message.startsWith("MSG_")) {
      // Formato esperado: "MSG_[ID]_[contenido]"
      int firstUnderscore = message.indexOf('_');           // Encontrar primer guión bajo
      int secondUnderscore = message.indexOf('_', firstUnderscore + 1);  // Encontrar segundo guión bajo
      
      // Verificar formato correcto del mensaje
      if (firstUnderscore > 0 && secondUnderscore > firstUnderscore) {
        // Extraer ID del remitente
        uint8_t senderId = message.substring(firstUnderscore + 1, secondUnderscore).toInt();
        // Extraer contenido del mensaje
        String userMsg = message.substring(secondUnderscore + 1);
        
        updateUserMessage(senderId, userMsg);  // Actualizar registro del usuario
        
        // Preparar mensaje temporal para mostrar
        currentDisplayMsg = "ID:" + String(senderId) + " >> " + userMsg;
        msgDisplayTime = millis();
        
        both.printf("USUARIO ID:%d envió: %s\n", senderId, userMsg.c_str());
      }
    }
  }
}

void updateInterface() {
  display.clear();  // Limpiar pantalla
 
  switch (state) {
    case 1:
      // ========== INTERFAZ 1: REGISTRO DE USUARIOS ==========
      display.println("=== REGISTRO USUARIOS ===");
      
      // Mostrar mensaje temporal si es reciente (menos de 4 segundos)
      if (millis() - msgDisplayTime < 4000 && currentDisplayMsg != "") {
        display.println(">> " + currentDisplayMsg + " <<");
      } else {
        display.println("\nEsperando...");
      }
      break;
      
    case 2:
      // ========== INTERFAZ 2: PIDIENDO  USUARIOS ==========
      display.println("=== PIDIENDO USUARIOS ===");
      if (nodeCount > 0) {
        display.printf("Total: %d\n", nodeCount);
        
        // Mostrar últimos 2 usuarios (o todos si hay menos de 2)
        for (int i = 0; i < nodeCount; i++) {
          display.printf("Pidiendo Usuario: %d\n", userIds[i]);
          Start_Tx("ASK_" + String(userIds[i]));
          delay(3000);
          message = Start_Rx();
        }
      } else {
        display.println("Sin usuarios");
      }
      break;
  }
 
  display.display();  // Actualizar pantalla física
}


void rx() {
  rxFlag = true;  // Activar bandera de mensaje recibido
}

String Start_Rx() {
  String rxdata;
  rxFlag = false;  // Limpiar bandera de recepción
  
  radio.readData(rxdata);  // Leer datos del buffer de recepción
  
  // Verificar si la recepción fue exitosa
  if (_radiolib_status == RADIOLIB_ERR_NONE) {
    both.printf("RX [%s]\n", rxdata.c_str());  // Mostrar mensaje recibido en consola
  }
  
  radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);  // Reactivar modo recepción
  return rxdata;
}


void Start_Tx(String data) {
  both.printf("TX [%s]\n", data.c_str());  // Mostrar mensaje a enviar en consola
  radio.transmit(data.c_str());             // Transmitir datos
  last_tx = millis();                       // Guardar timestamp de transmisión
  radio.startReceive(RADIOLIB_SX126X_RX_TIMEOUT_INF);  // Volver a modo recepción
  rxFlag = false;
}


void addUser(uint8_t id) {
  if (nodeCount < 10) {  // Verificar que no se exceda el límite de usuarios
    userIds[nodeCount] = id;                    // Guardar ID del usuario
    lastMessage[nodeCount] = "Registrado";      // Mensaje inicial
    nodeCount++;                                // Incrementar contador de usuarios
  }
}

void updateUserMessage(uint8_t id, String msg) {
  // Buscar el usuario en la lista y actualizar su información
  for (int i = 0; i < nodeCount; i++) {
    if (userIds[i] == id) {
      lastMessage[i] = msg;      // Actualizar último mensaje
      break;                     // Salir del bucle una vez encontrado
    }
  }
}