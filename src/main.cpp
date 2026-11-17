#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>         
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <time.h> 
#include <WebServer.h>
#include "pagina.h"
#include <HX711.h> 
#include <Wire.h>               
#include <Adafruit_VL53L0X.h>   
#include <Adafruit_GFX.h>       
#include <Adafruit_SSD1306.h>   
#include <SPI.h>                
#include <MFRC522.h>            

#include "configuracion.h"      // copiar desde configuracion.example.h (no se sube a GitHub)
#include "easter_egg.h"         

// =======================================================
//  PANEL DE CONTROL DE HARDWARE (MODULAR)
// true  = usa el componente físico.
// false = lo simula por el monitor serie (útil para probar sin armar todo).
// =======================================================
bool usarBuzzer   = true;   // Zumbador de sonidos
bool usarPantalla = true;   // Pantalla OLED SSD1306
bool usarRFID     = true;   // Lector de tarjetas RC522
bool usarBascula  = true;   // Celda de carga y HX711
bool usarLaser    = true;   // Sensor de distancia VL53L0X
// =======================================================

FirebaseData fbdo; FirebaseAuth auth; FirebaseConfig config; FirebaseJson json; 
WebServer server(80);

const long gmtOffset_sec = -5 * 3600;
const int daylightOffset_sec = 0;

#define BUZZER_PIN 25 
#define SS_PIN 5
#define RST_PIN 27 
#define DT_PIN 16
#define SCK_PIN 4
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

MFRC522 rfid(SS_PIN, RST_PIN);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
HX711 bascula;
Adafruit_VL53L0X lox = Adafruit_VL53L0X();

float factorDeEscala = 3000.0; 
float pesoAnterior = 0.0;
unsigned long tiempoInicioEstabilidad = 0;
const unsigned long TIEMPO_REQUERIDO_QUIETO = 3000; 
const float MARGEN_ERROR_KG = 0.3; 
bool sensorLaserListo = false;
const float DISTANCIA_PARED_CM = 60.0; 

enum EstadoBascula { ESTADO_REPOSO, ESTADO_PESAJE, ESTADO_ALTURA, ESTADO_CALCULO_ALERTA, ESTADO_NUBE };
EstadoBascula estadoActual = ESTADO_REPOSO;

String idPaciente = "";   // nombre mostrado en pantalla y en el correo (solo vive en el equipo)
String idNube = "";       // identificador anónimo (UID de la tarjeta) usado como clave en Firebase
String correoDestino = "";
float pesoFinal = 0.0; float alturaFinal = 0.0; float imcFinal = 0.0; String categoriaNutricional = "";
bool solicitarDato = true; 

void pitarInicio() {
  if (!usarBuzzer) return;
  tone(BUZZER_PIN, 1200, 100); delay(120);
  tone(BUZZER_PIN, 1800, 150); delay(150);
  noTone(BUZZER_PIN);
}

void pitarBloqueo() {
  if (!usarBuzzer) return;
  tone(BUZZER_PIN, 2000, 200); delay(200);
  noTone(BUZZER_PIN);
}

void pitarExito() {
  if (!usarBuzzer) return;
  tone(BUZZER_PIN, 880, 150); delay(150);
  tone(BUZZER_PIN, 1046, 150); delay(150);
  tone(BUZZER_PIN, 1318, 300); delay(300);
  noTone(BUZZER_PIN);
}

void mostrarOLED(String titulo, String linea1, String linea2 = "", String linea3 = "") {
  if (usarPantalla) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    display.setCursor(0,0);
    display.println(titulo);
    display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
    
    display.setTextSize(1);
    display.setCursor(0, 16); display.println(linea1);
    if (linea2 != "") { display.setCursor(0, 30); display.println(linea2); }
    if (linea3 != "") { display.setCursor(0, 44); display.println(linea3); }
    
    display.display();
  } else {
    Serial.println("\n[ PANTALLA OLED ] ===================");
    Serial.print("TÍTULO: "); Serial.println(titulo);
    Serial.print(" "); Serial.println(linea1);
    if (linea2 != "") { Serial.print(" "); Serial.println(linea2); }
    if (linea3 != "") { Serial.print(" "); Serial.println(linea3); }
    Serial.println("=====================================");
  }
}

String obtenerFechaHoraLegible() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "Fecha_Desconocida"; 
  char buffer[30];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", &timeinfo);
  return String(buffer);
}

