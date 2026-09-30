# ESP Remote - Sistema de Control Inalámbrico mediante ESP-NOW

## Introducción

**ESP Remote** es un sistema de control inalámbrico desarrollado con ESP8266 y ESP32 que permite relacionar entradas físicas con salidas remotas mediante ESP-NOW.

El proyecto busca simplificar el uso de microcontroladores en instalaciones interactivas, permitiendo configurar el funcionamiento del sistema desde una interfaz web sin tener que modificar y recompilar el código para cada nueva configuración.

Está pensado especialmente para artistas, músicos, performers, realizadores audiovisuales, estudiantes y usuarios que quieran incorporar electrónica e interacción a sus proyectos sin necesitar conocimientos avanzados de programación.

En la versión actual:

- **ESP8266:** funciona como emisor.
- **ESP32:** funciona como receptor y servidor de la interfaz web.
- **ESP-NOW:** realiza la comunicación inalámbrica entre ambas placas.
- **Interfaz web:** permite configurar las relaciones entre entradas y salidas.

---

# Índice

1. Objetivo
2. Arquitectura
3. Tecnologías utilizadas
4. Hardware del prototipo
5. Funcionamiento
6. ESP8266 emisor
7. ESP32 receptor
8. Comunicación ESP-NOW
9. Interfaz web
10. Cards
11. Control desde la web
12. Filtro por MAC
13. Configuración persistente
14. Conexión del prototipo
15. Instalación
16. Puesta en marcha
17. Ejemplo de configuración
18. Demostración
19. Aplicaciones posibles
20. Pruebas realizadas
21. Autoría

---

# Objetivo

El objetivo principal de ESP Remote es desarrollar una herramienta que permita configurar sistemas electrónicos inalámbricos sin tener que modificar constantemente su código.

El sistema permite:

- Comunicar dos microcontroladores mediante ESP-NOW.
- Asociar entradas físicas con salidas remotas.
- Configurar GPIO de entrada y salida desde una interfaz web.
- Trabajar con señales digitales y analógicas.
- Controlar salidas digitales y PWM.
- Crear configuraciones sin recompilar el firmware.
- Controlar salidas directamente desde un celular o computadora.
- Guardar la configuración para conservarla después de reiniciar el dispositivo.

---

# Arquitectura

La arquitectura actual del sistema es:

```text
Entradas físicas
      │
      ▼
ESP8266 - EMISOR
      │
      │ ESP-NOW Broadcast
      ▼
ESP32 - RECEPTOR
      │
      ├── Salidas digitales / PWM
      │
      └── WiFi + Interfaz Web
                │
                ▼
        Celular / Computadora
```

Las placas no necesitan una conexión física entre ellas para comunicarse.

---

# Tecnologías utilizadas

## Hardware

- ESP8266 NodeMCU
- ESP32
- Pulsadores
- Potenciómetro
- LEDs para pruebas
- Resistencias

## Software y protocolos

- Arduino IDE
- ESP-NOW
- WiFi
- WebServer
- EEPROM
- HTML
- CSS
- JavaScript

---

# Hardware utilizado para el prototipo

Para validar el funcionamiento se construyó un circuito de prueba utilizando:

- 1 ESP8266
- 1 ESP32
- 3 pulsadores
- 1 potenciómetro
- LEDs
- Resistencias de 220 Ω
- Protoboard y cables

Estos componentes corresponden solamente al **prototipo de prueba**.

ESP Remote no está pensado exclusivamente para controlar LEDs mediante pulsadores y potenciómetros. El sistema puede adaptarse a otros sensores, actuadores y dispositivos compatibles con las entradas y salidas disponibles.

---

# Funcionamiento

El ESP8266 lee las entradas físicas conectadas.

Cuando detecta un cambio, genera un mensaje y lo transmite mediante **ESP-NOW**.

El ESP32 recibe ese mensaje y busca si existe una card configurada para esa entrada.

Si encuentra una relación válida, ejecuta la salida correspondiente.

Por ejemplo:

```text
Pulsador
   │
GPIO5 - ESP8266
   │
   │ ESP-NOW
   ▼
ESP32
   │
GPIO25
   │
   ▼
Salida
```

La relación entre GPIO5 y GPIO25 no tiene que estar escrita de forma fija en el código: puede configurarse desde la interfaz web.

---

# ESP8266 - Emisor

El ESP8266 funciona como dispositivo emisor.

La versión actual del firmware permite leer:

- 3 entradas digitales.
- 1 entrada analógica.

Las entradas digitales utilizan `INPUT_PULLUP`, por lo que los pulsadores se conectan entre el GPIO correspondiente y GND.

Configuración utilizada actualmente:

```text
D1 / GPIO5  → Entrada digital
D2 / GPIO4  → Entrada digital
D5 / GPIO14 → Entrada digital

A0 → Entrada analógica
```

El ESP8266 lee A0 en un rango aproximado de:

```text
0 - 1023
```

Los cambios detectados se envían automáticamente mediante ESP-NOW.

Para evitar lecturas innecesarias se implementaron:

- debounce para las entradas digitales;
- umbral mínimo de cambio para la entrada analógica.

---

# ESP32 - Receptor y servidor web

El ESP32 cumple dos funciones principales:

1. Recibir los mensajes ESP-NOW.
2. Generar la interfaz web de configuración y control.

Cuando recibe un mensaje, compara:

- tipo de entrada;
- GPIO de entrada;

con las cards configuradas.

Si encuentra una coincidencia, ejecuta la salida correspondiente.

El ESP32 permite trabajar con:

- salidas digitales;
- salidas PWM.

Los valores analógicos recibidos desde el ESP8266 en rango `0-1023` son adaptados a un rango PWM de:

```text
0 - 255
```

---

# Comunicación ESP-NOW

La comunicación entre las placas utiliza **ESP-NOW**.

Cada mensaje contiene:

```text
Tipo de entrada
GPIO de entrada
Valor digital
Valor analógico
```

De esta forma, el receptor puede identificar qué entrada cambió y qué valor debe utilizar.

---

# Modo Broadcast

El ESP8266 transmite utilizando la dirección:

```text
FF:FF:FF:FF:FF:FF
```

Esto corresponde al modo **Broadcast**.

El emisor no necesita conocer previamente la dirección MAC del ESP32 receptor.

Los dispositivos deben utilizar el mismo canal ESP-NOW. En esta versión se utiliza:

```text
Canal 1
```

---

# Interfaz web

El ESP32 crea automáticamente su propia red WiFi.

El nombre tiene el formato:

```text
ESP-REMOTE-XXXX
```

`XXXX` corresponde a los últimos caracteres utilizados para identificar al ESP32.

La contraseña es:

```text
12345678
```

Una vez conectado a esa red, la interfaz se encuentra en:

```text
http://192.168.4.1
```

No es necesario instalar ninguna aplicación.

La interfaz puede utilizarse desde:

- celular;
- tablet;
- notebook;
- computadora con WiFi.

---

# Cards

El sistema utiliza el concepto de **cards**.

Cada card representa una configuración independiente de control.

La versión actual permite crear hasta:

```text
8 cards
```

Una card puede definir:

- Nombre.
- Uso o no de una entrada física.
- Tipo de entrada.
- GPIO de entrada.
- Tipo de salida.
- GPIO de salida.
- Modo llave.
- Modo pulsador.
- Tiempo de retención.
- Lógica invertida.
- Salida digital.
- Salida PWM.

Los GPIO ya utilizados por otras cards se identifican desde la interfaz para evitar configuraciones duplicadas.

También existe una opción **Manual** que permite ingresar un GPIO directamente.

---

# Cards sin entrada física

Una de las funciones incorporadas en la versión final es la posibilidad de crear una card **sin entrada física**.

Esto permite controlar una salida directamente desde la interfaz web.

Por ejemplo:

```text
Celular
   │
   │ WiFi
   ▼
ESP32
   │
GPIO
   │
   ▼
Salida
```

