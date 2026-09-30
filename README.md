# ESP Remote - Sistema de Control Inalámbrico mediante ESP-NOW

## Introducción

**ESP Remote** es un sistema de control inalámbrico desarrollado con ESP8266 y ESP32 que permite relacionar entradas físicas con salidas remotas mediante ESP-NOW.

El proyecto busca simplificar el uso de microcontroladores en instalaciones interactivas, permitiendo configurar el funcionamiento del sistema desde una interfaz web sin tener que modificar y recompilar el código para cada nueva configuración.

Está pensado especialmente para artistas, músicos, performers, realizadores audiovisuales, estudiantes y usuarios que quieran incorporar electrónica e interacción a sus proyectos sin necesitar conocimientos avanzados de programación.

En la versión actual:

- **ESP8266 NodeMCU:** funciona como emisor.
- **ESP32:** funciona como receptor y servidor de la interfaz web.
- **ESP-NOW:** realiza la comunicación inalámbrica entre ambas placas.
- **Interfaz web:** permite configurar las relaciones entre entradas y salidas.

---

# Índice

1. Acceso al sistema
2. Objetivo
3. Arquitectura
4. Tecnologías utilizadas
5. Hardware del prototipo
6. Funcionamiento general
7. ESP8266 emisor
8. ESP32 receptor
9. Comunicación ESP-NOW
10. Modo Broadcast
11. Interfaz web
12. Concepto de Módulos
13. Modos digitales
14. Control PWM
15. Dashboard
16. Filtro por MAC Address
17. Configuración persistente
18. Conexión electrónica
19. Instalación del entorno
20. Programación de las placas
21. Puesta en marcha
22. Ejemplo de configuración
23. Demostración
24. Aplicaciones posibles
25. Pruebas realizadas
26. Autoría

---

# Acceso al sistema

Una vez programadas y encendidas las placas, el **ESP32 receptor genera automáticamente una red WiFi propia**.

## 1. Conectarse a la red WiFi

Desde una computadora, celular o tablet buscar una red con el siguiente formato:

```text
ESP-REMOTE-XXXX
```

`XXXX` corresponde a los últimos caracteres utilizados para identificar la dirección MAC del ESP32.

Por ejemplo:

```text
ESP-REMOTE-A4F2
```

La contraseña de la red es:

```text
12345678
```

## 2. Acceder a la interfaz

Una vez conectado a la red `ESP-REMOTE-XXXX`, abrir un navegador e ingresar:

```text
http://192.168.4.1
```

El procedimiento es el mismo desde una computadora, celular o tablet.

```text
CELULAR / COMPUTADORA
        │
        │ WiFi
        ▼
  ESP-REMOTE-XXXX
        │
        ▼
   192.168.4.1
        │
        ▼
     ESP Remote
```

No es necesario tener conexión a Internet.

El ESP32 genera su propia red WiFi y aloja localmente la interfaz web.

> Si el celular indica que la red no tiene acceso a Internet, se debe permanecer conectado igualmente. Esto es normal, ya que la red se utiliza para comunicarse directamente con el ESP32.

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
- Facilitar el uso de sistemas embebidos a personas sin conocimientos avanzados de programación.

---

# Arquitectura

La arquitectura actual del sistema es:

```text
Entradas físicas
2 switches + 1 potenciómetro
        │
        ▼
ESP8266 NodeMCU - EMISOR
        │
        │ ESP-NOW
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
- 2 switches
- 1 potenciómetro
- LEDs para pruebas
- Resistencias
- Protoboard
- Cables

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

- 1 ESP8266 NodeMCU
- 1 ESP32
- 2 switches
- 1 potenciómetro
- LEDs de prueba
- Resistencias de 220 Ω
- Protoboard
- Cables

Estos componentes corresponden solamente al **prototipo utilizado para probar y validar el funcionamiento del sistema**.

ESP Remote no está limitado a switches, potenciómetros o LEDs. El sistema puede adaptarse a otros sensores, actuadores y dispositivos compatibles.

---

# Funcionamiento general

El ESP8266 funciona como **emisor** y lee las entradas físicas.

En el prototipo se utilizan:

```text
2 switches
+
1 potenciómetro
```

Cuando el ESP8266 detecta un cambio en una entrada, genera un mensaje y lo transmite inalámbricamente mediante ESP-NOW.

El ESP32 funciona como **receptor**.

Cuando recibe el mensaje, identifica la entrada que produjo el cambio y busca si existe un módulo configurado para esa entrada.

Si encuentra una relación, ejecuta la salida correspondiente.

Por ejemplo:

```text
Switch
   │
