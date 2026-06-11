### 1. System Setup (Configuración del Sistema) {#system-setup-configuración-del-sistema}

En esta etapa debes definir la arquitectura de hardware y cómo se conectará el microcontrolador al display. Lo más común y eficiente en cuanto a pines es usar el **modo de 4 bits**.

- **Pines de Control:**

  - **RS (Register Select):** 0 para Comandos, 1 para Datos.

  - **RW (Read/Write):** Generalmente se conecta a GND (siempre escribimos al LCD) para ahorrar un pin.

  - **EN (Enable):** Pulso para confirmar la lectura de datos.

- **Pines de Datos:** D4, D5, D6, y D7.

- **Temporización (Timings):** El LCD requiere tiempos de espera específicos (delays) entre comandos, especialmente durante la inicialización. Como vamos a usar un statechart, la idea es **evitar retardos bloqueantes** (como delay_ms()) y usar temporizadores no bloqueantes basados en un \"Tick\" del sistema.

### 2. Modeling - Statechart (Modelado de Estados) {#modeling---statechart-modelado-de-estados}

El objetivo de usar un Statechart (Máquina de Estados Finita - FSM) es que el microcontrolador no se quede \"congelado\" esperando a que el LCD procese la información.

Puedes modelar el sistema con los siguientes estados principales:

1.  **LCD_STATE_POWER_ON**: Estado inicial. Espera unos 40ms a que el voltaje del LCD se estabilice.

2.  **LCD_STATE_INIT**: Secuencia de inicialización (enviar comandos de configuración de 4 bits, apagar cursor, limpiar pantalla). Este estado puede tener sub-estados debido a los tiempos de espera requeridos entre cada comando.

3.  **LCD_STATE_IDLE**: El LCD está listo y esperando que haya un nuevo caracter o comando en el buffer para ser enviado.

4.  **LCD_STATE_SEND_UPPER_NIBBLE**: Envía los 4 bits más significativos del dato/comando y activa el pin EN.

5.  **LCD_STATE_SEND_LOWER_NIBBLE**: Envía los 4 bits menos significativos del dato/comando y activa el pin EN.

6.  **LCD_STATE_WAIT**: Espera el tiempo necesario (típicamente \~40µs a 2ms) para que el LCD procese el dato antes de volver a IDLE.

### 3. C Coding & Porting (Implementación en C) {#c-coding-porting-implementación-en-c}

Para que el código sea **portable** (fácil de llevar de un STM32 a un PIC, AVR o ESP32), debes separar estrictamente la lógica de estado (independiente del hardware) de la manipulación de pines (dependiente del hardware).

Esto se logra mediante una **Capa de Abstracción de Hardware (HAL)** utilizando punteros a funciones o macros.

#### 

#### A. Abstracción de Hardware (Porting Layer) {#a.-abstracción-de-hardware-porting-layer}

Primero, defines las funciones que el usuario deberá proveer según su microcontrolador:

> C
>
> // lcd_port.h\
> \
> typedef struct {\
> void (\*set_rs)(uint8_t state);\
> void (\*set_en)(uint8_t state);\
> void (\*set_data_pins)(uint8_t nibble);\
> uint32_t (\*get_tick_ms)(void); // Para el control de tiempos no bloqueante\
> } LCD_HardwareConfig_t;\
> \
> // Función para registrar los callbacks del hardware\
> void LCD_InitHardware(LCD_HardwareConfig_t \*hw_config);

#### B. Implementación de la Máquina de Estados (State Machine) {#b.-implementación-de-la-máquina-de-estados-state-machine}

Luego, el código principal evalúa el estado. Esta función debería llamarse periódicamente en el bucle principal (main loop) o en un timer.

