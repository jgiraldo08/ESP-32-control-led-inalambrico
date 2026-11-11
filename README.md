# Control inalámbrico de un LED con ESP32 y Wi-Fi

## Descripción

El ESP32 crea **su propia red Wi-Fi** y publica una página web. Desde el navegador del celular se enciende, apaga o alterna un LED, sin cables ni internet. El LED se apaga solo 10 segundos después de encenderse.

## Objetivo general

Controlar de forma inalámbrica un actuador físico (un LED) mediante solicitudes HTTP, usando un ESP32 como punto de acceso y servidor web, y aplicar un temporizador sin bloquear el programa (`millis()` en lugar de `delay()`).

## Estructura del proyecto

```text
Control-LED-ESP32-WiFi/
├── Control_LED_ESP32_WiFi.ino   Programa del ESP32
├── README.md                    Este archivo
├── TEST_REPORT.md               Informe de pruebas (aprobadas, fallidas y pendientes)
├── .gitignore
└── tests/host/                  Pruebas automáticas en el PC
    ├── test_harness.cpp
    ├── instrucciones.md
    └── mock/                    API de Arduino simulada (solo para las pruebas)
```

## Materiales

| Cantidad | Elemento |
|---|---|
| 1 | ESP32 DevKit (ESP32-WROOM-32) |
| 1 | LED (preferiblemente rojo) |
| 1 | Resistencia de 220 Ω |
| 1 | Protoboard |
| 2–3 | Cables Dupont |
| 1 | Cable USB **de datos** |
| 1 | Computador con Arduino IDE |
| 1 | Celular con Wi-Fi |

## Conexiones

```text
GPIO23 del ESP32 --- [ 220 Ω ] --- |>| --- GND del ESP32
                                    LED
                             ánodo -> cátodo
```

| Desde | Hasta |
|---|---|
| GPIO23 (`IO23`) | Una pata de la resistencia de 220 Ω |
| Otra pata de la resistencia | Pata **larga** del LED (ánodo, +) |
| Pata **corta** del LED (cátodo, −) | GND |

1. Desconecta el USB antes de montar.
2. Verifica que el LED esté en serie con la resistencia y que no haya un cable directo entre GPIO23 y GND.
3. Conecta el USB al terminar.

Con 220 Ω y un LED rojo (≈ 2,0 V) circulan unos 5,9 mA: `(3,3 V − 2,0 V) / 220 Ω`. **Nunca** conectes el LED sin resistencia ni apliques 5 V a un GPIO.

## Configuración de Arduino IDE

1. Instala **Arduino IDE 2.x**.
2. Abre **Archivo > Preferencias** y, en *URLs adicionales para el Gestor de tarjetas*, agrega:
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. En **Herramientas > Placa > Gestor de tarjetas** busca `esp32 by Espressif Systems` e instala la versión estable.
4. Conecta el ESP32 con un cable USB de datos.
5. Selecciona la placa **ESP32 Dev Module** y el **puerto** correspondiente.
6. Abre `Control_LED_ESP32_WiFi.ino`. Si Arduino IDE avisa que *el boceto debe estar en una carpeta con el mismo nombre*, acepta que la cree (la carpeta se llamará `Control_LED_ESP32_WiFi`). Es un requisito de Arduino: el nombre de la carpeta debe coincidir con el del `.ino`, y la carpeta de GitHub se llama con guiones.
7. Pulsa **Verificar** y luego **Subir**. Si no sube, mantén presionado **BOOT** al iniciar la carga.
8. Abre el **Monitor serie a 115200 baudios**.

### Librerías

Solo `WiFi.h` y `WebServer.h`, que **ya vienen incluidas** en el paquete `esp32` de Espressif. No hay que instalar nada más.

## Red Wi-Fi e IP

| Dato | Valor |
|---|---|
| Nombre de la red (SSID) | `ESP32_LED_AULA` |
| Contraseña | `Mecatronica2026` |
| Dirección IP | `192.168.4.1` |
| Dirección web | `http://192.168.4.1/` |

El SSID, la clave y el pin se cambian en las constantes del inicio del `.ino`. La clave debe tener **8 o más caracteres**; si no, la red no se crea. Si varios grupos trabajan en la misma aula, usa un SSID distinto por equipo (por ejemplo `ESP32_LED_G01`).

## Instrucciones de funcionamiento

1. Alimenta el ESP32 por USB. El LED arranca apagado y el monitor serie muestra el nombre de la red y la IP.
2. En el celular entra a **Configuración > Wi-Fi** y conéctate a `ESP32_LED_AULA`.
3. Si el celular avisa «Sin internet», **permanece conectado**: es normal, la red es local.
4. Abre el navegador y escribe `http://192.168.4.1/` (con `http://`, no `https://`).
5. Usa los botones:

| Botón | Qué hace |
|---|---|
| **Encender** | Enciende el LED e inicia (o reinicia) el plazo de 10 s |
| **Apagar** | Apaga el LED |
| **Alternar** | Invierte el estado actual |

6. Diez segundos después de encenderse, el LED se apaga solo. La página se recarga cada 2 s, así que el texto pasa a APAGADO sin tocar nada.

Si abres `/encender`, `/apagar` o `/alternar` escribiendo la dirección en el navegador, el ESP32 responde «405 Método no permitido» y no cambia el LED: las órdenes solo se aceptan desde los botones (método POST). Una ruta que no existe responde 404.

## Funciones del programa

| Función | Qué hace |
|---|---|
| `setup()` | Abre el puerto serie, deja el LED apagado, crea la red con `WiFi.softAP()`, registra las rutas e inicia el servidor. Si la red falla, informa el error y deja el programa en reposo. |
| `loop()` | Atiende al navegador con `handleClient()` y revisa el autoapagado en cada vuelta. |
| `fijarLED(encender)` | Único punto que escribe en el GPIO. Mantiene `ledEncendido` igual al pin y guarda `millis()` al encender. |
| `revisarAutoapagado()` | Si el LED está encendido y pasaron 10 s, lo apaga. Solo compara tiempos: no bloquea. |
| `mostrarPagina()` | `GET /`: envía el HTML con el estado actual y los tres botones. |
| `encenderLED()` | `POST /encender` |
| `apagarLED()` | `POST /apagar` |
| `alternarLED()` | `POST /alternar` |
| `redirigirAlInicio()` | Responde 303 para que el navegador vuelva a `/` y muestre el estado nuevo. |
| `rutaNoValida()` | Responde 405 (ruta existente con método incorrecto) o 404 (ruta inexistente). |

**Sobre `millis()`:** devuelve los milisegundos desde que arrancó el ESP32. Al encender se guarda ese valor; en cada vuelta de `loop()` se comprueba si `millis() - instanteEncendido >= 10000`. Así el servidor sigue atendiendo solicitudes durante los 10 s, a diferencia de `delay(10000)`, que detendría todo el programa.

## Pruebas

- **Pruebas automáticas en el PC:** `tests/host/instrucciones.md`. Verifican la lógica del sketch real (arranque, rutas HTTP, botones, temporizador, errores).
- **Informe:** [`TEST_REPORT.md`](TEST_REPORT.md) separa lo aprobado, lo fallido y lo **pendiente**: la compilación real para ESP32 y las pruebas con la placa aún no se han realizado.

## Seguridad y límites

Solo para cargas de bajo consumo como un LED: no conectes motores, relés ni tiras LED directamente al GPIO. El servidor usa HTTP sin cifrado en una red local; cualquier dispositivo con la contraseña puede usar los botones. No lo expongas a internet.
