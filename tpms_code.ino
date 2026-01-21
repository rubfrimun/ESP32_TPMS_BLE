// TPMS - Sistema de monitoreo de presión de neumáticos
// VERSIÓN FINAL - Envío de datos a un ESP32 Maestro vía BLE usando un único paquete de datos.
// Este dispositivo actúa como un "Gateway" o "Puente":
// 1. Escanea y decodifica los sensores TPMS (rol de Cliente BLE).
// 2. Crea un Servidor BLE con una única característica para que un ESP32 Maestro pueda leer los datos (rol de Servidor BLE).

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>

// --- Direcciones MAC de tus sensores TPMS ---
const char* macSensorDelantera = "4b:a1:00:00:7b:3a";
const char* macSensorTrasera   = "4b:9c:00:00:a1:13";

// --- Factor de conversión de PSI a Bar ---
const float PSI_TO_BAR = 0.0689476f;

// --- UUIDs para el Servicio y Característica BLE ---
// El maestro solo necesita conectarse a esta única característica para recibir todos los datos.
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_TPMS "beb5483e-36e1-4688-b7f5-ea07361b26a8" // Característica única para datos TPMS

// --- Variables para guardar los últimos datos recibidos ---
float presionDelanteraBar  = -1.0; // Usamos -1.0 para indicar que aún no hay lectura válida
int8_t temperaturaDelantera = 0;
float presionTraseraBar    = -1.0;
int8_t temperaturaTrasera   = 0;

// --- Timestamps para el timeout ---
unsigned long lastSeenDelantera = 0;
unsigned long lastSeenTrasera   = 0;
const unsigned long SENSOR_TIMEOUT_MS = 8000; // 8 segundos de timeout

// --- Objetos Globales BLE ---
BLEScan* pBLEScan;
BLECharacteristic* pTpmsChar = NULL;
bool deviceConnected = false;

// --- Clase para manejar la conexión/desconexión del Maestro ---
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Maestro conectado.");
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Maestro desconectado. Reiniciando publicidad...");
      BLEDevice::startAdvertising(); // Volver a ser visible para nuevas conexiones
    }
};

// --- Función para enviar datos al maestro ---
// Esta función empaqueta y envía los datos a través de la característica BLE.
void enviarDatosAlMaestro(uint8_t ruedaID, float presionBar, int8_t temperatura) {
    if (deviceConnected) {
        // --- ESTRUCTURA DEL PAQUETE (6 bytes) ---
        // Byte 0: Identificador de la rueda (1=Delantera, 2=Trasera)
        // Bytes 1-4: Presión en Bar (float)
        // Byte 5: Temperatura en °C (int8_t)
        uint8_t data[6];

        data[0] = ruedaID;
        memcpy(&data[1], &presionBar, sizeof(float)); // Copia los 4 bytes del float
        data[5] = temperatura;

        pTpmsChar->setValue(data, sizeof(data));
        pTpmsChar->notify(); // Envía la notificación al maestro
    }
}


