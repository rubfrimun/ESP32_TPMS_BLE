# Ingeniería Inversa de Sensores TPMS (BLE) con ESP32

![Montaje de prueba con jeringa](docs/jeringa_setup.jpg)

Este repositorio documenta el proceso de ingeniería inversa para decodificar la trama de datos de sensores TPMS (Tire Pressure Monitoring System) que se comunican vía Bluetooth Low Energy (BLE). El objetivo es poder leer la presión, temperatura y otros datos de los sensores utilizando un microcontrolador ESP32 en lugar de la unidad receptora propietaria.

---

### 🛠️ Hardware Utilizado
*   **Microcontrolador:** Placa de desarrollo ESP32.
*   **Sensores:** 2x Sensores TPMS externos con conectividad BLE.
*   **Herramientas de prueba:** Una jeringa de 60 ml y una válvula de neumático para crear una cámara de presión variable.

---

### Parte 1: ¿Qué son los Sensores TPMS y cómo funcionan?

Los TPMS son pequeños dispositivos instalados en las válvulas de los neumáticos que monitorizan en tiempo real la presión y la temperatura. Su principal función es alertar al conductor de condiciones inseguras, como una llanta baja.

Estos sensores en particular utilizan **BLE (Bluetooth Low Energy)** para transmitir sus datos. No establecen una conexión permanente, sino que emiten periódicamente un paquete de datos llamado **"Advertising Packet"**. Este paquete puede ser capturado por cualquier dispositivo BLE cercano, como nuestro ESP32.

![Sensores utilizados en el proyecto](docs/sensores.jpg)

---

### Parte 2: El Desafío - El Protocolo del Fabricante

El fabricante de los sensores proporciona un "mapa de bytes" que describe la estructura de los datos que envían. Sin embargo, esta información debe ser verificada y decodificada correctamente.

Este es el mapa de la trama de datos de 7 bytes:

![Mapa de Bytes del Fabricante](docs/byte_map.png)

Nuestro objetivo es escribir un programa que capture esta trama y traduzca cada campo a un valor legible (presión en PSI, temperatura en °C, etc.).

---

### Parte 3: El Experimento - Simulación de Presión

Para verificar qué bytes corresponden a la presión, era necesario variar la presión del sensor de forma controlada. Para ello, se construyó un sistema de prueba simple y efectivo:
1.  Se acopló una válvula de neumático a la punta de una jeringa de 60 ml.
2.  Se enroscó el sensor TPMS a la válvula.
3.  Al presionar el émbolo de la jeringa, se aumenta la presión del aire dentro del sistema, simulando el inflado de un neumático.

Este montaje nos permite observar en tiempo real cómo cambian los bytes de la trama de datos a medida que ejercemos presión.

---

### Parte 4: El Código de Diagnóstico y el Análisis

Para capturar y analizar los datos, se desarrolló un script para el ESP32. Este código no intenta decodificar la trama; su única función es:
1.  Escanear dispositivos BLE cercanos.
2.  Filtrar por las direcciones MAC de nuestros dos sensores TPMS.
3.  Imprimir la trama de datos cruda (7 bytes) en formato hexadecimal y decimal para su análisis.

```cpp
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
                        Serial.printf("[%02X] ", (uint8_t)rawDataStr[i]);
                    }
                    Serial.println();

                    // 2. Imprimimos los bytes centrales en DECIMAL
                    Serial.print("Bytes Centrales (DEC): ");
                    for (int i = 1; i < rawDataStr.length() - 2; i++) {
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
```

Al ejecutar este código mientras se manipulaba la jeringa, se observó que los bytes en los índices `3` y `4` (`PPPP` en el mapa) variaban consistentemente con la presión aplicada, confirmando la documentación del fabricante.

---

### 🚀 Próximos Pasos
Con la trama de datos ya decodificada, los siguientes pasos podrían ser:
-   Desarrollar un firmware completo que traduzca los bytes a valores humanos (PSI, °C, Voltios).
-   Mostrar la información en una pantalla OLED o en una aplicación móvil.
-   Crear un sistema de alertas para presiones bajas o altas.

---

### Licencia
Este proyecto se distribuye bajo la **Licencia MIT**. Ver el archivo `LICENSE` para más detalles.