GPIO ESP8266
   │
   │ ESP-NOW
   ▼
 ESP32
   │
GPIO de salida
   │
   ▼
Dispositivo
```

La relación entre una entrada y una salida puede modificarse desde la interfaz web sin tener que cambiar el código.

---

# ESP8266 NodeMCU - Emisor

El ESP8266 funciona como dispositivo emisor.

En el prototipo final se utilizan:

- 2 entradas digitales para los switches.
- 1 entrada analógica para el potenciómetro.

La entrada analógica utiliza:

```text
A0
```

Cuando cambia el estado de una entrada, el ESP8266 transmite la información mediante ESP-NOW.

Esto permite enviar tanto estados digitales como valores variables obtenidos desde una entrada analógica.

---

# ESP32 - Receptor y servidor web

El ESP32 cumple dos funciones principales:

1. Recibir los mensajes enviados mediante ESP-NOW.
2. Generar la red WiFi y la interfaz web de ESP Remote.

Cuando recibe información del ESP8266, compara el tipo y GPIO de entrada con los módulos configurados.

Si encuentra una coincidencia, controla la salida correspondiente.

El ESP32 puede trabajar con:

- Salidas digitales.
- Salidas PWM.

También permite controlar salidas directamente desde el dashboard web.

---

# Comunicación ESP-NOW

La comunicación entre las placas utiliza **ESP-NOW**.

ESP-NOW permite que los microcontroladores se comuniquen directamente sin depender de un router WiFi ni de una conexión a Internet.

Cada mensaje permite identificar:

- Tipo de entrada.
- GPIO de entrada.
- Valor digital.
- Valor analógico.

De esta manera, el receptor puede determinar qué entrada generó el evento y utilizar su valor para controlar la salida configurada.

---

# Modo Broadcast

El ESP8266 transmite utilizando la dirección:

```text
FF:FF:FF:FF:FF:FF
```

Esto corresponde a una transmisión **Broadcast**.

El emisor no necesita conocer previamente la dirección MAC del receptor.

Cualquier receptor compatible que se encuentre escuchando en el mismo canal puede recibir el mensaje.

El ESP32 puede posteriormente decidir si acepta o ignora esos mensajes mediante el filtro por MAC Address.

---

# Interfaz web

La interfaz web se encuentra alojada directamente en el ESP32.

Para ingresar:

```text
WiFi:
ESP-REMOTE-XXXX

Contraseña:
12345678

Dirección:
http://192.168.4.1
```

Desde la interfaz se puede:

- Crear módulos.
- Editar módulos.
- Eliminar módulos.
- Seleccionar GPIO.
- Configurar entradas.
- Configurar salidas.
- Probar salidas.
- Configurar PWM.
- Configurar el comportamiento de las entradas digitales.
- Configurar el filtro por MAC.
- Controlar salidas desde el dashboard.

El objetivo de esta interfaz es que gran parte de la configuración pueda realizarse sin modificar directamente el código fuente.

---

# Concepto de Módulos

El sistema utiliza el concepto de **módulos**.

Cada módulo representa una relación configurable entre una entrada y una salida.

Por ejemplo:

```text
Entrada ESP8266
      │
      ▼
    MÓDULO
      │
      ▼
Salida ESP32
```

Un módulo permite definir:

- Nombre.
- Uso o no de una entrada física.
- Tipo de entrada.
- GPIO de entrada.
- Tipo de salida.
- GPIO de salida.
- Modo switch.
- Modo pulsador.
- Tiempo de retención.
- Lógica invertida.
- Salida digital.
- Salida PWM.

La versión actual permite crear hasta:

```text
8 módulos
```

Cada módulo puede configurarse de manera independiente.

Los GPIO utilizados por otros módulos se identifican desde la interfaz para evitar configuraciones duplicadas.

También existe una opción **Manual** que permite ingresar directamente un GPIO.

---

# Módulos sin entrada física

También es posible crear un módulo sin asociarlo a una entrada física.

En ese caso, la salida puede controlarse directamente desde el dashboard utilizando un celular o una computadora.

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

Esto permite utilizar ESP Remote no solamente como receptor de controles físicos, sino también como sistema de control remoto mediante una interfaz web.

---

# Modos digitales

Las entradas digitales pueden configurarse con diferentes comportamientos.

## Modo switch

La salida responde al estado del switch.

```text
Switch ON  → Salida ON
Switch OFF → Salida OFF
```

## Modo pulsador

Una entrada digital también puede configurarse para funcionar como pulsador.

En este modo es posible definir un tiempo de retención.

Al liberar el pulsador comienza a contar el tiempo configurado antes de apagar la salida.

Por ejemplo:

```text
Retención: 1000 ms
```

La salida permanece activa durante un segundo después de liberar el pulsador.

---

# Control PWM

El potenciómetro permite generar un valor analógico desde el ESP8266.

```text
Potenciómetro
      │
      ▼
     A0
      │
      ▼