// --- Clase para manejar los resultados del escaneo de sensores TPMS ---
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (advertisedDevice.haveManufacturerData()) {
            String macAddress = advertisedDevice.getAddress().toString().c_str();

            if (macAddress.equalsIgnoreCase(macSensorDelantera) || macAddress.equalsIgnoreCase(macSensorTrasera)) {
                std::string rawData = advertisedDevice.getManufacturerData();

                if (rawData.length() == 7) {
                    // --- LÓGICA DE DECODIFICACIÓN (PSI -> Bar) ---
                    int8_t temperatura = (int8_t)rawData[2];
                    
                    // 1. Obtener el valor crudo del byte 4, que es la presión en PSI.
                    float presionPSI = (float)(uint8_t)rawData[4];
                    
                    // 2. Convertir de PSI a Bar usando el factor proporcionado.
                    float presionBar = presionPSI * PSI_TO_BAR;

                    // --- Actualizamos las variables globales y enviamos al maestro ---
                    if (macAddress.equalsIgnoreCase(macSensorDelantera)) {
                        // Solo actualizamos y enviamos si el dato es nuevo
                        if (presionDelanteraBar != presionBar || temperaturaDelantera != temperatura) {
                           presionDelanteraBar  = presionBar;
                           temperaturaDelantera = temperatura;
                           lastSeenDelantera    = millis();
                           enviarDatosAlMaestro(1, presionDelanteraBar, temperaturaDelantera); // ID 1 para Delantera
                        }
                    } else if (macAddress.equalsIgnoreCase(macSensorTrasera)) {
                        if(presionTraseraBar != presionBar || temperaturaTrasera != temperatura) {
                           presionTraseraBar    = presionBar;
                           temperaturaTrasera   = temperatura;
                           lastSeenTrasera      = millis();
                           enviarDatosAlMaestro(2, presionTraseraBar, temperaturaTrasera); // ID 2 para Trasera
                        }
                    }
                }
            }
        }
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando Gateway TPMS...");

  // --- 1. Configuración del Cliente BLE (para escanear sensores) ---
  BLEDevice::init("ESP32_TPMS_Gateway");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);

  // --- 2. Configuración del Servidor BLE (para enviar datos al Maestro) ---
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Crear una ÚNICA Característica para todos los datos TPMS (Lectura y Notificación)
  pTpmsChar = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TPMS,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                   );
  pTpmsChar->addDescriptor(new BLE2902()); // Necesario para que las notificaciones funcionen

  pService->start(); // Iniciar el servicio

  // Iniciar la publicidad para que el Maestro pueda encontrar este dispositivo
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); 
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Servidor BLE iniciado. Esperando conexión del maestro...");
}

void loop() {
  pBLEScan->start(2, false); // Escanea por 2 segundos
  pBLEScan->clearResults();

  Serial.println("\n--- REPORTE DE SENSORES ---");
  unsigned long ahora = millis();

  // --- Comprobar Timeout Rueda Delantera ---
  if ((ahora - lastSeenDelantera > SENSOR_TIMEOUT_MS) && (presionDelanteraBar != -1.0)) {
      Serial.println("Rueda Delantera: Sin señal (Timeout)");
      presionDelanteraBar = -1.0; // Marcar como inválido
      temperaturaDelantera = 0;
      enviarDatosAlMaestro(1, presionDelanteraBar, temperaturaDelantera); // Notificar al maestro del timeout
  }

  // --- Comprobar Timeout Rueda Trasera ---
  if ((ahora - lastSeenTrasera > SENSOR_TIMEOUT_MS) && (presionTraseraBar != -1.0)) {
      Serial.println("Rueda Trasera:   Sin señal (Timeout)");
      presionTraseraBar = -1.0; // Marcar como inválido
      temperaturaTrasera = 0;
      enviarDatosAlMaestro(2, presionTraseraBar, temperaturaTrasera); // Notificar al maestro del timeout
  }

  // --- Imprimir estado actual en el Monitor Serial ---
  if (presionDelanteraBar == -1.0) {
      Serial.println("Rueda Delantera: Sin señal");
  } else {
      Serial.print("Rueda Delantera: Presión: ");
      Serial.print(presionDelanteraBar, 2); // Imprimir con 2 decimales para Bar
      Serial.print(" Bar | Temperatura: ");
      Serial.print(temperaturaDelantera);
      Serial.println(" C");
  }

  if (presionTraseraBar == -1.0) {
      Serial.println("Rueda Trasera:   Sin señal");
  } else {
      Serial.print("Rueda Trasera:   Presión: ");
      Serial.print(presionTraseraBar, 2); 
      Serial.print(" Bar | Temperatura: ");
      Serial.print(temperaturaTrasera);
      Serial.println(" C");
  }
  
  delay(2000); // Espera antes del próximo ciclo de escaneo
}
