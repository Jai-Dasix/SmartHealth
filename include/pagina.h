#ifndef PAGINA_H
#define PAGINA_H
#include <Arduino.h>

const String html_web = R"=====(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Dashboard Báscula IoT</title>
  <style>
    body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background-color: #eef2f5; text-align: center; color: #333; padding: 20px; }
    .card { background: white; border-radius: 10px; padding: 20px; max-width: 500px; margin: 0 auto; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }
    h1 { color: #2c3e50; font-size: 1.8rem;}
    .status { font-size: 1.3rem; font-weight: bold; color: #e74c3c; padding: 15px; border: 2px dashed #e74c3c; border-radius: 5px; margin-bottom: 20px; background: #fff4f4;}
    .data-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; text-align: left; }
    .data-box { background: #f8f9fa; padding: 15px; border-left: 5px solid #3498db; border-radius: 4px; font-size: 1.1rem; }
    .val { font-weight: bold; color: #2980b9; font-size: 1.3rem; display: block; margin-top: 5px;}
  </style>
  <script>
    // Pide los datos al ESP32 cada segundo (1000 ms)
    setInterval(() => {
      fetch('/api/datos_en_vivo')
        .then(res => res.json())
        .then(data => {
          // Actualiza la pantalla con los datos recibidos
          document.getElementById('estado').innerText = data.estado;
          document.getElementById('paciente').innerText = data.paciente || '---';
          document.getElementById('peso').innerText = data.peso > 0 ? data.peso + ' kg' : '---';
          document.getElementById('altura').innerText = data.altura > 0 ? data.altura + ' cm' : '---';
          document.getElementById('imc').innerText = data.imc > 0 ? data.imc : '---';
          document.getElementById('categoria').innerText = data.categoria || '---';
          
          // Cambia el color del estado según lo que esté haciendo
          let color = (data.estado === "ESPERANDO PACIENTE") ? "#27ae60" : "#e74c3c";
          document.getElementById('estado').style.color = color;
          document.getElementById('estado').style.borderColor = color;
        });
    }, 1000);
  </script>
</head>
<body>
  <div class="card">
    <h1>Monitor Médico en Vivo</h1>
    <div class="status" id="estado">Sincronizando...</div>
    <div class="data-grid">
      <div class="data-box" style="grid-column: span 2;">ID Paciente: <span class="val" id="paciente">---</span></div>
      <div class="data-box">Peso: <span class="val" id="peso">---</span></div>
      <div class="data-box">Altura: <span class="val" id="altura">---</span></div>
      <div class="data-box">IMC: <span class="val" id="imc">---</span></div>
      <div class="data-box">Categoría: <span class="val" id="categoria">---</span></div>
    </div>
  </div>
</body>
</html>
)=====";

#endif