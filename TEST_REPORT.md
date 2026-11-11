# Informe de pruebas

**Proyecto:** Control inalámbrico de un LED con ESP32 y Wi-Fi
**Fecha de las pruebas:** 9 de noviembre de 2026
**Entorno:** g++ 13.3.0 (Ubuntu 24.04), sin compilador de ESP32, sin acceso a internet y **sin placa ESP32**.

> **Importante.** Todo lo marcado como «Aprobada» se verificó en un PC con una simulación de la API de Arduino. **No se realizó ninguna prueba física y no se compiló con el paquete oficial del ESP32.** Esas pruebas aparecen como pendientes en la sección 6.

## 1. Resumen

| Prueba | Estado | Detalle |
|---|---|---|
| Revisión estática manual del código | Aprobada | 5 hallazgos corregidos (sección 4) |
| Compilación en PC con `-Wall -Wextra -Wpedantic` | Aprobada | Sin advertencias |
| Pruebas automáticas de lógica (39) | Aprobada | 39 superadas, 0 fallidas |
| Pruebas de mutación (9 errores introducidos a propósito) | Aprobada | Las 9 versiones con errores fueron detectadas |
| Compilación real para ESP32 (Arduino IDE / arduino-cli) | **Pendiente** | No había herramientas disponibles |
| Pruebas físicas con la placa y el celular | **Pendiente** | No hay placa; ver sección 6 |
| Pruebas fallidas | Ninguna | |

## 2. Requisitos revisados

| Requisito | Evidencia | Estado |
|---|---|---|
| Compatibilidad con ESP32 DevKit / WROOM-32 | Usa solo `WiFi.h`, `WebServer.h` y funciones estándar de Arduino-ESP32 (E1). Revisión de código, no compilación. | Revisado; compilación **pendiente** (F1) |
| GPIO 23 como salida, LED apagado al inicio | E3, A1, A2 | Aprobada (lógica). Que GPIO23 sea adecuado en tu placa debe confirmarse con su mapa de pines; no se consultó en esta sesión. |
| Red propia con `WIFI_AP` | E2, A3, A4 | Aprobada (lógica); la red real queda **pendiente** (F4) |
| Servidor web | A5, A7, P1 | Aprobada (lógica) |
| Página accesible desde el celular | P1–P4 (HTML con *viewport*, recarga y sin caché) | Aprobada (contenido); acceso real **pendiente** (F5, F6) |
| Botón Encender | C1–C3, C7 | Aprobada (lógica) |
| Botón Apagar | C4, C5, C7 | Aprobada (lógica) |
| Botón Alternar | C6, C7 | Aprobada (lógica) |
| Autoapagado a los 10 s | T1, T2, T4–T8 | Aprobada con reloj simulado; tiempo real **pendiente** (F10, F11) |
| `millis()` sin bloqueos con `delay()` | E4, E5, G1, T7 | Aprobada |
| Estado actualizado en la página | C2, C4, T3, P4 | Aprobada (contenido); comportamiento del navegador **pendiente** (F10) |
| Errores y solicitudes HTTP | H1–H5, R1, R2 | Aprobada (lógica); respuesta real de la biblioteca **pendiente** (F13) |

## 3. Resultado de las pruebas automáticas

Comando: `g++ -std=c++17 -Wall -Wextra -Wpedantic -I mock test_harness.cpp` y ejecución del resultado (ver `tests/host/instrucciones.md`).