> [C]{.mark}
>
> [// lcd_fsm.c\
> \
> typedef enum {\
> LCD_STATE_POWER_ON,\
> LCD_STATE_INIT,\
> LCD_STATE_IDLE,\
> LCD_STATE_SEND_UPPER,\
> LCD_STATE_SEND_LOWER,\
> LCD_STATE_WAIT\
> } LCD_State_t;\
> \
> static LCD_State_t current_state = LCD_STATE_POWER_ON;\
> static uint32_t timer_reference = 0;\
> static LCD_HardwareConfig_t \*hw;\
> \
> void LCD_Task(void) {\
> if (!hw) return; // Protección si no se configuró el hardware\
> ]{.mark}
>
> [\
> switch (current_state) {\
> case LCD_STATE_POWER_ON:\
> if (hw-\>get_tick_ms() - timer_reference \>= 40) {\
> // Pasaron 40ms, iniciar secuencia\
> current_state = LCD_STATE_INIT;\
> timer_reference = hw-\>get_tick_ms();\
> }\
> break;\
> \
> case LCD_STATE_INIT:\
> // Aquí iría la lógica de inicialización en 4-bits\...\
> // Una vez terminado:\
> current_state = LCD_STATE_IDLE;\
> break;\
> \
> case LCD_STATE_IDLE:\
> // Revisar si hay datos en un buffer para enviar\
> // Si hay datos: current_state = LCD_STATE_SEND_UPPER;\
> break;\
> \
> case LCD_STATE_SEND_UPPER:\
> // hw-\>set_rs(\...);\
> // hw-\>set_data_pins(data \>\> 4);\
> // hw-\>set_en(1); luego hw-\>set_en(0);\
> current_state = LCD_STATE_SEND_LOWER;\
> break;\
> \
> case LCD_STATE_SEND_LOWER:\
> // hw-\>set_data_pins(data & 0x0F);\
> // hw-\>set_en(1); luego hw-\>set_en(0);\
> timer_reference = hw-\>get_tick_ms();\
> current_state = LCD_STATE_WAIT;\
> break;\
> \
> case LCD_STATE_WAIT:\
> if (hw-\>get_tick_ms() - timer_reference \>= 2) { // Esperar 2ms\
> current_state = LCD_STATE_IDLE;\
> }\
> break;\
> }\
> }]{.mark}

### Arquitectura General del Sistema

El código implementa un planificador cooperativo guiado por tiempo (time-triggered) mediante un \"tick\" del sistema (generalmente de 1 milisegundo) generado por un timer de hardware (SysTick). Esto permite ejecutar múltiples tareas de forma concurrente (aparentemente simultánea) sin depender de un Sistema Operativo de Tiempo Real (RTOS).

Las tareas se ejecutan secuencialmente dentro de app_update(), y es responsabilidad de cada tarea no ser bloqueante (no usar delays activos largos como bucles while vacíos). Para lograr esto, la lógica de las tareas se implementa mediante máquinas de estados (Statecharts).

### 

### Análisis de Archivos Core del Sistema

#### 1. app.c (Capa de Aplicación y Planificador) {#app.c-capa-de-aplicación-y-planificador}

Este es el motor principal del sistema. Define cómo se inicializan y se ejecutan las tareas.

- **Lista de Tareas:** Define un arreglo de estructuras task_cfg_list\[\]. Cada estructura contiene punteros a las funciones de inicialización y actualización de cada tarea (task_test y task_display).

- **Métricas de Desempeño:** Define task_dta_list para hacer un perfil de rendimiento de cada tarea:

  - NOE: Número de ejecuciones.

  - LET: Tiempo de la última ejecución.

  - BCET / WCET: Tiempo de ejecución en el mejor y peor caso (Best/Worst-Case Execution Time). Esto es crucial en sistemas embebidos de tiempo real para asegurar que ninguna tarea acapare el CPU por demasiado tiempo.

- **app_init():** Inicializa variables, contadores de ciclos (DWT para medir tiempos precisos en microsegundos) y llama a la función \_init de cada tarea registrada en la lista. Por último, inicializa las interrupciones (app_it_init).

- **app_update():** Es el bucle de ejecución principal. Revisa la variable g_app_tick_cnt (que incrementa el SysTick). Si hubo al menos un tick, procesa *todas* las tareas llamando a sus funciones \_update. Mide cuánto tarda cada tarea en ejecutarse y actualiza las métricas (LET, BCET, WCET).

