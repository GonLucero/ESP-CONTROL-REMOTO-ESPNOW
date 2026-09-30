# ESP Remote - Sistema de Control Inalámbrico mediante ESP-NOW

## Introducción

**ESP Remote** es un sistema de control inalámbrico desarrollado con ESP8266 y ESP32 que permite configurar relaciones entre entradas físicas y salidas remotas mediante ESP-NOW.

El objetivo principal del proyecto es que estas relaciones puedan configurarse desde una **interfaz web**, sin necesidad de modificar y recompilar el código cada vez que se quiera cambiar el funcionamiento del sistema.

Está pensado especialmente para artistas, músicos, performers, realizadores audiovisuales, estudiantes y usuarios que quieran incorporar electrónica e interacción a sus proyectos sin necesitar conocimientos avanzados de programación.

La idea es que cada usuario pueda adaptar el sistema a su propio proyecto, seleccionando desde la interfaz qué entradas y salidas desea utilizar y cómo deben comportarse.

> **Importante:** para desarrollar y comprobar el funcionamiento de ESP Remote se construyó un prototipo de prueba con **2 switches y 1 potenciómetro como entradas, y 3 LEDs como salidas**.
>
> Esta configuración corresponde únicamente al montaje utilizado para desarrollar y validar el proyecto. **No representa una configuración obligatoria de ESP Remote.**
>
> El objetivo del sistema es justamente permitir que cada usuario configure sus propios GPIO, entradas, salidas y comportamientos según las necesidades de su proyecto.

En la versión desarrollada:

- **ESP8266 NodeMCU:** funciona como emisor.
- **ESP32:** funciona como receptor y servidor de la interfaz web.
- **ESP-NOW:** realiza la comunicación inalámbrica entre ambas placas.
- **Interfaz web:** permite configurar las relaciones entre entradas y salidas.

---

# Índice

1. Acceso al sistema
2. Objetivo
3. Arquitectura general
4. Prototipo utilizado para las pruebas
5. Tecnologías utilizadas
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
18. Conexión del prototipo de prueba
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

Una vez programadas y encendidas las placas, el **ESP32 genera automáticamente una red WiFi propia**.

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

La contraseña es:

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

> Si un celular indica que la red no tiene acceso a Internet, se debe permanecer conectado igualmente. Esto es normal, ya que la red se utiliza para comunicarse directamente con el ESP32.

---

# Objetivo

El objetivo principal de ESP Remote es facilitar la creación y configuración de sistemas electrónicos inalámbricos.

En lugar de definir de manera fija en el código qué entrada controla cada salida, ESP Remote permite realizar estas asociaciones desde una interfaz gráfica.

El sistema permite:

- Comunicar microcontroladores mediante ESP-NOW.
- Asociar entradas físicas con salidas remotas.
- Seleccionar GPIO desde una interfaz web.
- Trabajar con señales digitales y analógicas.
- Controlar salidas digitales y PWM.
- Configurar diferentes comportamientos para las entradas.
- Modificar configuraciones sin recompilar el firmware.
- Controlar salidas directamente desde un celular o computadora.
- Guardar la configuración para conservarla después de reiniciar el dispositivo.
- Facilitar el uso de sistemas embebidos a personas sin conocimientos avanzados de programación.

El objetivo no es proporcionar un circuito único, sino una **base configurable que cada usuario pueda adaptar a su proyecto**.

---

# Arquitectura general

La arquitectura general de ESP Remote puede representarse de la siguiente manera:

```text
ENTRADAS
   │
   │
   ▼
ESP8266 - EMISOR
   │
   │ ESP-NOW
   ▼
ESP32 - RECEPTOR
   │
   ├────────► SALIDAS
   │
   └────────► INTERFAZ WEB
                    │
                    ▼
             CELULAR / PC
```

Las **entradas** pueden ser diferentes dispositivos compatibles con el sistema.

Las **salidas** también pueden variar dependiendo del proyecto.

Por lo tanto, la arquitectura no define que deban utilizarse necesariamente switches, potenciómetros o LEDs.

Esos componentes fueron utilizados solamente para construir el prototipo de prueba.

La idea es que el usuario pueda definir desde la interfaz:

```text
ENTRADA
   │
   ▼
GPIO seleccionado
   │
   ▼
MÓDULO CONFIGURADO
   │
   ▼
GPIO de salida
   │
   ▼
SALIDA
```

---

# Prototipo utilizado para las pruebas