```text
[OK   ] E1   Usa solo las librerias WiFi.h y WebServer.h del paquete ESP32
[OK   ] E2   Crea la red con WIFI_AP y softAP()
[OK   ] E3   El LED esta en el GPIO 23
[OK   ] E4   El codigo no contiene ninguna llamada a delay()
[OK   ] E5   El temporizador usa millis()
[OK   ] A1   GPIO 23 configurado como salida
[OK   ] A2   Al arrancar el LED esta apagado
[OK   ] A3   Modo Wi-Fi: punto de acceso (WIFI_AP)
[OK   ] A4   softAP() con SSID ESP32_LED_AULA y clave de 8+ caracteres
[OK   ] A5   El servidor HTTP queda iniciado
[OK   ] A6   El monitor serie informa el nombre de la red y la IP
[OK   ] A7   Rutas registradas: GET /, POST /encender, /apagar, /alternar y onNotFound
[OK   ] P1   GET / responde 200 con tipo text/html
[OK   ] P2   La pagina inicial muestra APAGADO y no deja marcadores sin reemplazar
[OK   ] P3   Hay tres botones (Encender, Apagar, Alternar) que envian POST
[OK   ] P4   Pagina movil, con recarga automatica y sin cache
[OK   ] C1   POST /encender: GPIO en HIGH y redireccion 303 a /
[OK   ] C2   La pagina muestra ENCENDIDO
[OK   ] C3   Encender dos veces seguidas mantiene el LED encendido
[OK   ] C4   POST /apagar: GPIO en LOW y la pagina muestra APAGADO
[OK   ] C5   Apagar con el LED ya apagado no cambia nada
[OK   ] C6   POST /alternar x3: encendido, apagado, encendido
[OK   ] C7   200 ordenes mezcladas: la variable y el pin siempre coinciden
[OK   ] T1   A los 9,999 s el LED sigue encendido
[OK   ] T2   A los 10 s el LED se apaga solo
[OK   ] T3   Despues del autoapagado la pagina muestra APAGADO
[OK   ] T4   Apagar antes de los 10 s funciona y el LED no se reenciende
[OK   ] T5   Encender de nuevo antes del vencimiento reinicia los 10 s
[OK   ] T6   Alternar a encendido tambien inicia un plazo nuevo de 10 s
[OK   ] T7   El autoapagado funciona aunque millis() se desborde
[OK   ] T8   Con el LED apagado, loop() no lo vuelve a encender
[OK   ] H1   Rutas que no existen responden 404
[OK   ] H2   GET /encender responde 405 e indica Allow: POST
[OK   ] H3   GET /apagar y GET /alternar responden 405
[OK   ] H4   POST / responde 405 e indica Allow: GET
[OK   ] H5   Las solicitudes invalidas no cambian el LED
[OK   ] R1   Si falla la red: servidor sin iniciar y LED apagado
[OK   ] R2   Si falla la red se informa el error por el monitor serie
[OK   ] G1   Durante todas las pruebas no se llamo a delay()

Resultado: 39/39 pruebas superadas, 0 fallidas
```

## 4. Hallazgos de la revisión y correcciones

| # | Hallazgo | Corrección |
|---|---|---|
| 1 | El código original no tenía el botón Alternar ni el autoapagado | Se agregaron `alternarLED()`, `revisarAutoapagado()` y la ruta `POST /alternar` |
| 2 | La página es estática: tras el autoapagado el celular seguiría mostrando ENCENDIDO hasta recargar | Se agregó `<meta http-equiv="refresh" content="2">` y `Cache-Control: no-store` |
| 3 | Si fallaba la creación de la red, el programa quedaba en un bucle con `delay(1000)` | Se usa la bandera `redActiva`: informa el error y `loop()` queda en reposo, sin `delay()` en todo el código |
| 4 | Las rutas inválidas dependían del 404 genérico de la biblioteca | Se agregó `rutaNoValida()`: 405 con `Allow` si la ruta existe con otro método, 404 si no existe |
| 5 | El temporizador usaba `unsigned long` (32 bits en ESP32, pero 64 bits en un PC) | Se usa `uint32_t` para que la resta funcione igual al desbordarse `millis()` (cada ~49 días); probado en T7 |

Nota sobre la estructura: la carpeta del proyecto se llama `Control-LED-ESP32-WiFi` y el boceto `Control_LED_ESP32_WiFi.ino`. Arduino IDE exige que carpeta y `.ino` se llamen igual, por lo que pedirá crear una carpeta `Control_LED_ESP32_WiFi`. No afecta al funcionamiento (ver README).

## 5. Pruebas de mutación

Para comprobar que las pruebas detectan errores reales, se copió el sketch nueve veces introduciendo un fallo en cada copia. Todas fueron detectadas.