#### 

#### 2. app_it.c (Manejo de Interrupciones) {#app_it.c-manejo-de-interrupciones}

Gestiona las interrupciones a nivel de aplicación.

7.  **HAL_SYSTICK_Callback():** Esta función se ejecuta automáticamente en cada pulso del timer de hardware (típicamente cada 1ms). Su único trabajo es incrementar la variable global g_app_tick_cnt. Esta variable es la que app_update lee para saber si es hora de ejecutar las tareas.

8.  También incluye un esqueleto para manejar interrupciones externas (HAL_GPIO_EXTI_Callback), útil si se agregaran botones en el futuro.

#### 

#### 3. systick.c (Delay Bloqueante) {#systick.c-delay-bloqueante}

Proporciona la función systick_delay_us().

- Implementa un retardo *bloqueante* (busy-waiting) preciso a nivel de microsegundos leyendo directamente los registros de hardware del periférico SysTick (SysTick-\>VAL).

- **Nota:** Aunque el objetivo del sistema general es ser *no bloqueante*, esta función es a menudo necesaria durante la inicialización de hardware de bajo nivel (como la del LCD), donde los tiempos de espera requeridos son del orden de microsegundos (demasiado rápidos para manejarlos con el tick de 1ms de la máquina de estados principal) y la inicialización suele hacerse antes de que empiece el bucle principal.

### 

### Análisis de Archivos del Display

#### 4. display.c y display.h (Driver de Bajo Nivel del LCD) {#display.c-y-display.h-driver-de-bajo-nivel-del-lcd}

Este módulo se encarga de la comunicación directa con el hardware físico del LCD (presumiblemente un controlador HD44780 o compatible). **No usa máquinas de estado; es un código procedural de configuración.**

- **displayInit():** Realiza la compleja secuencia de inicialización que requiere el controlador LCD. Define comandos para configurar el tamaño de fuente, encender/apagar el cursor, limpiar la pantalla, y establecer el modo de operación (4 bits o 8 bits). Se aprecia el uso de delays bloqueantes (aunque el código usa una función genérica delay() en lugar de systick_delay_us(), el concepto es el mismo) porque esta secuencia tiene requerimientos de temporización estrictos.

- **displayCharPositionWrite():** Calcula la dirección de memoria interna (DDRAM) del LCD basándose en la fila (Y) y la columna (X) solicitada y envía el comando para mover el cursor. Contempla un display de hasta 4 filas.

- **displayStringWrite():** Envía caracteres al LCD uno por uno usando iteración de punteros.

- **Abstracción de Hardware:** Usa funciones como displayPinWrite y displayDataBusWrite para abstraer los pines del microcontrolador. Depende de variables de tipo DigitalOut (típico del entorno mbed) para controlar el estado de los pines. Se puede configurar para trabajar en modo de 4 bits o de 8 bits a través del parámetro de inicialización displayConnection_t.

#### 

#### 

#### 5. task_display_interface.c {#task_display_interface.c}

Proporciona una interfaz limpia para que otras tareas se comuniquen con la tarea del display sin modificar su estado directamente.

- **put_event_task_display():** Esta función es llamada por otras tareas (como task_test) cuando quieren mostrar algo. Escribe el mensaje deseado en un buffer interno de la tarea de display (p_task_display_dta-\>ddram) y levanta una bandera (flag) indicando que hay un evento de actualización pendiente (EV_DSP_UPDATE).

#### 

#### 6. task_display.c (Tarea Gestora de la Pantalla) {#task_display.c-tarea-gestora-de-la-pantalla}

Esta tarea es responsable de enviar el contenido del buffer interno al driver físico del LCD.

- **task_display_init():** Inicializa la máquina de estados en modo ocioso (ST_DSP_IDLE), limpia las variables y llama a displayInit(DISPLAY_CONNECTION_GPIO_4BITS) para inicializar el hardware en modo de 4 bits. Luego, hace una escritura inicial en el display (presumiblemente vacía o con los datos por defecto del buffer).