De esta manera ESP Remote también puede utilizarse como sistema de control remoto desde un celular o computadora.

---

# Modos digitales

Las cards digitales pueden configurarse en diferentes modos.

## Modo llave

La salida mantiene el estado recibido por la entrada.

```text
Entrada ON  → Salida ON
Entrada OFF → Salida OFF
```

## Modo pulsador

La salida se activa al presionar el pulsador.

Cuando el pulsador se libera comienza a contar el tiempo de retención configurado antes de apagar la salida.

Ejemplo:

```text
Retención: 1000 ms
```

La salida permanece activa durante un segundo después de liberar el pulsador.

---

# Control PWM

Las entradas analógicas pueden utilizarse para controlar una salida PWM.

En el prototipo:

```text
Potenciómetro
      │
      ▼
A0 ESP8266
      │
   0 - 1023
      │
   ESP-NOW
      ▼
ESP32
      │
   0 - 255
      ▼
Salida PWM
```

Esto permite controlar de manera gradual elementos compatibles con PWM.

---

# Dashboard

La página principal funciona como un dashboard.

Desde allí se visualizan las cards configuradas y el estado de cada salida.

Dependiendo de la configuración, también es posible:

- encender y apagar salidas digitales;
- accionar salidas configuradas como pulsadores;
- modificar una salida PWM mediante un control deslizante.

Las acciones se realizan sin necesidad de recargar completamente la página.

---

# Configuración y prueba de salidas

Desde la pantalla de configuración es posible probar las salidas antes de utilizar el sistema definitivo.

Para salidas digitales se dispone de controles de:

```text
ON / OFF
```

Para salidas PWM se puede modificar directamente el valor de salida.

Esto permite comprobar conexiones y GPIO desde la propia interfaz.

---

# Filtro por MAC Address

Por defecto, el receptor puede aceptar los mensajes enviados mediante broadcast.

Opcionalmente puede activarse un filtro por dirección MAC.

En ese caso se especifica la MAC del ESP8266 autorizado.

Esto permite que el ESP32 ignore mensajes provenientes de otros emisores.

---

# Configuración persistente

La configuración se almacena utilizando **EEPROM**.

Esto permite conservar las cards y parámetros configurados aunque el ESP32 se reinicie o pierda alimentación.

Entre los datos almacenados se encuentran:

- cards;
- entradas;
- salidas;
- GPIO;
- modos;
- inversión de lógica;
- tiempos de retención;
- filtro MAC.

---

# Conexión electrónica utilizada en las pruebas

> **Importante:** las siguientes conexiones corresponden únicamente al prototipo utilizado para validar el sistema.

## ESP8266 - Emisor

### Pulsadores

```text
GPIO5  ---- Pulsador ---- GND
GPIO4  ---- Pulsador ---- GND
GPIO14 ---- Pulsador ---- GND
```

Equivalencias en NodeMCU:

```text
D1 = GPIO5
D2 = GPIO4
D5 = GPIO14
```

### Potenciómetro

```text
3V3 ---- extremo del potenciómetro

A0  ---- terminal central

GND ---- extremo del potenciómetro
```

---

## ESP32 - Receptor

Las salidas pueden asignarse desde la interfaz web.

Por ejemplo:

```text
GPIO25 ---- Resistencia ---- LED ---- GND
```

Para otros actuadores debe utilizarse la electrónica de potencia o adaptación correspondiente cuando sea necesaria.

---

# Instalación del entorno

El proyecto fue desarrollado utilizando **Arduino IDE**.

## Soporte ESP32

Desde Arduino IDE:

```text
Herramientas
→ Placa
→ Gestor de placas
```

Buscar:

```text
esp32 by Espressif Systems
```

e instalar el paquete correspondiente.

## Soporte ESP8266

Instalar el soporte para placas ESP8266 en Arduino IDE.

Guía utilizada durante el desarrollo:

