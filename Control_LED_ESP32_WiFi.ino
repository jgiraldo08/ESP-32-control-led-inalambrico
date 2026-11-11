/*
  Control_LED_ESP32_WiFi.ino
  Control inalambrico de un LED con un ESP32 (DevKit / ESP32-WROOM-32).

  El ESP32 crea su propia red Wi-Fi (modo punto de acceso) y publica una pagina
  web con tres botones: Encender, Apagar y Alternar. El LED esta conectado al
  GPIO 23 (con una resistencia de 220 ohmios) y se apaga solo 10 segundos
  despues de encenderse.

  Librerias: WiFi y WebServer, incluidas en el paquete "esp32" de Espressif.
  No hace falta instalar nada mas ni tener internet.
*/

#include <WiFi.h>
#include <WebServer.h>

// ---------- Configuracion (puedes cambiar estos valores) ----------
const char* NOMBRE_RED = "ESP32_LED_AULA";   // Nombre de la red Wi-Fi (SSID)
const char* CLAVE_RED  = "Mecatronica2026";  // Minimo 8 caracteres
const int   PIN_LED    = 23;                 // GPIO del LED (ESP32 clasico)

// Tiempo que el LED permanece encendido antes de apagarse solo.
// Si lo cambias, actualiza tambien el texto de la nota en mostrarPagina().
const uint32_t TIEMPO_AUTOAPAGADO_MS = 10000;

// ---------- Estado del programa ----------
WebServer servidor(80);            // Servidor HTTP en el puerto 80
bool ledEncendido = false;         // Estado logico del LED (lo que muestra la pagina)
uint32_t instanteEncendido = 0;    // Valor de millis() cuando se encendio el LED
bool redActiva = false;            // true si el punto de acceso se creo bien

// ---------- Control del LED ----------

// Unico lugar donde se escribe en el GPIO: asi la variable y el pin nunca
// quedan desincronizados. Al encender se guarda la hora para el autoapagado
// (encender de nuevo reinicia los 10 segundos).
void fijarLED(bool encender) {
  ledEncendido = encender;
  digitalWrite(PIN_LED, encender ? HIGH : LOW);
  if (encender) {
    instanteEncendido = millis();
  }
}

// Apaga el LED si ya pasaron 10 s desde que se encendio. Se llama en cada
// vuelta de loop() y solo compara tiempos: no bloquea el servidor.
// La resta con uint32_t funciona bien incluso cuando millis() reinicia a 0
// (ocurre aproximadamente cada 49 dias).
void revisarAutoapagado() {
  if (ledEncendido && (millis() - instanteEncendido >= TIEMPO_AUTOAPAGADO_MS)) {
    fijarLED(false);
    Serial.println("Autoapagado: pasaron 10 s");
  }
}

// ---------- Respuestas HTTP ----------

// Despues de una orden se redirige a la pagina principal (HTTP 303) para que
// el navegador la pida de nuevo y muestre el estado actualizado.
void redirigirAlInicio() {
  servidor.sendHeader("Location", "/");
  servidor.send(303, "text/plain", "");
}

// GET /  -> pagina con el estado y los botones.
// La etiqueta <meta http-equiv="refresh"> recarga la pagina cada 2 s para que
// el texto refleje tambien el autoapagado.
void mostrarPagina() {
  String pagina = R"HTML(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="refresh" content="2">
  <title>Control LED ESP32</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; margin: 40px; background: #f5f7fb; }
    button { font-size: 20px; padding: 15px 25px; margin: 8px; cursor: pointer; }
    .nota { color: #555; font-size: 14px; }
  </style>
</head>
<body>
  <h1>Control de LED por Wi-Fi</h1>
  <p>Estado actual: <strong>%ESTADO%</strong></p>
  <form action="/encender" method="POST"><button type="submit">Encender</button></form>
  <form action="/apagar" method="POST"><button type="submit">Apagar</button></form>
  <form action="/alternar" method="POST"><button type="submit">Alternar</button></form>
  <p class="nota">El LED se apaga solo 10 segundos después de encenderse.<br>
  La página se actualiza cada 2 segundos.</p>
</body>
</html>
)HTML";

  pagina.replace("%ESTADO%", ledEncendido ? "ENCENDIDO" : "APAGADO");
  servidor.sendHeader("Cache-Control", "no-store");
  servidor.send(200, "text/html; charset=utf-8", pagina);
}

// POST /encender
void encenderLED() {
  fijarLED(true);
  Serial.println("Orden recibida: encender LED");
  redirigirAlInicio();
}

// POST /apagar
void apagarLED() {
  fijarLED(false);
  Serial.println("Orden recibida: apagar LED");
  redirigirAlInicio();
}

// POST /alternar -> invierte el estado actual.
void alternarLED() {
  fijarLED(!ledEncendido);
  Serial.println(ledEncendido ? "Orden recibida: alternar (LED encendido)"
                              : "Orden recibida: alternar (LED apagado)");
  redirigirAlInicio();
}

// Cualquier otra solicitud: 405 si la ruta existe pero con otro metodo
// (por ejemplo, abrir /encender escribiendo la direccion en el navegador),
// y 404 si la ruta no existe. En ambos casos el LED no cambia.
void rutaNoValida() {
  String ruta = servidor.uri();
  bool rutaConocida = (ruta == "/" || ruta == "/encender" || ruta == "/apagar" || ruta == "/alternar");

  if (rutaConocida) {
    servidor.sendHeader("Allow", (ruta == "/") ? "GET" : "POST");
    servidor.send(405, "text/plain", "Metodo no permitido");
  } else {
    servidor.send(404, "text/plain", "Ruta no encontrada");
  }
}

// ---------- Programa principal ----------

void setup() {
  Serial.begin(115200);

  // El LED siempre arranca apagado.
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  // Crear la red Wi-Fi propia del ESP32 (modo punto de acceso).
  WiFi.mode(WIFI_AP);
  redActiva = WiFi.softAP(NOMBRE_RED, CLAVE_RED);

  if (!redActiva) {
    Serial.println("ERROR: no se pudo crear la red Wi-Fi. Revisa que la clave tenga 8 o mas caracteres y reinicia la placa.");
    return;  // loop() queda en reposo; el LED permanece apagado
  }

  Serial.println("Red Wi-Fi creada correctamente");
  Serial.print("Nombre: ");
  Serial.println(NOMBRE_RED);
  Serial.print("Direccion IP: ");
  Serial.println(WiFi.softAPIP());   // Normalmente 192.168.4.1

  // Rutas del servidor web.
  servidor.on("/", HTTP_GET, mostrarPagina);
  servidor.on("/encender", HTTP_POST, encenderLED);
  servidor.on("/apagar", HTTP_POST, apagarLED);
  servidor.on("/alternar", HTTP_POST, alternarLED);
  servidor.onNotFound(rutaNoValida);
  servidor.begin();
  Serial.println("Servidor HTTP listo");
}

void loop() {
  if (!redActiva) {
    return;  // Sin red no hay nada que atender
  }
  servidor.handleClient();   // Atiende las solicitudes del navegador
  revisarAutoapagado();      // Temporizador de 10 s con millis(), sin delay
}
