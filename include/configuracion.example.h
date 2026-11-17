#ifndef CONFIGURACION_H
#define CONFIGURACION_H

#include <Arduino.h>

// Copia este archivo como "include/configuracion.h" y completa tus datos.
// "configuracion.h" está en .gitignore y no se sube al repositorio.

// ==========================================
// 1. RED Y NUBE
// ==========================================
#define WIFI_SSID "tu_red_wifi"
#define WIFI_PASSWORD "tu_clave_wifi"
#define API_KEY "tu_api_key_de_firebase"
#define DATABASE_URL "tu-proyecto-default-rtdb.firebaseio.com"
const String APPS_SCRIPT_URL = "https://script.google.com/macros/s/TU_ID_DE_DESPLIEGUE/exec";

// ==========================================
// 2. PACIENTES (UID de la tarjeta RFID -> nombre y correo del representante)
// El UID se obtiene con herramientas/diagnostico_rfid.cpp.txt
// ==========================================
const String UID_TARJETA_1 = "00000000";
const String NOMBRE_PACIENTE_1 = "Paciente1";
const String CORREO_PACIENTE_1 = "representante1@ejemplo.com";

const String UID_TARJETA_2 = "11111111";
const String NOMBRE_PACIENTE_2 = "Paciente2";
const String CORREO_PACIENTE_2 = "representante2@ejemplo.com";

// Se usa con tarjetas no registradas o al ingresar un nombre por teclado.
const String CORREO_POR_DEFECTO = "correo@ejemplo.com";

// ==========================================
// 3. MODO SECRETO (tarjetas que activan la animación de la pantalla)
// ==========================================
const String UID_CEDULA_JEFE = "A1B2C3D4";
const String UID_CEDULA_AMIGO = "E5F6G7H8";

#endif
