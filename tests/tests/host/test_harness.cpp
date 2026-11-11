// Pruebas automaticas del sketch real en un PC (ver instrucciones.md).
#include "arduino_mock.h"
#include "../../Control_LED_ESP32_WiFi.ino"
#include <fstream>
#include <sstream>

#define RUTA_SKETCH "../../Control_LED_ESP32_WiFi.ino"
static int total = 0, fallos = 0;

static void verificar(bool ok, const char* id, const char* descripcion) {
  total++; if (!ok) fallos++;
  printf("[%s] %-4s %s\n", ok ? "OK   " : "FALLO", id, descripcion);
}
static bool contiene(const std::string& t, const char* s) { return t.find(s) != std::string::npos; }
static int contar(const std::string& t, const char* s) {
  int n = 0; for (size_t p = t.find(s); p != std::string::npos; p = t.find(s, p + 1)) n++; return n;
}
static int pedir(const char* m, const char* u) { return servidor.request(m, u); }
static bool pagina_dice(const char* estado) { return pedir("GET", "/") == 200 && contiene(servidor.body, estado); }
static bool coherente() { return ledEncendido == (pin_state[PIN_LED] == HIGH); }  // variable == pin

int main() {
  // ---- E: revision estatica del codigo fuente ----
  std::ifstream f(RUTA_SKETCH); std::stringstream ss; ss << f.rdbuf(); std::string fuente = ss.str();
  verificar(contiene(fuente, "#include <WiFi.h>") && contiene(fuente, "#include <WebServer.h>"), "E1", "Usa solo las librerias WiFi.h y WebServer.h del paquete ESP32");
  verificar(contiene(fuente, "WIFI_AP") && contiene(fuente, "softAP("), "E2", "Crea la red con WIFI_AP y softAP()");
  verificar(contiene(fuente, "PIN_LED    = 23") || contiene(fuente, "PIN_LED = 23"), "E3", "El LED esta en el GPIO 23");
  verificar(!contiene(fuente, "delay("), "E4", "El codigo no contiene ninguna llamada a delay()");
  verificar(contiene(fuente, "millis()"), "E5", "El temporizador usa millis()");

  // ---- A: arranque ----
  setup(); loop();
  verificar(pin_mode[PIN_LED] == OUTPUT, "A1", "GPIO 23 configurado como salida");
  verificar(PIN_LED == 23 && pin_state[PIN_LED] == LOW && !ledEncendido, "A2", "Al arrancar el LED esta apagado");
  verificar(WiFi.modo == WIFI_AP, "A3", "Modo Wi-Fi: punto de acceso (WIFI_AP)");
  verificar(WiFi.llamadas_softAP == 1 && WiFi.ssid == "ESP32_LED_AULA" && WiFi.clave_len >= 8, "A4", "softAP() con SSID ESP32_LED_AULA y clave de 8+ caracteres");
  verificar(servidor.started, "A5", "El servidor HTTP queda iniciado");
  verificar(contiene(Serial.out, "Direccion IP: 192.168.4.1") && contiene(Serial.out, "ESP32_LED_AULA"), "A6", "El monitor serie informa el nombre de la red y la IP");
  verificar(servidor.rutas.count("GET /") && servidor.rutas.count("POST /encender") && servidor.rutas.count("POST /apagar") && servidor.rutas.count("POST /alternar") && servidor.no_encontrada, "A7", "Rutas registradas: GET /, POST /encender, /apagar, /alternar y onNotFound");

  // ---- P: pagina web ----
  int c = pedir("GET", "/");
  verificar(c == 200 && servidor.tipo.find("text/html") == 0, "P1", "GET / responde 200 con tipo text/html");
  verificar(contiene(servidor.body, "APAGADO") && !contiene(servidor.body, "%ESTADO%"), "P2", "La pagina inicial muestra APAGADO y no deja marcadores sin reemplazar");
  verificar(contar(servidor.body, "method=\"POST\"") == 3 && contiene(servidor.body, "action=\"/encender\"") && contiene(servidor.body, "action=\"/apagar\"") && contiene(servidor.body, "action=\"/alternar\""), "P3", "Hay tres botones (Encender, Apagar, Alternar) que envian POST");
  verificar(contiene(servidor.body, "http-equiv=\"refresh\"") && contiene(servidor.body, "name=\"viewport\"") && servidor.cabeceras["Cache-Control"] == "no-store", "P4", "Pagina movil, con recarga automatica y sin cache");

  // ---- C: control del LED ----
  c = pedir("POST", "/encender");
  verificar(c == 303 && servidor.cabeceras["Location"] == "/" && pin_state[PIN_LED] == HIGH && coherente(), "C1", "POST /encender: GPIO en HIGH y redireccion 303 a /");
  verificar(pagina_dice("ENCENDIDO"), "C2", "La pagina muestra ENCENDIDO");
  pedir("POST", "/encender");
  verificar(ledEncendido && pin_state[PIN_LED] == HIGH, "C3", "Encender dos veces seguidas mantiene el LED encendido");
  c = pedir("POST", "/apagar");
  verificar(c == 303 && pin_state[PIN_LED] == LOW && coherente() && pagina_dice("APAGADO"), "C4", "POST /apagar: GPIO en LOW y la pagina muestra APAGADO");
  pedir("POST", "/apagar");
  verificar(!ledEncendido && pin_state[PIN_LED] == LOW, "C5", "Apagar con el LED ya apagado no cambia nada");
  bool a = pedir("POST", "/alternar") == 303 && pin_state[PIN_LED] == HIGH && coherente() && pagina_dice("ENCENDIDO");
  bool b = pedir("POST", "/alternar") == 303 && pin_state[PIN_LED] == LOW && coherente() && pagina_dice("APAGADO");
  bool d = pedir("POST", "/alternar") == 303 && pin_state[PIN_LED] == HIGH && coherente();
  verificar(a && b && d, "C6", "POST /alternar x3: encendido, apagado, encendido");
  pedir("POST", "/apagar");
  bool estable = true;
  for (int i = 0; i < 50; i++) {
    estable &= pedir("POST", "/encender") == 303 && coherente() && pin_state[PIN_LED] == HIGH;
    estable &= pedir("POST", "/alternar") == 303 && coherente() && pin_state[PIN_LED] == LOW;
    estable &= pedir("POST", "/alternar") == 303 && coherente() && pin_state[PIN_LED] == HIGH;
    estable &= pedir("POST", "/apagar") == 303 && coherente() && pin_state[PIN_LED] == LOW;
  }
  verificar(estable, "C7", "200 ordenes mezcladas: la variable y el pin siempre coinciden");

  // ---- T: temporizador de autoapagado ----
  mock_ms = 1000; pedir("POST", "/encender");
  mock_ms = 1000 + 9999; loop();
  verificar(ledEncendido && pin_state[PIN_LED] == HIGH, "T1", "A los 9,999 s el LED sigue encendido");
  mock_ms = 1000 + 10000; loop();
  verificar(!ledEncendido && pin_state[PIN_LED] == LOW && coherente(), "T2", "A los 10 s el LED se apaga solo");
  verificar(pagina_dice("APAGADO"), "T3", "Despues del autoapagado la pagina muestra APAGADO");
  mock_ms = 50000; pedir("POST", "/encender"); mock_ms = 54000; pedir("POST", "/apagar"); mock_ms = 90000; loop();
  verificar(!ledEncendido && pin_state[PIN_LED] == LOW, "T4", "Apagar antes de los 10 s funciona y el LED no se reenciende");
  mock_ms = 100000; pedir("POST", "/encender"); mock_ms = 106000; pedir("POST", "/encender");
  mock_ms = 106000 + 9999; loop(); bool sigue1 = ledEncendido;
  mock_ms = 106000 + 10000; loop();
  verificar(sigue1 && !ledEncendido, "T5", "Encender de nuevo antes del vencimiento reinicia los 10 s");
  mock_ms = 200000; pedir("POST", "/alternar"); mock_ms = 203000; pedir("POST", "/alternar"); mock_ms = 206000; pedir("POST", "/alternar");
  mock_ms = 206000 + 9999; loop(); bool sigue2 = ledEncendido;
  mock_ms = 206000 + 10000; loop();
  verificar(sigue2 && !ledEncendido, "T6", "Alternar a encendido tambien inicia un plazo nuevo de 10 s");
  mock_ms = 0xFFFFFFFFu - 2000; pedir("POST", "/encender");
  mock_ms += 9999; loop(); bool sigue3 = ledEncendido;   // millis() ya dio la vuelta a 0
  mock_ms += 1; loop();
  verificar(sigue3 && !ledEncendido, "T7", "El autoapagado funciona aunque millis() se desborde");
  mock_ms = 300000; loop();
  verificar(!ledEncendido && pin_state[PIN_LED] == LOW, "T8", "Con el LED apagado, loop() no lo vuelve a encender");

  // ---- H: errores HTTP ----
  int antes = pin_state[PIN_LED];
  verificar(pedir("GET", "/inexistente") == 404 && pedir("POST", "/otra") == 404, "H1", "Rutas que no existen responden 404");
  verificar(pedir("GET", "/encender") == 405 && servidor.cabeceras["Allow"] == "POST", "H2", "GET /encender responde 405 e indica Allow: POST");
  verificar(pedir("GET", "/apagar") == 405 && pedir("GET", "/alternar") == 405, "H3", "GET /apagar y GET /alternar responden 405");
  verificar(pedir("POST", "/") == 405 && servidor.cabeceras["Allow"] == "GET", "H4", "POST / responde 405 e indica Allow: GET");
  verificar(pin_state[PIN_LED] == antes && coherente(), "H5", "Las solicitudes invalidas no cambian el LED");

  // ---- R: fallo al crear la red ----
  servidor.started = false; servidor.rutas.clear(); servidor.no_encontrada = nullptr; Serial.out.clear();
  mock_softap_ok = false; setup(); loop(); loop();
  verificar(!servidor.started && servidor.rutas.empty() && pin_state[PIN_LED] == LOW, "R1", "Si falla la red: servidor sin iniciar y LED apagado");
  verificar(contiene(Serial.out, "ERROR: no se pudo crear la red Wi-Fi"), "R2", "Si falla la red se informa el error por el monitor serie");

  // ---- Global ----
  verificar(delay_calls == 0, "G1", "Durante todas las pruebas no se llamo a delay()");

  printf("\nResultado: %d/%d pruebas superadas, %d fallidas\n", total - fallos, total, fallos);
  return fallos ? 1 : 0;
}
