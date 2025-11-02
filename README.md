# Ingeniería Inversa de Sensores TPMS (BLE) con ESP32

![Montaje de prueba con jeringa](sensores_1)

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

![Sensores utilizados en el proyecto](simulacion_1)

---

### Parte 2: El Desafío - El Protocolo del Fabricante

El fabricante de los sensores proporciona un "mapa de bytes" que describe la estructura de los datos que envían. Sin embargo, esta información debe ser verificada y decodificada correctamente.

Este es el mapa de la trama de datos de 7 bytes:

![Mapa de Bytes del Fabricante](tramas_tpms_ble)

Nuestro objetivo es escribir un programa que capture esta trama y traduzca cada campo a un valor legible (presión en PSI, temperatura en °C, etc.).

---

### Parte 3: El Experimento - Simulación de Presión

Para verificar qué bytes corresponden a la presión, era necesario variar la presión del sensor de forma controlada. Para ello, se construyó un sistema de prueba simple y efectivo:
1.  Se acopló una válvula de neumático a la punta de una jeringa de 60 ml.
2.  Se enroscó el sensor TPMS a la válvula.
3.  Al presionar el émbolo de la jeringa, se aumenta la presión del aire dentro del sistema, simulando el inflado de un neumático.

Este montaje nos permite observar en tiempo real cómo cambian los bytes de la trama de datos a medida que ejercemos presión.

---

### Parte 4: El Código de diagnóstico

Para capturar y analizar los datos, se desarrolló un script para el ESP32. Este código no intenta decodificar la trama; su única función es:
1.  Escanear dispositivos BLE cercanos.
2.  Filtrar por las direcciones MAC de nuestros dos sensores TPMS.
3.  Imprimir la trama de datos cruda (7 bytes) en formato hexadecimal y decimal para su análisis.


### Parte 5. Análisis de los datos obtenidos.
Al ejecutar este código mientras se manipulaba la jeringa, se observó que los bytes en los índices `3` y `4` (`PPPP` en el mapa) variaban consistentemente con la presión aplicada, confirmando la documentación del fabricante. Sin embargo, no estaba claro cuál correspondia a presión y temperatura. Para ello, convertimos los bytes a decimal, una vez en decimal, podemos observar lo siguiente.

- Byte 1, status, siempre toma el mismo valor.
- Byte 2, muestra el valor de la bateria en formato 1/10 V
- Byte 3, toma el valor de temperatura en ºC
- Byte 4, según el fabricante conjunto al byte 5 forman la presión, pero mantiene el mismo valor, en este caso 01
- Byte 5, valor de presión en 1/10 psi
- Byte 6 y 7, son bytes que nos mandan mensajes de checksend, se puede entender por un final de trama, no siempre es el mismo valor 

---

### Parte 6. Volcado de datos.
Con la trama de datos ya decodificada, los siguientes pasos son:
-   Desarrollar un firmware completo que traduzca los bytes a valores de PSI Bar, °C, Voltios.
-   Crear un sistema de alertas para presiones bajas o altas.
-   Monitoreo para que siempre muestre los valores a pesar de ser repetidos.

---

### Licencia
Este proyecto se distribuye bajo la **Licencia MIT**. Ver el archivo `LICENSE` para más detalles.