String obtenerTextoEstado() {
  switch(estadoActual) {
    case ESTADO_REPOSO: return "ESPERANDO PACIENTE";
    case ESTADO_PESAJE: return "MIDIENDO PESO...";
    case ESTADO_ALTURA: return "MIDIENDO LONGITUD...";
    case ESTADO_CALCULO_ALERTA: return "CALCULANDO RESULTADOS...";
    case ESTADO_NUBE: return "GUARDANDO Y ENVIANDO CORREO...";
    default: return "DESCONOCIDO";
  }
}

void enviarCorreoAppsScript() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(APPS_SCRIPT_URL);
    http.addHeader("Content-Type", "application/json");

    String sugerencia = "";
    if (categoriaNutricional == "Bajo peso") sugerencia = "Sugerencia: Se recomienda evaluar la frecuencia de alimentacion con su pediatra.";
    else if (categoriaNutricional == "Normopeso") sugerencia = "Sugerencia: ¡Excelente noticia! El desarrollo del bebe es normal.";
    else sugerencia = "Sugerencia: Se aconseja comentar estos valores en la proxima consulta.";

    String fechaCorreo = "Fecha_Desconocida";
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      char buffer[30];
      strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
      fechaCorreo = String(buffer);
    }

    String payload = "";
    payload += "{";
    payload += "\"email\":\""; payload += correoDestino; payload += "\",";
    payload += "\"paciente\":\""; payload += idPaciente; payload += "\",";
    payload += "\"peso\":\""; payload += String(pesoFinal, 2); payload += "\",";
    payload += "\"altura\":\""; payload += String(alturaFinal, 1); payload += "\",";
    payload += "\"imc\":\""; payload += String(imcFinal, 2); payload += "\",";
    payload += "\"categoria\":\""; payload += categoriaNutricional; payload += "\",";
    payload += "\"sugerencia\":\""; payload += sugerencia; payload += "\",";
    payload += "\"fecha\":\""; payload += fechaCorreo; payload += "\""; 
    payload += "}";

    int httpResponseCode = http.POST(payload);
    if (httpResponseCode > 0) Serial.println("¡Correo enviado exitosamente vía Apps Script!");
    else { Serial.print("Error al enviar correo HTTP: "); Serial.println(httpResponseCode); }
    http.end();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(60000); 

  // Inicialización condicional de hardware
  if (usarBuzzer) {
    pinMode(BUZZER_PIN, OUTPUT);
    tone(BUZZER_PIN, 1000, 100); 
  }
  
  if (usarPantalla || usarLaser) Wire.begin(21, 22);   // bus I2C compartido (OLED y VL53L0X)

  if (usarPantalla) {
    if(display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) display.clearDisplay();
  }
  
  if (usarRFID) {
    SPI.begin(); 
    rfid.PCD_Init();
  }
  
  if (usarBascula) {
    bascula.begin(DT_PIN, SCK_PIN); 
    bascula.set_scale(factorDeEscala); 
    bascula.tare(); 
  }
  
  if (usarLaser) {
    if (lox.begin()) sensorLaserListo = true;
  }

  mostrarOLED("BASCULA IOT", "Iniciando sistema...");

  mostrarOLED("WIFI", "Conectando...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(300); }
  mostrarOLED("WIFI CONECTADO", "IP:", WiFi.localIP().toString());
  
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
  config.api_key = API_KEY; config.database_url = DATABASE_URL; config.signer.test_mode = true; 
  Firebase.begin(&config, &auth); Firebase.reconnectWiFi(true); fbdo.setBSSLBufferSize(1024, 1024); 

  server.on("/", []() { server.send(200, "text/html", html_web); });
  server.on("/api/datos_en_vivo", []() {
    FirebaseJson datosWeb;
    datosWeb.set("estado", obtenerTextoEstado());
    datosWeb.set("paciente", idPaciente);
    datosWeb.set("peso", estadoActual == ESTADO_PESAJE && usarBascula ? pesoAnterior : pesoFinal);
    datosWeb.set("altura", alturaFinal); datosWeb.set("imc", imcFinal); datosWeb.set("categoria", categoriaNutricional);
    String res; datosWeb.toString(res, false); server.send(200, "application/json", res);
  });
  server.begin();
  estadoActual = ESTADO_REPOSO;
}