- **task_display_update():** Es un envoltorio que simplemente llama a la máquina de estados.

- *(El comportamiento de la máquina de estados se detalla más abajo).*

### 

### Análisis de la Tarea de Prueba (Task Test)

#### 7. task_test.c y task_test_attribute.h {#task_test.c-y-task_test_attribute.h}

Es una tarea diseñada para generar tráfico de prueba hacia el display.

- **task_test_init():** Configura el timer inicial y escribe un mensaje estático (\"LCD Display Test\", \"Porting C code\") usando la función de interfaz put_event_task_display.

- *(El comportamiento de su máquina de estados se detalla a continuación).*

### Análisis de las Máquinas de Estado

#### Comportamiento de void task_test_statechart(void)

Esta función actúa como un temporizador no bloqueante implementado por software, encargado de actualizar un contador en la pantalla periódicamente.

1.  **Conteo Continuo:** Cada vez que la función es llamada, incrementa un contador global interno de la tarea (counter).

2.  **Temporización (Tick Down):** Utiliza la variable tick como un temporizador en cuenta regresiva. Si tick es mayor a 0, simplemente lo decrementa. Si la tarea se llama cada 1ms, esto representa un milisegundo por decremento.

3.  **Evento de Disparo:** Cuando tick llega a 0 (después de 1000 iteraciones, dado que DEL_TEST_XX_MAX es 1000), el temporizador expira. En ese momento:

    - Recarga el temporizador tick con su valor máximo (DEL_TEST_XX_MAX), reiniciando el ciclo.

    - Prepara una cadena de texto base en la línea 1: \"Test Nro: \*\*\*\*\*\*\".

    - Calcula un número de prueba dividiendo los ciclos totales (counter) por el valor máximo (DEL_TEST_XX_MAX). Básicamente, está calculando cuántos segundos (o ciclos de temporizador) han pasado.

    - Convierte ese número a texto usando snprintf y lo envía al display para que se escriba encima de los asteriscos a partir de la columna 10.

En resumen: Esta máquina de estados espera 1000 ticks (generalmente 1 segundo si el tick del sistema es 1ms) y, cuando ese tiempo pasa, le envía un nuevo número formateado a la tarea del display para que lo actualice.

#### 

#### Comportamiento de void task_display_statechart(void)

Esta función gestiona el proceso de escritura en el LCD de manera reactiva, basándose en banderas (flags) que indican si hay información nueva. Tiene dos estados principales:

1.  **Estado ST_DSP_IDLE (Ocioso):**

    - La tarea permanece aquí sin hacer nada hasta que detecta una solicitud.

    - Revisa constantemente si se ha levantado la bandera flag y si el evento actual es EV_DSP_UPDATE. Esta bandera y evento son modificados por la función put_event_task_display() cuando otra tarea quiere mostrar algo.

    - Si detecta el evento, hace la transición al estado ST_DSP_UPDATE.

2.  **Estado ST_DSP_UPDATE (Actualizando):**

    - Verifica nuevamente que el evento sea válido.

    - Baja (limpia) la bandera flag a false para indicar que el evento está siendo procesado.

    - Utiliza las funciones de bajo nivel del driver (displayCharPositionWrite y displayStringWrite) para volcar el contenido completo de su buffer interno (las filas ddram\[0\] y ddram\[1\]) hacia el hardware físico del LCD. Primero posiciona el cursor en el inicio de la línea 0 y manda la cadena de esa línea, luego hace lo mismo para la línea 1.

    - Una vez terminada la escritura, regresa el estado de la tarea a ST_DSP_IDLE, quedando a la espera de un nuevo evento.

En resumen: Esta máquina espera sentada hasta que alguien le dice \"tengo datos nuevos\". Cuando eso ocurre, toma su buffer de memoria, lo envía completo al LCD, y vuelve a sentarse a esperar.
