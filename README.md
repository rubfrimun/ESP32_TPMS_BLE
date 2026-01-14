# 🔬 Ingeniería Inversa de Sensores TPMS (BLE) con ESP32

> Este repositorio documenta el proceso de ingeniería inversa para decodificar la trama de datos de sensores TPMS (Tire Pressure Monitoring System) que se comunican vía Bluetooth Low Energy (BLE). El objetivo es poder leer la presión, temperatura y otros datos de los sensores utilizando un microcontrolador ESP32.

<p align="center">
  <img src="simulacion_2.jpg" alt="Montaje de prueba con jeringa y sensor TPMS" width="400">
</p>

---

###  Hardware utilizado
*   **Microcontrolador:** Placa de desarrollo ESP32.
*   **Sensores:** 2x Sensores TPMS externos con conectividad BLE.
*   **Herramientas de prueba:** Una jeringa de 60 ml y una válvula de neumático para crear una cámara de presión variable.

---

### Parte 1: ¿Qué son los sensores TPMS y cómo funcionan?

Los TPMS son pequeños dispositivos instalados en las válvulas de los neumáticos que monitorizan en tiempo real la presión y la temperatura. Su principal función es alertar al conductor de condiciones inseguras.

Estos sensores en particular utilizan **BLE (Bluetooth Low Energy)** para emitir periódicamente un paquete de datos llamado **"Advertising Packet"**, que puede ser capturado por cualquier dispositivo BLE cercano, como nuestro ESP32.

<p align="center">
  <img src="sensores_2.jpg" alt="Sensores TPMS utilizados en el proyecto" width="400">
</p>

---

### Parte 2: El sesafío - El protocolo del fabricante

El fabricante proporciona un "mapa de bytes" que describe la estructura de los datos. Sin embargo, esta información debe ser verificada y decodificada correctamente para ser útil.

<p align="left">
  <img src="tramas_tpms_ble.jpg" alt="Mapa de bytes del fabricante" width="700">
</p>

---

### Parte 3: El experimento - Simulación de presión

Para verificar qué bytes corresponden a la presión, se construyó un sistema de prueba simple y efectivo:
1.  Se acopló una válvula de neumático a la punta de una jeringa de 60 ml.
2.  Se enroscó el sensor TPMS a la válvula.
3.  Al presionar el émbolo, se aumenta la presión del aire, permitiendo observar en tiempo real cómo cambian los bytes de la trama de datos.

---

### Parte 4: El código de diagnóstico

Para capturar y analizar los datos, se desarrolló un script para el ESP32. Su única función es:
1.  Escanear dispositivos BLE cercanos.
2.  Filtrar por las direcciones MAC de nuestros dos sensores TPMS.
3.  Imprimir la trama de datos cruda (7 bytes) en formato hexadecimal para su análisis posterior.

>  **El código fuente completo para este escáner de diagnóstico se encuentra en el archivo [`tpms_diagnostic_scanner.ino`](tpms_diagnostic_scanner.ino).**

---

### Parte 5: 📈 Análisis de los datos obtenidos

Al ejecutar el código mientras se manipulaba la jeringa, se pudieron confirmar y clarificar las funciones de cada byte de la trama.

| Byte (Índice) | Campo (Fabricante) | Observación y Conclusión                                     |
|:-------------:|:------------------:|--------------------------------------------------------------|
|       0       |        `SS`        | **Status:** Mantiene un valor constante. Probablemente flags. |
|       1       |        `BB`        | **Batería:** Su valor corresponde al voltaje en 1/10 V.      |
|       2       |        `TT`        | **Temperatura:** Su valor decimal es la temperatura en °C.   |
|       3       |      `PPPP` (Alto) | Mantiene un valor constante (0x01). No parece ser presión.     |
|       4       |      `PPPP` (Bajo) | **Presión:** Varía con la presión. Su valor es la presión en 1/10 PSI.|
|      5, 6     |       `CCCC`       | **Checksum:** Varían. Probablemente para validar la integridad de la trama. |

**Conclusión clave:** El campo de presión `PPPP` de 16 bits en realidad no se usa como tal. La presión real se encuentra únicamente en el byte de índice 4.

---

### Parte 6:  Conclusiones y próximos pasos

Con la trama de datos ya decodificada, el camino está claro para desarrollar una aplicación completa:
-   **Firmware final:** Crear un programa que traduzca los bytes a valores legibles (PSI, Bar, °C, Voltios) y los muestre en una pantalla.
-   **Sistema de Alertas:** Implementar alarmas para notificar al usuario de presiones peligrosamente bajas o altas.
-   **Monitoreo Constante:** Diseñar la lógica para que los valores se actualicen en pantalla de forma continua y fiable.

---

### Licencia
Este proyecto se distribuye bajo la **Licencia MIT**. Ver el archivo `LICENSE` para más detalles.