Para desarrollar y validar ESP Remote se construyó una configuración sencilla que permitiera comprobar diferentes tipos de señales.

El prototipo utiliza:

## Entradas

```text
2 switches
1 potenciómetro
```

conectados al ESP8266.

## Salidas

```text
3 LEDs
```

conectados al ESP32.

El objetivo de este montaje fue poder comprobar:

- Estados digitales.
- Valores analógicos.
- Comunicación inalámbrica.
- Salidas digitales.
- Salidas PWM.
- Configuración desde la web.

Por ejemplo:

```text
PROTOTIPO DE PRUEBA

2 switches ──┐
             │
Potenciómetro├──► ESP8266
             │
             └──── ESP-NOW ────► ESP32 ────► 3 LEDs
```

> Este circuito es únicamente un ejemplo de implementación.
>
> **ESP Remote no está diseñado específicamente para controlar tres LEDs mediante dos switches y un potenciómetro.**
>
> Estos componentes fueron elegidos porque permiten comprobar de manera simple y visual el funcionamiento del sistema.

---

# Tecnologías utilizadas

## Hardware utilizado durante el desarrollo

- ESP8266 NodeMCU
- ESP32
- 2 switches
- 1 potenciómetro
- 3 LEDs
- Resistencias
- Protoboard
- Cables

Los switches, potenciómetro y LEDs corresponden al prototipo de prueba.

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

# Funcionamiento general

ESP Remote divide el sistema en dos partes principales.

El **ESP8266 funciona como emisor** y se encarga de leer las entradas.

El **ESP32 funciona como receptor** y controla las salidas configuradas.

Cuando el ESP8266 detecta un cambio en una entrada, transmite la información mediante ESP-NOW.

El ESP32 recibe el mensaje y busca si existe un módulo configurado para esa entrada.

Si encuentra una relación, ejecuta la salida correspondiente.

```text
ENTRADA
   │
   ▼
ESP8266
   │
   │ ESP-NOW
   ▼
ESP32
   │
   ▼
SALIDA
```

La relación entre entrada y salida se configura desde la interfaz web.

Por lo tanto, esa relación no necesita quedar definida permanentemente dentro del código.

---

# ESP8266 NodeMCU - Emisor

El ESP8266 funciona como dispositivo emisor.

Su función es:

1. Leer las entradas físicas.
2. Detectar cambios.
3. Generar un mensaje.
4. Enviarlo mediante ESP-NOW.

En el prototipo utilizado para las pruebas se conectaron:

```text
2 entradas digitales → switches

1 entrada analógica → potenciómetro
```

La entrada analógica utiliza:

```text
A0
```

Esta es la configuración utilizada para demostrar el funcionamiento, pero las entradas digitales y sus GPIO pueden configurarse de acuerdo con las posibilidades contempladas por el sistema.

---

# ESP32 - Receptor y servidor web

El ESP32 cumple principalmente dos funciones:

1. Recibir los mensajes enviados mediante ESP-NOW.
2. Generar la red WiFi y la interfaz web de ESP Remote.

Cuando recibe información del ESP8266, compara los datos recibidos con los módulos configurados.

Si encuentra una coincidencia, ejecuta la salida correspondiente.

El sistema permite trabajar con:

- Salidas digitales.
- Salidas PWM.

En el prototipo se utilizaron **3 LEDs** para visualizar fácilmente el resultado.

Los LEDs no son una parte obligatoria del sistema y pueden ser reemplazados por otros dispositivos utilizando la electrónica necesaria para cada caso.

---

# Comunicación ESP-NOW

La comunicación entre las placas utiliza **ESP-NOW**.

ESP-NOW permite que los microcontroladores se comuniquen directamente sin depender de un router WiFi ni de una conexión a Internet.

Los mensajes permiten identificar información como:

- Tipo de entrada.
- GPIO de entrada.
- Valor digital.
- Valor analógico.

De esta manera, el receptor puede determinar qué entrada generó el evento y utilizar el valor recibido para controlar la salida configurada.

---

# Modo Broadcast

El ESP8266 transmite utilizando la dirección:

```text
FF:FF:FF:FF:FF:FF
```

Esto corresponde a una transmisión **Broadcast**.

El emisor no necesita conocer previamente la dirección MAC del receptor.

El ESP32 puede decidir si acepta o ignora los mensajes mediante el filtro por MAC Address.

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
- Seleccionar comportamientos.
- Probar salidas.
- Configurar PWM.
- Configurar el filtro por MAC.
- Controlar salidas desde el dashboard.

