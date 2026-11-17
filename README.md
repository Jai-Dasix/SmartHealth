# SmartHealth

Estación de triaje nutricional automatizada para control pediátrico (hasta 50 cm y 5 kg), basada en ESP32. Mide peso y talla, calcula el IMC, lo clasifica, muestra el resultado en una pantalla OLED, lo guarda en Firebase y avisa por correo al representante.

El diseño completo (alcance, diagramas, interfaces, alternativas, plan de pruebas y ética) está en [DESIGN.md](DESIGN.md).

**Autor:** Jaimar David Salazar Cabrera

## Hardware

| Componente | Conexión (ESP32) |
|---|---|
| Celda de carga 5 kg + HX711 | DT = GPIO 16, SCK = GPIO 4 |
| Sensor láser VL53L0X | I2C: SDA = GPIO 21, SCL = GPIO 22 |
| Pantalla OLED SSD1306 (0x3C) | I2C: SDA = GPIO 21, SCL = GPIO 22 |
| Lector RFID RC522 | SPI: SS = 5, SCK = 18, MISO = 19, MOSI = 23, RST = 27 |
| Buzzer | GPIO 25 |

## Estructura

```
src/main.cpp                  Firmware (máquina de estados)
include/pagina.h              Página web en vivo servida por el ESP32
include/easter_egg.h          Animación secreta de la pantalla
include/configuracion.example.h   Plantilla de credenciales y pacientes
herramientas/                 Calibración del HX711 y diagnóstico del RFID
docs/                         Diagramas y documento de diseño en Word
```

## Cómo compilar

1. Instala [PlatformIO](https://platformio.org/) (extensión de VS Code).
2. Copia `include/configuracion.example.h` como `include/configuracion.h` y completa tus datos (Wi-Fi, Firebase, Apps Script, tarjetas).
3. Calibra la celda de carga con `herramientas/calibracion_hx711.cpp.txt` (cópialo temporalmente a `src/main.cpp`) y ajusta `factorDeEscala` en `src/main.cpp`.
4. Compila y carga con PlatformIO (`pio run -t upload`) y abre el monitor serie a 115200.

En la parte superior de `src/main.cpp` hay un panel (`usarBuzzer`, `usarPantalla`, `usarRFID`, `usarBascula`, `usarLaser`): en `false` el componente se simula por el monitor serie.
