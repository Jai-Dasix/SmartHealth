#ifndef EASTER_EGG_H
#define EASTER_EGG_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <MFRC522.h>

const unsigned char dino_frame_1[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x01, 0xFC, 0x00, 0x01, 0xFF, 0x00, 0x03, 0xFF,
  0x00, 0x03, 0xFF, 0x00, 0x03, 0xFF, 0x00, 0x03, 0xFF, 0x00, 0x03, 0xF0,
  0x00, 0x03, 0xFF, 0x00, 0x03, 0xF0, 0x00, 0x03, 0xE0, 0x00, 0x07, 0xC0,
  0x20, 0x07, 0x80, 0x30, 0x0F, 0x00, 0x38, 0x1E, 0x00, 0x3C, 0x3C, 0x00,
  0x3E, 0x78, 0x00, 0x1F, 0xF0, 0x00, 0x0F, 0xE0, 0x00, 0x07, 0xC0, 0x00,
  0x06, 0x00, 0x00, 0x06, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char dino_frame_2[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x01, 0xFC, 0x00, 0x01, 0xFF, 0x00, 0x03, 0xFF,
  0x00, 0x03, 0xFF, 0x00, 0x03, 0xFF, 0x00, 0x03, 0xFF, 0x00, 0x03, 0xF0,
  0x00, 0x03, 0xFF, 0x00, 0x03, 0xF0, 0x00, 0x03, 0xE0, 0x00, 0x07, 0xC0,
  0x20, 0x07, 0x80, 0x30, 0x0F, 0x00, 0x38, 0x1E, 0x00, 0x3C, 0x3C, 0x00,
  0x3E, 0x78, 0x00, 0x1F, 0xF0, 0x00, 0x0F, 0xE0, 0x00, 0x07, 0xC0, 0x00,
  0x03, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// NUEVO: Se agregó 'bool conSensores' a la función
void ejecutarEasterEgg(Adafruit_SSD1306 &pantalla, MFRC522 &lectorRfid, String cedula1, String cedula2, int pinBuzzer, bool conSensores) {
  bool saliendo = false;
  int frame = 0;
  int posX = -20;

  if (conSensores) {
    tone(pinBuzzer, 659, 150); delay(150);
    tone(pinBuzzer, 784, 200); delay(200);
    noTone(pinBuzzer);
  } else {
    Serial.println("\n🎶 [PI-PU!] 🎶");
    Serial.println("🦖 ¡EASTER EGG ACTIVADO EN MODO CONSOLA! 🦖");
    Serial.println("(Escribe tu codigo otra vez para salir)");
  }
  
  while (!saliendo) {
    if (conSensores) {
      pantalla.clearDisplay();
      pantalla.setTextSize(1);
      pantalla.setTextColor(SSD1306_WHITE);
      pantalla.setCursor(20, 0);
      pantalla.println("MODO SECRETO!");
      
      if (frame % 2 == 0) pantalla.drawBitmap(posX, 25, dino_frame_1, 24, 24, SSD1306_WHITE);
      else pantalla.drawBitmap(posX, 25, dino_frame_2, 24, 24, SSD1306_WHITE);
      
      pantalla.drawLine(0, 50, 128, 50, SSD1306_WHITE);
      pantalla.display();

      posX += 4;
      if (posX > 128) posX = -24; 
    } else {
      // Simulación de dinosaurio en texto para la consola
      if (frame % 2 == 0) Serial.print(" 🦖... ");
      else Serial.print(" ...🦖 ");
    }
    frame++;

    unsigned long tiempoInicio = millis();
    while (millis() - tiempoInicio < 120) { 
      
      // 1. Revisa el Lector RFID (Solo si hay hardware físico)
      if (conSensores && lectorRfid.PICC_IsNewCardPresent() && lectorRfid.PICC_ReadCardSerial()) {
        String codigo = "";
        for (byte i = 0; i < lectorRfid.uid.size; i++) {
          if (lectorRfid.uid.uidByte[i] < 0x10) codigo += "0";
          codigo += String(lectorRfid.uid.uidByte[i], HEX);
        }
        codigo.toUpperCase();
        
        if (codigo == cedula1 || codigo == cedula2) {
          saliendo = true;
          lectorRfid.PICC_HaltA();
          break; 
        }
        lectorRfid.PICC_HaltA(); 
      }

      // 2. Revisa el Teclado (Siempre activo, físico o simulación)
      if (Serial.available() > 0) {
        String entrada = Serial.readStringUntil('\n');
        entrada.trim();
        if (entrada == cedula1 || entrada == cedula2) {
          saliendo = true;
          if (!conSensores) Serial.println("\nVolviendo a la normalidad...");
          break;
        }
      }
      yield(); 
    }
  }
  
  if (conSensores) {
    tone(pinBuzzer, 440, 150); delay(150);
    noTone(pinBuzzer);
    pantalla.clearDisplay();
    pantalla.display();
  }
}

#endif