La interfaz es una parte central del proyecto, ya que permite modificar el comportamiento del sistema **sin necesidad de editar directamente el código fuente**.

---

# Concepto de Módulos

ESP Remote utiliza el concepto de **módulos**.

Cada módulo representa una relación configurable entre una entrada y una salida.

```text
ENTRADA
   │
   ▼
MÓDULO
   │
   ▼
SALIDA
```

Cada módulo puede configurarse de manera independiente.

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

También existe una opción **Manual** para ingresar directamente un GPIO.

Esto permite que el usuario construya diferentes configuraciones sin que el prototipo original determine obligatoriamente cómo debe utilizarse el sistema.

---

# Módulos sin entrada física

Un módulo también puede configurarse sin una entrada física.

En este caso la salida se controla directamente desde el dashboard.

```text
CELULAR / PC
      │
      │ WiFi
      ▼
    ESP32
      │
      ▼
    SALIDA
```

De esta manera ESP Remote también puede utilizarse como sistema de control remoto desde un navegador.

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

En este modo puede definirse un tiempo de retención.

Por ejemplo:

```text
Retención: 1000 ms
```

Al liberar el pulsador comienza a contar el tiempo configurado antes de apagar la salida.

---

# Control PWM

Una entrada analógica puede asociarse a una salida PWM.

En el prototipo se utilizó un potenciómetro conectado a A0 para comprobar este funcionamiento.

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

Durante las pruebas, esta salida se visualizó mediante uno de los LEDs.

El potenciómetro y el LED son solamente los dispositivos utilizados para demostrar el comportamiento analógico/PWM.

---

# Dashboard

La pantalla principal funciona como un dashboard.

Desde allí se visualizan los módulos configurados y el estado de sus salidas.

Dependiendo de la configuración de cada módulo es posible:

- Encender una salida.
- Apagar una salida.
- Accionar una salida digital.
- Modificar un valor PWM.

El dashboard puede utilizarse desde un celular, tablet o computadora.

---

# Prueba de salidas

Desde la configuración también es posible probar las salidas.

Para una salida digital se puede comprobar su encendido y apagado.

Para una salida PWM se puede modificar directamente su valor.

Esta función permite verificar que un GPIO y el dispositivo conectado funcionan correctamente antes de utilizar el módulo.

---

# Filtro por MAC Address

ESP Remote permite utilizar un filtro opcional por dirección MAC.

Sin el filtro, el receptor puede aceptar los mensajes enviados mediante Broadcast.

Con el filtro activado se puede indicar qué ESP8266 está autorizado a controlar el receptor.

Esto permite utilizar varios dispositivos en un mismo espacio y restringir qué emisor controla cada sistema.

---

# Configuración persistente

La configuración realizada desde la interfaz se almacena mediante EEPROM.

Esto permite conservarla aunque el ESP32 sea apagado o reiniciado.

Se almacenan datos como:

- Módulos.
- GPIO.
- Tipos de entrada.
- Tipos de salida.
- Modos de funcionamiento.
- Tiempo de retención.
- Inversión.
- Filtro MAC.

De esta manera no es necesario volver a realizar toda la configuración cada vez que se enciende el sistema.

---

# Conexión electrónica del prototipo de prueba

> **Esta sección describe exclusivamente el circuito construido para probar ESP Remote.**
>
> No representa una conexión obligatoria para utilizar el sistema.

## ESP8266 - Emisor

Para las pruebas se utilizaron:

```text
2 switches
1 potenciómetro
```

### Switches

Los switches se conectaron entre los GPIO utilizados y GND.

```text
GPIO ---- Switch ---- GND
```

Las entradas digitales utilizan `INPUT_PULLUP`.

### Potenciómetro

```text
3V3 ---- extremo del potenciómetro

A0  ---- terminal central

GND ---- extremo del potenciómetro
```

---

# ESP32 - Receptor

Para las pruebas se utilizaron **3 LEDs** como salidas.

Cada LED se conectó utilizando una resistencia:

```text
GPIO ---- Resistencia 220 Ω ---- LED ---- GND
```

Los LEDs permiten observar visualmente los cambios producidos por los switches y el potenciómetro.

En una aplicación diferente, estas salidas podrían utilizarse para controlar otros dispositivos, teniendo en cuenta las características eléctricas y la electrónica necesaria para cada uno.

---

# Instalación del entorno

El proyecto fue desarrollado utilizando **Arduino IDE**.