void loop() {
  server.handleClient(); 

  switch (estadoActual) {

    case ESTADO_REPOSO:
      if (solicitarDato) {
        idPaciente = ""; idNube = ""; correoDestino = ""; pesoFinal = 0; alturaFinal = 0; imcFinal = 0; categoriaNutricional = "";
        if (usarBascula && bascula.is_ready()) bascula.tare(); 
        
        mostrarOLED("LISTO", "Acerque llavero", "o tarjeta RFID", "");
        
        if (!usarRFID) {
          Serial.println("\nEscriba el NOMBRE en el teclado:");
        } else {
          Serial.println("\nAcerque la TARJETA al lector:");
        }
        
        solicitarDato = false;
      }
      
      // =========================================
      // MODO TECLADO (Siempre activo como respaldo o simulación)
      // =========================================
      if (Serial.available() > 0) {
        idPaciente = Serial.readStringUntil('\n'); 
        idPaciente.trim();
        
        if (idPaciente.length() > 0) {
          
          if (idPaciente == UID_CEDULA_JEFE || idPaciente == UID_CEDULA_AMIGO) {
            ejecutarEasterEgg(display, rfid, UID_CEDULA_JEFE, UID_CEDULA_AMIGO, BUZZER_PIN, usarPantalla);
            solicitarDato = true; 
            break; 
          }

          if (idPaciente.equalsIgnoreCase(NOMBRE_PACIENTE_1)) {
            idPaciente = NOMBRE_PACIENTE_1; idNube = UID_TARJETA_1; correoDestino = CORREO_PACIENTE_1;
          } else if (idPaciente.equalsIgnoreCase(NOMBRE_PACIENTE_2)) {
            idPaciente = NOMBRE_PACIENTE_2; idNube = UID_TARJETA_2; correoDestino = CORREO_PACIENTE_2;
          } else {
            idNube = "SIN_TARJETA"; correoDestino = CORREO_POR_DEFECTO;
          }

          pitarInicio(); estadoActual = ESTADO_PESAJE; solicitarDato = true; tiempoInicioEstabilidad = millis(); 
        }
      }

      // =========================================
      // MODO RFID (Físico)
      // =========================================
      if (usarRFID && rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        String codigoTarjeta = "";
        for (byte i = 0; i < rfid.uid.size; i++) {
          if (rfid.uid.uidByte[i] < 0x10) codigoTarjeta += "0";
          codigoTarjeta += String(rfid.uid.uidByte[i], HEX);
        }
        codigoTarjeta.toUpperCase();
        
        if (codigoTarjeta == UID_CEDULA_JEFE || codigoTarjeta == UID_CEDULA_AMIGO) {
          rfid.PICC_HaltA(); 
          Serial.println("¡EASTER EGG DESCUBIERTO!");
          ejecutarEasterEgg(display, rfid, UID_CEDULA_JEFE, UID_CEDULA_AMIGO, BUZZER_PIN, usarPantalla);
          solicitarDato = true; 
        } 
        else if (codigoTarjeta == UID_TARJETA_1) {
          idPaciente = NOMBRE_PACIENTE_1; idNube = codigoTarjeta; correoDestino = CORREO_PACIENTE_1; rfid.PICC_HaltA(); pitarInicio(); estadoActual = ESTADO_PESAJE; solicitarDato = true; tiempoInicioEstabilidad = millis();
        } else if (codigoTarjeta == UID_TARJETA_2) {
          idPaciente = NOMBRE_PACIENTE_2; idNube = codigoTarjeta; correoDestino = CORREO_PACIENTE_2; rfid.PICC_HaltA(); pitarInicio(); estadoActual = ESTADO_PESAJE; solicitarDato = true; tiempoInicioEstabilidad = millis();
        } else {
          idPaciente = codigoTarjeta; idNube = codigoTarjeta; correoDestino = CORREO_POR_DEFECTO; rfid.PICC_HaltA(); pitarInicio(); estadoActual = ESTADO_PESAJE; solicitarDato = true; tiempoInicioEstabilidad = millis();
        }
      }
      break;

    case ESTADO_PESAJE:
      if (solicitarDato) {
        String titulo = "HOLA "; titulo += idPaciente;
        String linea2 = "a "; linea2 += idPaciente;
        mostrarOLED(titulo, "Por favor, ponga", linea2, "en la bascula.");
        if(!usarBascula) Serial.println("\n[SIMULACION] Ingrese el PESO (kg) en el teclado:");
        solicitarDato = false;
      }
      
      if (usarBascula && bascula.is_ready()) {
        float pesoActual = bascula.get_units(3); 
        if (pesoActual < 0) pesoActual = 0; 
        if (abs(pesoActual - pesoAnterior) > MARGEN_ERROR_KG) {
          tiempoInicioEstabilidad = millis(); pesoAnterior = pesoActual;
          mostrarOLED("PESANDO...", "Por favor,", "quedese quieto", "un momento.");
        } else {
          if (millis() - tiempoInicioEstabilidad >= TIEMPO_REQUERIDO_QUIETO) {
            if (pesoActual > 0.1) { pesoFinal = pesoActual; pitarBloqueo(); estadoActual = ESTADO_ALTURA; solicitarDato = true; } 
            else tiempoInicioEstabilidad = millis(); 
          }
        }
      } else if (!usarBascula) { 
        if (Serial.available() > 0) {
          pesoFinal = Serial.parseFloat();
          while(Serial.available()) Serial.read(); 
          if (pesoFinal > 0.1) { pitarBloqueo(); estadoActual = ESTADO_ALTURA; solicitarDato = true; }
        }
      }
      break;

    case ESTADO_ALTURA:
      if (solicitarDato) {
        String linea2 = "de "; linea2 += idPaciente; linea2 += "...";
        mostrarOLED("MIDIENDO...", "Midiendo longitud", linea2, "");
        if(!usarLaser) Serial.println("\n[SIMULACION] Ingrese la LONGITUD (cm) en el teclado:");
        solicitarDato = false; delay(1500); 
      }

      if (usarLaser && sensorLaserListo) {
        VL53L0X_RangingMeasurementData_t medicion;
        float sumaDistancias = 0; int exitosas = 0;
        for (int i = 0; i < 5; i++) {
          lox.rangingTest(&medicion, false);
          if (medicion.RangeStatus != 4) { sumaDistancias += (medicion.RangeMilliMeter / 10.0); exitosas++; }
          delay(50);
        }
        if (exitosas > 0) {
          float distanciaSensor = sumaDistancias / exitosas;
          alturaFinal = DISTANCIA_PARED_CM - distanciaSensor;
          if (alturaFinal > 10.0 && alturaFinal <= 55.0) { pitarBloqueo(); estadoActual = ESTADO_CALCULO_ALERTA; solicitarDato = true; } 
          else delay(500); 
        }
      } else if (!usarLaser) { 
        if (Serial.available() > 0) {
          alturaFinal = Serial.parseFloat();
          while(Serial.available()) Serial.read();
          if (alturaFinal > 0) { pitarBloqueo(); estadoActual = ESTADO_CALCULO_ALERTA; solicitarDato = true; }
        }
      }
      break;

    case ESTADO_CALCULO_ALERTA:
      imcFinal = pesoFinal / pow(alturaFinal/100.0, 2);
      
      if (imcFinal < 13.0) categoriaNutricional = "Bajo peso";
      else if (imcFinal >= 13.0 && imcFinal <= 18.0) categoriaNutricional = "Normopeso";
      else if (imcFinal > 18.0 && imcFinal <= 20.0) categoriaNutricional = "Sobrepeso";
      else categoriaNutricional = "Obesidad";
      
      {
        String txtPeso = "Peso: "; txtPeso += String(pesoFinal, 2); txtPeso += " kg";
        String txtLongitud = "Longitud: "; txtLongitud += String(alturaFinal, 1); txtLongitud += " cm";
        String txtIMC = "Estado: "; txtIMC += categoriaNutricional;
        mostrarOLED("RESULTADOS", txtPeso, txtLongitud, txtIMC);
      }
      delay(4000); estadoActual = ESTADO_NUBE; break;

    case ESTADO_NUBE: {
      mostrarOLED("ENVIANDO...", "Subiendo a nube", "y enviando email...", "");
      Serial.println("\n--- INICIANDO GUARDADO ---");
      
      json.clear(); 
      json.set("peso_kg", pesoFinal); json.set("altura_cm", alturaFinal); 
      json.set("imc", imcFinal); json.set("categoria", categoriaNutricional);

      String fechaHoraReal = obtenerFechaHoraLegible(); 
      String ruta = "/Pacientes/"; ruta += idNube; ruta += "/historial_pesajes/"; ruta += fechaHoraReal;
      
      Serial.print("Ruta de Firebase: "); Serial.println(ruta);
      if (Firebase.RTDB.setJSON(&fbdo, ruta.c_str(), &json)) Serial.println("[OK] Guardado en Firebase exitoso.");
      else { Serial.print("[ERROR] Firebase rechazo el dato: "); Serial.println(fbdo.errorReason()); }

      enviarCorreoAppsScript();

      pitarExito(); 
      mostrarOLED("EXITO", "Datos guardados", "y correo enviado!", "Gracias!");
      delay(4000); estadoActual = ESTADO_REPOSO; break;
    }
  }
}