// Version minima de la API de Arduino/ESP32 para compilar el sketch en un PC.
// NO sustituye al compilador real del ESP32: solo permite probar la logica.
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <string>

#define HIGH 1
#define LOW 0
#define OUTPUT 1
enum HTTPMethod { HTTP_GET, HTTP_POST };
enum { WIFI_AP = 2 };

struct String : std::string {
  String(const char* s = "") : std::string(s) {}
  String(const std::string& s) : std::string(s) {}
  void replace(const char* a, const char* b) {
    size_t p = find(a);
    while (p != npos) { std::string::replace(p, strlen(a), b); p = find(a, p + strlen(b)); }
  }
};

// Reloj y pines simulados. millis() es de 32 bits, igual que en el ESP32,
// para poder probar el desbordamiento.
uint32_t mock_ms = 0;
int delay_calls = 0, pin_state[40] = {0}, pin_mode[40] = {0};
uint32_t millis() { return mock_ms; }
void delay(uint32_t ms) { delay_calls++; mock_ms += ms; }
void pinMode(int p, int m) { pin_mode[p] = m; }
void digitalWrite(int p, int v) { pin_state[p] = v; }

struct IPAddress {};
struct SerialMock {
  std::string out;
  void begin(int) {}
  void print(const char* s) { out += s; }
  void println(const char* s) { out += s; out += '\n'; }
  void println(IPAddress) { out += "192.168.4.1\n"; }
} Serial;

bool mock_softap_ok = true;  // Poner en false para simular que falla la red
struct WiFiMock {
  int modo = -1, llamadas_softAP = 0;
  std::string ssid; size_t clave_len = 0;
  void mode(int m) { modo = m; }
  bool softAP(const char* s, const char* c) {
    llamadas_softAP++; ssid = s; clave_len = strlen(c);
    return mock_softap_ok && clave_len >= 8;  // el ESP32 exige 8 o mas caracteres
  }
  IPAddress softAPIP() { return {}; }
} WiFi;

struct WebServer {
  std::map<std::string, std::function<void()>> rutas;
  std::function<void()> no_encontrada;
  std::map<std::string, std::string> cabeceras;
  int code = 0; std::string body, tipo, ruta_actual; bool started = false;
  WebServer(int) {}
  void on(const char* u, HTTPMethod m, std::function<void()> f) { rutas[std::string(m == HTTP_GET ? "GET " : "POST ") + u] = f; }
  void onNotFound(std::function<void()> f) { no_encontrada = f; }
  void begin() { started = true; }
  void handleClient() {}
  String uri() { return ruta_actual; }
  void sendHeader(const char* k, const char* v) { cabeceras[k] = v; }
  void send(int c, const char* t, const String& b) { code = c; tipo = t; body = b; }
  // Ayuda de prueba: simula la llegada de una solicitud HTTP.
  int request(const char* metodo, const char* uri_) {
    code = 0; body.clear(); tipo.clear(); cabeceras.clear(); ruta_actual = uri_;
    auto it = rutas.find(std::string(metodo) + " " + uri_);
    if (it != rutas.end()) it->second();
    else if (no_encontrada) no_encontrada();
    else code = 404;
    return code;
  }
};