ESP8266
      │
   ESP-NOW
      ▼
ESP32
      │
      ▼
Salida PWM
```

El valor recibido puede utilizarse para controlar de manera progresiva una salida compatible con PWM.

Por ejemplo, puede utilizarse para modificar la intensidad de un LED durante las pruebas.

---

# Dashboard

La pantalla principal funciona como un dashboard.

Desde allí se visualizan los módulos configurados y el estado de las salidas.

Dependiendo de la configuración de cada módulo se puede:

- Encender una salida.
- Apagar una salida.
- Accionar una salida digital.
- Modificar un valor PWM.

Las acciones pueden realizarse directamente desde un celular, tablet o computadora.

---

# Prueba de salidas

Desde la pantalla de configuración también es posible probar las salidas.

Para las salidas digitales se dispone de controles para encenderlas y apagarlas.

Para las salidas PWM se puede modificar directamente el valor de salida.

Esto permite verificar el funcionamiento de un GPIO antes de utilizarlo dentro del sistema definitivo.

---

# Filtro por MAC Address

El sistema permite utilizar un filtro opcional por dirección MAC.

Sin filtro, el receptor puede aceptar los mensajes ESP-NOW enviados mediante Broadcast.

Con el filtro activado se puede indicar qué ESP8266 está autorizado a controlar el receptor.

Esto resulta útil cuando existen varios dispositivos ESP trabajando dentro de un mismo espacio.

---

# Configuración persistente

La configuración realizada desde la interfaz se almacena mediante EEPROM.

Esto permite conservar la configuración aunque el ESP32 sea apagado o reiniciado.

Se almacenan datos como:

- Módulos.
- GPIO.
- Tipos de entrada.
- Tipos de salida.
- Modos de funcionamiento.
- Tiempo de retención.
- Inversión.
- Filtro MAC.

De esta manera no es necesario volver a configurar el sistema cada vez que se enciende.

---

# Conexión electrónica del prototipo

> **Importante:** las siguientes conexiones corresponden al prototipo utilizado para realizar las pruebas. No representan una limitación del sistema.

## ESP8266 - Emisor

El montaje utilizado para las pruebas incluye:

```text
2 switches
1 potenciómetro
```

### Switches

Los switches se conectan entre sus GPIO correspondientes y GND.

```text
GPIO ---- Switch ---- GND
```

Las entradas digitales utilizan la resistencia interna `INPUT_PULLUP` del ESP8266.

### Potenciómetro

El potenciómetro se conecta de la siguiente manera:

```text
3V3 ---- extremo del potenciómetro

A0  ---- terminal central

GND ---- extremo del potenciómetro
```

---

# ESP32 - Receptor

Las salidas utilizadas durante las pruebas fueron conectadas a GPIO configurados desde la interfaz.

Para comprobar visualmente el funcionamiento se utilizaron LEDs con resistencias.

```text
GPIO ---- Resistencia 220 Ω ---- LED ---- GND
```

Estos LEDs se utilizaron solamente para visualizar fácilmente el estado de las salidas durante el desarrollo.

Para controlar motores, iluminación, relés u otros dispositivos puede ser necesario utilizar una etapa electrónica adicional adecuada al dispositivo conectado.

---

# Instalación del entorno

El proyecto fue desarrollado utilizando **Arduino IDE**.

Antes de cargar los programas es necesario instalar el soporte correspondiente para las placas ESP32 y ESP8266.

---

## Instalación de ESP32

En Arduino IDE ingresar a:

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

Una vez instalado se podrá seleccionar:

```text
ESP32 Dev Module
```

para programar el receptor.

---

# Instalación de ESP8266 / NodeMCU

Para poder programar la placa NodeMCU utilizada como emisor es necesario instalar el soporte para **ESP8266** en Arduino IDE.

La instalación agrega al entorno las definiciones y librerías necesarias para compilar y cargar programas en placas basadas en ESP8266.

Una guía para realizar la instalación se encuentra en:

[Usando ESP8266 con Arduino IDE - Naylamp Mechatronics](https://naylampmechatronics.com/blog/56_usando-esp8266-con-el-ide-de-arduino.html)

Una vez instalado el soporte, seleccionar:

```text
Herramientas
→ Placa
→ ESP8266 Boards
→ NodeMCU 1.0 (ESP-12E Module)
```

Este paso es necesario para poder compilar y cargar correctamente el firmware del emisor.

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

## ESP32 - Receptor + interfaz web

Archivo:

```text
ESP32_RECEPTOR_WEB_v6.ino
```

Placa utilizada:

```text
ESP32 Dev Module
```

Cada archivo debe cargarse en la placa correspondiente mediante Arduino IDE.

---

# Puesta en marcha

Una vez cargado el firmware en ambas placas:

1. Encender el ESP8266.
2. Encender el ESP32.
3. Desde un celular, tablet o computadora buscar la red:

```text
ESP-REMOTE-XXXX
```

4. Conectarse utilizando la contraseña:

```text
12345678
```

5. Abrir un navegador.

6. Ingresar:

```text
http://192.168.4.1
```

7. Abrir la sección de configuración.

8. Crear o modificar los módulos.

9. Seleccionar las entradas y salidas.

10. Guardar la configuración.

11. Volver al dashboard.

12. Probar el funcionamiento utilizando los switches, el potenciómetro o los controles de la interfaz web.

---

# Ejemplo de configuración

## Módulo digital

Un switch conectado al ESP8266 puede asociarse con una salida del ESP32.

```text
Switch
   │
   ▼
