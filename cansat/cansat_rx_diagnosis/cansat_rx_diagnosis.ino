#include <LoRa.h>

// Pines LoRa
#define LORA_DI00 2
#define LORA_RST 9
#define LORA_NSS 10

// Variables de diagnóstico
unsigned long packetsReceived = 0;
unsigned long lastPacketTime = 0;
unsigned long startTime = 0;
bool loRaInitialized = false;

void setup() {
  Serial.begin(9600);
  delay(3000);
  
  Serial.println();
  Serial.println("=== SNIFFER LoRa RX - DIAGNOSTICO AVANZADO ===");
  Serial.println();
  
  startTime = millis();
  
  // Intentar inicializar LoRa con múltiples configuraciones
  initializeLoRa();
  
  if (loRaInitialized) {
    Serial.println();
    Serial.println("🎯 ESCUCHANDO PAQUETES...");
    Serial.println("   Frecuencia: 433MHz");
    Serial.println("   Modo: Recepción continua");
    Serial.println("===========================================");
  }
}

void initializeLoRa() {
  Serial.println("🔄 INICIALIZANDO RECEPTOR LoRa...");
  
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DI00);
  
  // Probar diferentes frecuencias cercanas
  long frequencies[] = {433E6, 433.1E6, 432.9E6, 433.2E6, 432.8E6};
  
  for (int i = 0; i < 5; i++) {
    Serial.print("   Probando ");
    Serial.print(frequencies[i] / 1E6, 1);
    Serial.print(" MHz... ");
    
    if (LoRa.begin(frequencies[i])) {
      loRaInitialized = true;
      Serial.println("✅ OK");
      
      // Configurar para mejor recepción
      LoRa.setSpreadingFactor(7);
      LoRa.setSignalBandwidth(125E3);
      LoRa.setCodingRate4(5);
      LoRa.setSyncWord(0x12);
      
      Serial.print("   ✅ Receptor activo en ");
      Serial.print(frequencies[i] / 1E6, 1);
      Serial.println(" MHz");
      break;
      
    } else {
      Serial.println("❌ Fallo");
    }
    delay(500);
  }
  
  if (!loRaInitialized) {
    Serial.println();
    Serial.println("❌ CRITICAL: No se pudo inicializar ningún receptor");
    Serial.println("🔧 Verificar:");
    Serial.println("   - Conexiones de pines");
    Serial.println("   - Alimentación 3.3V estable");
    Serial.println("   - Módulo LoRa funcionando");
    return;
  }
  
  // Configurar callback para recepción (usando DIO0)
  LoRa.onReceive(onReceive);
  LoRa.receive();
  
  Serial.println("   ✅ Modo recepción activado");
}

void loop() {
  // El procesamiento principal se hace en onReceive()
  
  // Mostrar estado periódicamente
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus >= 5000) {
    showReceptionStatus();
    lastStatus = millis();
  }
  
  // Reiniciar recepción si hay timeout
  if (packetsReceived > 0 && (millis() - lastPacketTime > 30000)) {
    Serial.println("🔄 Reiniciando recepción...");
    LoRa.receive();
    lastPacketTime = millis();
  }
  
  delay(100);
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;
  
  packetsReceived++;
  lastPacketTime = millis();
  
  // Leer paquete
  String message = "";
  while (LoRa.available()) {
    message += (char)LoRa.read();
  }
  
  // Mostrar información detallada
  Serial.println();
  Serial.println("=== PAQUETE RECIBIDO ===");
  Serial.print("📦 Paquete #");
  Serial.println(packetsReceived);
  Serial.print("🕒 Tiempo: ");
  Serial.print((millis() - startTime) / 1000);
  Serial.println("s");
  Serial.print("📏 Tamaño: ");
  Serial.print(packetSize);
  Serial.println(" bytes");
  Serial.print("📶 RSSI: ");
  Serial.print(LoRa.packetRssi());
  Serial.println(" dBm");
  Serial.print("📡 SNR: ");
  Serial.print(LoRa.packetSnr());
  Serial.println(" dB");
  Serial.print("🎯 Frecuencia error: ");
  Serial.print(LoRa.packetFrequencyError());
  Serial.println(" Hz");
  Serial.print("💾 Contenido: ");
  Serial.println(message);
  Serial.println("========================");
  
  // Reactivar recepción
  LoRa.receive();
}

void showReceptionStatus() {
  Serial.println();
  Serial.println("📊 ESTADO RECEPCION:");
  Serial.print("   Paquetes recibidos: ");
  Serial.println(packetsReceived);
  Serial.print("   Tiempo ejecución: ");
  Serial.print((millis() - startTime) / 1000);
  Serial.println(" segundos");
  
  if (packetsReceived > 0) {
    Serial.print("   Último paquete hace: ");
    Serial.print((millis() - lastPacketTime) / 1000);
    Serial.println(" segundos");
    Serial.println("   ✅ RECEPTOR FUNCIONANDO");
  } else {
    Serial.println("   ❌ NO SE RECIBEN PAQUETES");
    Serial.println("   🔄 Escaneando continuamente...");
  }
  
  // Verificar señal de fondo
  int rssi = LoRa.rssi();
  Serial.print("   RSSI fondo: ");
  Serial.print(rssi);
  Serial.println(" dBm");
}