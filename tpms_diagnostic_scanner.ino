// TPMS - CÓDIGO DE DIAGNÓSTICO
// Lee la trama de datos crudos y convierte cada byte a decimal,
// excepto el primer byte (índice 0) y los dos últimos.

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// --- Direcciones MAC de tus sensores ---
const char* macSensorDelantera = "4b:a1:00:00:7b:3a";
const char* macSensorTrasera   = "4b:9c:00:00:a1:13";

int scanTime = 3;
BLEScan* pBLEScan;

// Clase para manejar los resultados del escaneo
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (advertisedDevice.haveManufacturerData()) {
            String macAddress = advertisedDevice.getAddress().toString().c_str();

            if (macAddress.equalsIgnoreCase(macSensorDelantera) || macAddress.equalsIgnoreCase(macSensorTrasera)) {
                
                String rawDataStr = advertisedDevice.getManufacturerData().c_str();

                // Asegurarnos de que tenemos la longitud de datos esperada (7 bytes)
                if (rawDataStr.length() == 7) {
                    Serial.print("Sensor Encontrado: ");
                    Serial.println(macAddress);

                    // 1. Imprimimos la trama completa en HEX para referencia
                    Serial.print("Trama Cruda (HEX):    ");
                    for (int i = 0; i < rawDataStr.length(); i++) {
                        // %02X formatea el número como hexadecimal con 2 dígitos (ej: 0F, 1A)
                        Serial.printf("[%02X] ", (uint8_t)rawDataStr[i]);
                    }
                    Serial.println();

                    // 2. Imprimimos los bytes centrales en DECIMAL como solicitaste
                    Serial.print("Bytes Centrales (DEC): ");
                    // El bucle empieza en 1 (salta el primer byte)
                    // y termina antes de length() - 2 (salta los dos últimos bytes)
                    // Para una trama de 7 bytes, procesará los índices: 1, 2, 3, 4
                    for (int i = 1; i < rawDataStr.length() - 2; i++) {
                        // (uint8_t) asegura que el byte se trate como un número de 0 a 255
                        Serial.printf("[Byte %d: %d] ", i, (uint8_t)rawDataStr[i]);
                    }
                    Serial.println();
                }
            }
        }
    }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando Escáner TPMS en MODO DIAGNÓSTICO...");
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}

void loop() {
  pBLEScan->start(scanTime, false);
  pBLEScan->clearResults();
  delay(1000);
}