GPIO ESP8266
   │
   │ ESP-NOW
   ▼
ESP32
   │
   ▼
GPIO de salida
```

Desde la interfaz se selecciona:

```text
Entrada: Digital
GPIO de entrada: GPIO correspondiente del ESP8266

Salida: Digital
GPIO de salida: GPIO correspondiente del ESP32
```

---

## Módulo analógico

El potenciómetro conectado a A0 puede asociarse con una salida PWM.

```text
Potenciómetro
      │
      ▼
     A0
      │
      ▼
ESP8266
      │
   ESP-NOW
      ▼
ESP32
      │
      ▼
Salida PWM
```

El movimiento del potenciómetro modifica progresivamente el valor de la salida.

Estas asociaciones pueden configurarse desde la interfaz web sin modificar el firmware.

---

# Demostración

Se realizó una demostración del prototipo final donde puede observarse el funcionamiento completo del sistema:

- Conexión a la red `ESP-REMOTE-XXXX`.
- Acceso mediante `192.168.4.1`.
- Interfaz web.
- Creación y configuración de módulos.
- Funcionamiento de los dos switches.
- Funcionamiento del potenciómetro.
- Comunicación mediante ESP-NOW.
- Respuesta de las salidas.
- Control desde el dashboard.
- Configuración desde celular o computadora.

▶️ **[Ver demostración del funcionamiento de ESP Remote]https://www.youtube.com/watch?v=NwaHQkucDmQ**

> Reemplazar `URL_DE_LA_DEMOSTRACION` por el enlace definitivo al video.

---

# Aplicaciones posibles

ESP Remote fue desarrollado como una plataforma adaptable a diferentes proyectos.

Algunas posibles aplicaciones son:

- Instalaciones artísticas interactivas.
- Performances audiovisuales.
- Instrumentos musicales experimentales.
- Control inalámbrico de iluminación.
- Interacción mediante sensores.
- Activación remota de dispositivos.
- Prototipos de interacción física.
- Electrónica creativa.
- Experiencias educativas.
- Automatizaciones.
- Domótica.

El objetivo no es definir una única aplicación, sino ofrecer una estructura configurable sobre la cual puedan desarrollarse diferentes sistemas interactivos.

---

# Pruebas realizadas

Durante el desarrollo se probaron:

- Comunicación ESP-NOW entre ESP8266 y ESP32.
- Transmisión mediante Broadcast.
- Dos entradas digitales mediante switches.
- Entrada analógica mediante potenciómetro.
- Salidas digitales.
- Salidas PWM.
- Configuración mediante interfaz web.
- Acceso desde celular.
- Acceso desde computadora.
- Dashboard.
- Creación de módulos.
- Eliminación de módulos.
- Módulos con entrada física.
- Módulos sin entrada física.
- Modo switch.
- Modo pulsador.
- Retención temporizada.
- Inversión de lógica.
- Control PWM.
- Prueba de salidas desde la interfaz.
- Filtro por MAC Address.
- Persistencia mediante EEPROM.
- Asociación configurable entre GPIO de entrada y salida.

---

# Autoría

**Gonzalo Lucero**

Proyecto académico desarrollado en 2026 en el marco del grupo de estudio:

**“Aplicación de recursos de Internet de las cosas en proyectos de Artes Electrónicas”**

Laboratorio de Arte Electrónico e Inteligencia Artificial (LAEIA)

Universidad Nacional de Tres de Febrero (UNTREF)