| Error introducido | Pruebas fallidas | Cuáles |
|---|---|---|
| plazo de 5 s en vez de 10 s | 4 | T1, T5, T6, T7 |
| encender de nuevo no reinicia el plazo | 1 | T5 |
| contador de 64 bits (rompe el desbordamiento) | 1 | T7 |
| HIGH y LOW invertidos | 11 | C1, C3, C4, C5, C6, C7, T1, T2, T4, T8, H5 |
| usa delay() en loop | 6 | E4, T1, T5, T6, T7, G1 |
| sin onNotFound | 4 | A7, H2, H3, H4 |
| alternar siempre enciende | 2 | C6, C7 |
| no se llama al autoapagado en loop | 6 | T2, T3, T5, T6, T7, T8 |
| ruta existente con metodo erroneo da 404 | 2 | H2, H3 |

## 6. Pruebas físicas pendientes (para hacer con tu ESP32)

Marca el resultado de cada una.

| ID | Prueba | Resultado esperado | Resultado |
|---|---|---|---|
| F1 | **Verificar** (compilar) el `.ino` en Arduino IDE con `ESP32 Dev Module` | Compila sin errores | ☐ |
| F2 | Subir el programa; abrir el monitor serie a 115200 | Muestra «Red Wi-Fi creada correctamente», el nombre, `192.168.4.1` y «Servidor HTTP listo» | ☐ |
| F3 | Reiniciar el ESP32 (botón EN o reconectar USB) | El LED queda apagado | ☐ |
| F4 | Buscar redes Wi-Fi en el celular y conectarse con la clave | Aparece `ESP32_LED_AULA` y conecta | ☐ |
| F5 | Abrir `http://192.168.4.1/` (el celular puede avisar «Sin internet») | Carga la página | ☐ |
| F6 | Revisar la página en la pantalla del celular | Se ve el estado y los tres botones sin desbordes | ☐ |
| F7 | Pulsar Encender | El LED se ilumina; el texto dice ENCENDIDO; el monitor serie registra la orden | ☐ |
| F8 | Pulsar Apagar | El LED se apaga; el texto dice APAGADO | ☐ |
| F9 | Pulsar Alternar varias veces | El LED invierte su estado cada vez | ☐ |
| F10 | Encender y cronometrar sin tocar nada | El LED se apaga a los ~10 s (±1 s) y la página pasa a APAGADO en ≤ 2 s | ☐ |
| F11 | Encender, pulsar Encender de nuevo a los ~6 s | Se apaga ~10 s después del **segundo** encendido | ☐ |
| F12 | Encender y pulsar Apagar antes de los 10 s | Se apaga y no se vuelve a encender | ☐ |
| F13 | Escribir en el navegador `http://192.168.4.1/encender`, y luego `http://192.168.4.1/xyz` | `/encender` responde 405 y no cambia el LED; `/xyz` responde 404. *Se asume que la biblioteca llama a `onNotFound` cuando la ruta existe con otro método; si la respuesta difiere, el control del LED no se ve afectado.* | ☐ |
| F14 | Revisar el circuito (LED con resistencia de 220 Ω en GPIO23) | El LED brilla con normalidad y no se calienta. Si no enciende, revisar polaridad y conexiones | ☐ |
| F15 | Conectar dos celulares a la vez | Ambos ven y controlan el LED (el punto de acceso admite pocos clientes simultáneos; por defecto 4) | ☐ |
| F16 | Dejar el ESP32 encendido varios minutos | Sigue respondiendo | ☐ |
| F17 | Si el celular no abre la página, desactivar los datos móviles | Algunos teléfonos cambian a datos al detectar una red sin internet; con datos desactivados abre la página | ☐ |

## 7. Limitaciones de las pruebas en PC

La simulación cubre la **lógica** del programa. No cubre:

- Que el código compile con el paquete oficial del ESP32 (los nombres y firmas de la API se escribieron según la API conocida, sin verificarlos contra el compilador).
- La radio Wi-Fi, el reparto real de IP ni el comportamiento de `WebServer` con navegadores reales o varias conexiones.
- El cableado, el brillo y la corriente del LED.
- Los tiempos reales: el reloj es simulado.

## 8. Conclusión

La lógica del programa funciona según lo esperado en la simulación y las pruebas detectan errores introducidos a propósito. **Falta compilar con Arduino IDE (F1) y completar las pruebas físicas (F2–F17) antes de afirmar que el proyecto funciona en la placa.**