Antes de cargar los programas es necesario instalar el soporte correspondiente para ESP32 y ESP8266.

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

---

# Instalación de ESP8266 / NodeMCU

Para programar el NodeMCU utilizado como emisor es necesario instalar el soporte para **ESP8266** en Arduino IDE.

Este paquete incorpora las definiciones y librerías necesarias para compilar y cargar el firmware en placas basadas en ESP8266.

Guía utilizada:

[Usando ESP8266 con Arduino IDE - Naylamp Mechatronics](https://naylampmechatronics.com/blog/56_usando-esp8266-con-el-ide-de-arduino.html)

Una vez instalado el soporte seleccionar:

```text
Herramientas
→ Placa
→ ESP8266 Boards
→ NodeMCU 1.0 (ESP-12E Module)
```

---

# Programación de las placas

## ESP8266 - Emisor

Archivo:

```text
ESP8266_EMISOR_v6.ino
```

Placa:

```text
NodeMCU 1.0 (ESP-12E Module)
```

## ESP32 - Receptor + interfaz web

Archivo:

```text
ESP32_RECEPTOR_WEB_v6.ino
```

Placa:

```text
ESP32 Dev Module
```

Cada archivo debe cargarse en la placa correspondiente mediante Arduino IDE.

---

# Puesta en marcha

Una vez cargado el firmware en ambas placas:

1. Encender el ESP8266.
2. Encender el ESP32.
3. Desde un celular, tablet o computadora buscar:

```text
ESP-REMOTE-XXXX
```

4. Conectarse utilizando:

```text
12345678
```

5. Abrir un navegador.

6. Ingresar:

```text
http://192.168.4.1
```

7. Entrar a la configuración.

8. Crear los módulos necesarios.

9. Seleccionar las entradas y salidas que se quieran utilizar.

10. Seleccionar sus GPIO.

11. Configurar su comportamiento.

12. Guardar la configuración.

13. Volver al dashboard y probar el sistema.

---

# Ejemplo de configuración

Los siguientes ejemplos corresponden al prototipo de prueba y sirven únicamente para mostrar el funcionamiento.

## Ejemplo digital

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
LED
```

Desde la interfaz se configura qué GPIO de entrada debe controlar qué GPIO de salida.

## Ejemplo analógico

```text
Potenciómetro
      │
      ▼
     A0
      │
   ESP-NOW
      ▼
ESP32
      │
      ▼
LED mediante PWM
```

En otra implementación el usuario puede definir otras entradas, salidas y GPIO compatibles sin utilizar necesariamente estos componentes.

---

# Demostración

Se realizó una demostración utilizando el **prototipo de prueba de 2 switches, 1 potenciómetro y 3 LEDs**.

En el video puede observarse:

- Conexión a `ESP-REMOTE-XXXX`.
- Acceso mediante `192.168.4.1`.
- Interfaz web.
- Creación y configuración de módulos.
- Funcionamiento de los dos switches.
- Funcionamiento del potenciómetro.
- Comunicación mediante ESP-NOW.
- Respuesta de los tres LEDs.
- Control desde el dashboard.

▶️ **[Ver demostración del funcionamiento de ESP Remote](URL_DE_LA_DEMOSTRACION)**

> Reemplazar `URL_DE_LA_DEMOSTRACION` por el enlace definitivo al video.

---

# Aplicaciones posibles

ESP Remote no fue desarrollado para una única aplicación específica.

El prototipo con switches, potenciómetro y LEDs funciona como una **prueba de concepto** de una plataforma que puede adaptarse a distintos proyectos.

Algunas posibles aplicaciones son:

- Instalaciones artísticas interactivas.
- Performances audiovisuales.
- Instrumentos musicales experimentales.
- Control inalámbrico de iluminación.
- Sistemas con sensores.
- Activación remota de dispositivos.
- Prototipos de interacción física.
- Electrónica creativa.
- Experiencias educativas.
- Automatización.
- Domótica.

La intención es que cada usuario pueda partir del sistema base y configurar sus propios módulos según las necesidades de su proyecto.

---

# Autoría

**Gonzalo Lucero**

Proyecto académico desarrollado en 2026 en el marco del grupo de estudio:

**“Aplicación de recursos de Internet de las cosas en proyectos de Artes Electrónicas”**

Laboratorio de Arte Electrónico e Inteligencia Artificial (LAEIA)

Universidad Nacional de Tres de Febrero (UNTREF)
