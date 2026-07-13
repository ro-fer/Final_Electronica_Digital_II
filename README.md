# ECG con sensor AD8232 - Electrónica Digital II

Proyecto final de la materia **Electrónica Digital II**, desarrollado utilizando una placa **Blue Pill STM32F103C8T6** y un módulo **AD8232** para la adquisición de una señal de electrocardiograma (ECG).

El objetivo del proyecto fue adquirir una señal analógica proveniente del sensor, procesarla mediante el ADC del microcontrolador y calcular la frecuencia cardíaca en tiempo real. Además, se incorporó una indicación visual mediante LEDs para representar distintos rangos de BPM.

## Autora

**Fernández, Rocío Belén**
Estudiante de Ingeniería Biomédica
Universidad Nacional de San Martín - UNSAM

## Descripción general

El sistema implementado permite:

* Adquirir la señal ECG proveniente del módulo AD8232.
* Digitalizar la señal mediante el ADC de la Blue Pill.
* Detectar picos R de la señal electrocardiográfica.
* Calcular la frecuencia cardíaca en BPM.
* Detectar la desconexión de electrodos mediante las salidas `LO+` y `LO-` del módulo AD8232.
* Transmitir el valor de BPM por UART hacia una computadora.
* Indicar el estado de la medición y el rango de frecuencia cardíaca mediante LEDs.

El programa fue desarrollado en lenguaje C, trabajando directamente sobre registros del microcontrolador, sin utilizar librerías HAL. La estructura principal se basa en una máquina de estados no bloqueante, sincronizada mediante interrupciones.

## Componentes utilizados

* Placa Blue Pill STM32F103C8T6
* Sensor ECG AD8232
* Programador ST-LINK/V2
* Conversor USB-UART
* LEDs indicadores
* Resistencias
* Protoboard
* Electrodos para ECG
* Fuente de alimentación USB

## Conexiones principales

| Pin Blue Pill | Función                              |
| ------------- | ------------------------------------ |
| PA1           | Entrada ADC - señal ECG del AD8232   |
| PB0           | Entrada digital LO+                  |
| PB1           | Entrada digital LO-                  |
| PA9           | TX USART1                            |
| PA10          | RX USART1                            |
| PB5           | LED rojo - electrodos desconectados  |
| PB6           | LED amarillo - señal débil           |
| PB7           | LED verde - señal correcta           |
| PB8 a PB12    | LEDs indicadores de rango de BPM     |
| PC13          | LED onboard / indicador de actividad |

La comunicación UART se realiza de forma cruzada:

* `PA9 / TX` de la Blue Pill se conecta al `RX` del conversor USB-UART.
* `PA10 / RX` de la Blue Pill se conecta al `TX` del conversor USB-UART.
* GND de la Blue Pill se conecta al GND del conversor.

## Funcionamiento del programa

El programa realiza las siguientes etapas:

1. Configura el reloj principal del microcontrolador a 72 MHz.
2. Configura el temporizador SysTick para generar una frecuencia de muestreo de 250 Hz.
3. Inicializa el ADC para adquirir la señal analógica del AD8232.
4. Lee el estado de los pines `LO+` y `LO-` para verificar si los electrodos están conectados.
5. Procesa la señal ECG para detectar los picos R.
6. Calcula la frecuencia cardíaca en BPM a partir del intervalo entre latidos.
7. Actualiza los LEDs indicadores según el estado de la señal y el rango de BPM.
8. Transmite el valor calculado por UART en el formato:

```text
BPM=075
```

## Máquina de estados

El programa se organiza mediante una máquina de estados con tres estados principales:

| Estado      | Descripción                                                                         |
| ----------- | ----------------------------------------------------------------------------------- |
| `S_SYSTICK` | Espera la interrupción periódica del SysTick para iniciar una nueva conversión ADC. |
| `S_ADC`     | Espera el fin de conversión del ADC y procesa la muestra obtenida.                  |
| `S_TXDATA`  | Transmite por UART el valor de BPM calculado.                                       |

Esta estructura permite que el sistema funcione de manera ordenada y sin retardos bloqueantes.

## Detección de frecuencia cardíaca

Para detectar los latidos se implementó un algoritmo basado en un umbral adaptativo. Esto permite ajustar la detección según la amplitud de la señal adquirida, ya que esta puede variar dependiendo de la persona, la posición de los electrodos y la calidad del contacto.

El algoritmo incluye:

* Estimación de línea de base.
* Cálculo de una envolvente adaptativa.
* Umbral dinámico de detección.
* Criterio de pendiente mínima.
* Histéresis para evitar múltiples detecciones sobre el mismo pico.
* Período refractario para evitar falsos latidos.
* Validación de BPM dentro de un rango fisiológicamente plausible.

## Indicación mediante LEDs

Se incorporaron LEDs para visualizar de forma rápida el rango de frecuencia cardíaca calculado:

| Frecuencia cardíaca | LEDs encendidos |
| ------------------- | --------------- |
| BPM < 60            | 1 LED           |
| 60 ≤ BPM < 80       | 2 LEDs          |
| 80 ≤ BPM < 100      | 3 LEDs          |
| 100 ≤ BPM < 120     | 4 LEDs          |
| BPM ≥ 120           | 5 LEDs          |

Además, se utilizaron LEDs de estado para indicar:

* Electrodo desconectado.
* Señal débil.
* Señal correcta.

## Comunicación UART

La comunicación serial se configuró mediante USART1 a **115200 bps**.

La salida puede visualizarse desde una terminal serial, por ejemplo PuTTY, conectando la Blue Pill a la computadora mediante un conversor USB-UART.

Configuración utilizada:

* Baud rate: 115200
* Bits de datos: 8
* Paridad: ninguna
* Bits de stop: 1

## Archivos del repositorio

* `Fernandez_Rocio.c`: código fuente principal del proyecto.
* `TP_Final_ED2_FernandezRocio.pdf`: informe final del trabajo práctico.
* `Fotos-videos/`: material audiovisual del montaje y funcionamiento del sistema.
* `README.md`: descripción general del proyecto.

## Resultados

Durante las pruebas se verificó:

* Adquisición de señal desde el módulo AD8232.
* Cálculo de frecuencia cardíaca en tiempo real.
* Transmisión correcta del BPM por UART.
* Encendido de LEDs según el rango de frecuencia cardíaca.
* Detección de desconexión de electrodos mediante `LO+` y `LO-`.

En condiciones de reposo se obtuvieron valores de frecuencia cardíaca acordes a lo esperado. Al desconectar intencionalmente un electrodo, el sistema informó `BPM=000`, encendió el LED rojo y reinició el detector para evitar cálculos inválidos.

## Limitaciones

Este proyecto fue desarrollado con fines educativos. No cuenta con aislación médica certificada, validación clínica ni mecanismos de seguridad necesarios para su uso como equipo médico real.

Por lo tanto, no debe utilizarse para diagnóstico ni monitoreo clínico. Su finalidad es integrar conceptos de adquisición de señales analógicas, procesamiento digital básico, manejo de interrupciones, comunicación serial y sistemas embebidos.

## Referencias

* Código de referencia de la cátedra de Electrónica Digital II.
* Analog Devices - Datasheet AD8232.
* STMicroelectronics - Reference Manual STM32F10xxx.