[Usando ESP8266 con Arduino IDE - Naylamp Mechatronics](https://naylampmechatronics.com/blog/56_usando-esp8266-con-el-ide-de-arduino.html)

---

# Programación de las placas

## ESP8266 - Emisor

Archivo:

```text
ESP8266_EMISOR_v6.ino
```

Placa utilizada:

```text
NodeMCU 1.0 (ESP-12E Module)
```

## ESP32 - Receptor + Web

Archivo:

```text
ESP32_RECEPTOR_WEB_v6.ino
```

Placa:

```text
ESP32 Dev Module
```

---

# Puesta en marcha

1. Cargar el firmware del emisor en el ESP8266.
2. Cargar el firmware del receptor en el ESP32.
3. Alimentar ambas placas.
4. Esperar a que el ESP32 genere la red WiFi.
5. Desde un celular o computadora buscar:

```text
ESP-REMOTE-XXXX
```

6. Conectarse utilizando:

```text
12345678
```

7. Abrir en el navegador:

```text
192.168.4.1
```

8. Entrar a **Configuración**.
9. Crear o modificar las cards.
10. Seleccionar entradas y salidas.
11. Guardar la configuración.
12. Probar el funcionamiento desde el circuito físico o desde el dashboard.

---

# Ejemplo de configuración

## Pulsador

Entrada:

```text
ESP8266
D1 / GPIO5
Tipo: Digital
```

Salida:

```text
ESP32
GPIO25
Tipo: Digital
```

Resultado:

```text
Pulsador GPIO5
       │
       │ ESP-NOW
       ▼
    ESP32
       │
       ▼
Salida GPIO25
```

## Potenciómetro

Entrada:

```text
ESP8266
A0 / ADC0
Tipo: Analógica
```

Salida:

```text
ESP32
GPIO configurado
Tipo: PWM
```

El movimiento del potenciómetro modifica proporcionalmente el valor PWM de la salida.

---

# Demostración

Se realizó una demostración del prototipo donde puede observarse:

- funcionamiento de las entradas físicas;
- comunicación inalámbrica mediante ESP-NOW;
- respuesta de las salidas;
- dashboard;
- configuración mediante la interfaz web;
- control desde celular o computadora.

▶️ **[Ver demostración del proyecto](URL_DE_LA_DEMOSTRACION)**

> Reemplazar `URL_DE_LA_DEMOSTRACION` por el enlace definitivo al video.

---

# Aplicaciones posibles

ESP Remote fue desarrollado como una base adaptable a diferentes proyectos.

Algunas posibles aplicaciones son:

- Instalaciones artísticas interactivas.
- Performances audiovisuales.
- Instrumentos musicales experimentales.
- Control inalámbrico de iluminación.
- Interacción con sensores.
- Activación remota de dispositivos.
- Prototipos de interacción física.
- Electrónica creativa y educativa.
- Automatizaciones.
- Domótica.

El objetivo no es definir una única aplicación, sino ofrecer una estructura configurable sobre la cual puedan construirse diferentes sistemas interactivos.

---

# Pruebas realizadas

Durante el desarrollo se probaron:

- Comunicación ESP-NOW entre ESP8266 y ESP32.
- Transmisión ESP-NOW Broadcast.
- Entradas digitales.
- Entrada analógica A0.
- Salidas digitales.
- Salidas PWM.
- Conversión analógica 0-1023 a PWM 0-255.
- Modos llave y pulsador.
- Retención temporizada.
- Inversión de lógica.
- Control desde dashboard.
- Cards sin entrada física.
- Prueba de salidas desde la configuración.
- Filtro por MAC Address.
- Almacenamiento de configuración en EEPROM.
- Creación y eliminación de cards.
- Asociación configurable entre GPIO de entrada y salida.

---

# Autoría

**Gonzalo Lucero**

Proyecto académico desarrollado en 2026 en el marco del grupo de estudio:

**“Aplicación de recursos de Internet de las cosas en proyectos de Artes Electrónicas”**

Laboratorio de Arte Electrónico e Inteligencia Artificial (LAEIA)  
Universidad Nacional de Tres de Febrero (UNTREF)
