# Pruebas automáticas en el PC

`test_harness.cpp` compila el sketch **real** (`../../Control_LED_ESP32_WiFi.ino`) en tu computador, sustituyendo el ESP32 por una versión simulada de la API de Arduino (carpeta `mock/`). Simula solicitudes HTTP, el reloj `millis()` y el pin del LED, y comprueba el comportamiento del programa.

## Requisitos

Un compilador de C++17: `g++` en Linux y macOS (en macOS se instala con `xcode-select --install`), o MSYS2/MinGW en Windows. No necesitas la placa ni Arduino IDE.

## Ejecutar

Desde la raíz del proyecto:

```bash
cd tests/host
g++ -std=c++17 -Wall -Wextra -I mock test_harness.cpp -o test_harness
./test_harness
```

En Windows (MinGW) el ejecutable se llama `test_harness.exe`.

## Resultado esperado

Una línea por prueba y un resumen final. El programa termina con código 0 si todo pasa.

```text
[OK   ] T2   A los 10 s el LED se apaga solo
...
Resultado: 39/39 pruebas superadas, 0 fallidas
```

Una línea `[FALLO]` indica qué comportamiento dejó de cumplirse.

## Qué prueban

| Grupo | Contenido |
|---|---|
| E | Revisión estática del código fuente (librerías, WIFI_AP, GPIO 23, sin `delay()`, uso de `millis()`) |
| A | Arranque: GPIO como salida, LED apagado, red, servidor, IP y rutas |
| P | Página web: tipo HTML, tres botones POST, estado, recarga automática |
| C | Encender, apagar y alternar; la variable `ledEncendido` coincide con el pin |
| T | Autoapagado a los 10 s, reinicio del plazo y desbordamiento de `millis()` |
| H | Errores HTTP: 404, 405 y que no cambien el LED |
| R | Fallo al crear la red Wi-Fi |
| G | Ninguna llamada a `delay()` en toda la ejecución |

## Qué NO prueban

No sustituyen la compilación con el paquete ESP32 ni las pruebas con la placa (Wi-Fi real, cableado, navegador del celular). Esas están en `TEST_REPORT.md`, sección «Pruebas físicas pendientes».

## Agregar una prueba

Dentro de `main()` usa `pedir("POST", "/ruta")` para simular una solicitud, `mock_ms = ...; loop();` para avanzar el tiempo y `verificar(condicion, "ID", "descripcion")` para registrar el resultado.
