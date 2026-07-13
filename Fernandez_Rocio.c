/// PROYECTO: ELECTROCARDIÓGRAFO CON AD8232
// Alumna: Rocio Fernandez - UNSAM - Ingenieria Biomedica
//  Electrónica Digital II - 2do Cuatrimestre 2024

//  Este programa toma la señal analógica del AD8232 con el ADC, detecta
// los picos R de la señal ECG y calcula la frecuencia cardíaca en BPM.
// El resultado se envía por USART1 con el formato: BPM=YYY

// Profesores :  Miguel Ángel Sagreras - Álvarez, Nicolás
// Código base original: Miguel Ángel Sagreras (adc.c)

#define SRAM_SIZE		((uint32_t) 0x00005000)
#define SRAM_BASE		((uint32_t) 0x20000000)
#define STACKINIT		((interrupt_t)(SRAM_BASE+SRAM_SIZE))

typedef int			   int32_t;
typedef short		   int16_t;
typedef char		   int8_t;
typedef unsigned int   uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char  uint8_t;

typedef void(*interrupt_t)(void);

typedef union {
	uint8_t  byte[4];
	uint16_t hword[2];
	uint32_t word;
} word_t;

typedef word_t page[0x400/sizeof(uint32_t)];

// ============================================================
// Mapa de memoria
// ============================================================

enum {TIM2	= 0, TIM3  = 1, TIM4  = 2 };
enum {GPIOA = 0, GPIOB = 1, GPIOC = 2, GPIOD = 3, GPIOE = 4, GPIOF = 5 };
enum {DMA1	= 0 };
enum {CHN1	= 0, CHN2  = 1, CHN3  = 2, CHN4  = 3, CHN5	= 4, CHN6  = 5, CHN7 = 6, CHN8 = 7 };
enum {ADC1	= 0 };
struct {
	union {
		struct {
			uint32_t CR1;
			uint32_t CR2;
			uint32_t SMCR;
			uint32_t DIER;
			uint32_t SR;
			uint32_t EGR;
			uint32_t CCMR1;
			uint32_t CCMR2;
			uint32_t CCER;
			uint32_t CNT;
			uint32_t PSC;
			uint32_t ARR;
			uint32_t RES1;
			uint32_t CCR1;
			uint32_t CCR2;
			uint32_t CCR3;
			uint32_t CCR4;
			uint32_t BDTR;
			uint32_t DCR;
			uint32_t DMAR;
		} REGs;
		page reserved;
	} TIMs[3];

	word_t reserved1[(0x40002800-0x40000c00)/sizeof(word_t)];
	page RTC;
	page WWDG;
	page IWDG;
	word_t reserved2[(0x40003800-0x40003400)/sizeof(word_t)];
	page SPI2;
	word_t reserved3[(0x40004400-0x40003c00)/sizeof(word_t)];
	page USART[2];
	word_t reserved4[(0x40005400-0x40004c00)/sizeof(word_t)];
	page I2C[2];
	page USB;
	page USBCAN_SRAM;
	page bxCAN;
	word_t reserved5[(0x40006c00-0x40006800)/sizeof(word_t)];
	page BKP;
	page PWR;
	word_t reserved6[(0x40010000-0x40007400)/sizeof(word_t)];
	page AFIO;
	page EXTI;
	union {
		struct {
			uint32_t CRL;
			uint32_t CRH;
			uint32_t IDR;
			uint32_t ODR;
			uint32_t BSRR;
			uint32_t BRR;
			uint32_t LCKR;
		} REGs;
		page reserved;
	} GPIOs[5];
	word_t reserved7[(0x40012400-0x40011C00)/sizeof(word_t)];
	union {
		struct {
			uint32_t SR;
			uint32_t CR1;
			uint32_t CR2;
			uint32_t SMPR1;
			uint32_t SMPR2;
			uint32_t JOFR[4];
			uint32_t HTR;
			uint32_t LTR;
			uint32_t SQR1;
			uint32_t SQR2;
			uint32_t SQR3;
			uint32_t JSQR;
			uint32_t JDR[4];
			uint32_t DR;
		} REGs;
		page reserved;
	} ADC[2];
	page TIM1;
	page SPI1;
	word_t reserved8[(0x40013800-0x40013400)/sizeof(word_t)];
	union  {
		struct {
			uint32_t SR;
			uint32_t DR;
			uint32_t BRR;
			uint32_t CR1;
			uint32_t CR2;
			uint32_t CR3;
			uint32_t GTPR;
		} REGs;
		page reserved;
	} USART1;
	word_t reserved9[(0x40020000-0x40013C00)/sizeof(word_t)];
	union {
		struct {
			uint32_t ISR;
			uint32_t IFCR;
			struct {
				uint32_t CCR;
				uint32_t CNDTR;
				uint32_t CPAR;
				uint32_t CMAR;
				uint32_t RESERVED;
			} CHN[8];
		} REGs;
		page reserved;
	} DMAs[1];
	word_t reservedA[(0x40021000-0x40020400)/sizeof(word_t)];
	union {
		struct {
			uint32_t CR;
			uint32_t CFGR;
			uint32_t CIR;
			uint32_t APB2RSTR;
			uint32_t APB1RSTR;
			uint32_t AHBENR;
			uint32_t APB2ENR;
			uint32_t APB1ENR;
			uint32_t BDCR;
			uint32_t CSR;
			uint32_t AHBRSTR;
			uint32_t CFGR2;
		} REGs;
		page reserved;
	} RCC;
	word_t reservedB[(0x40022000-0x40021400)/sizeof(word_t)];
	union {
		struct {
			uint32_t ACR;
			uint32_t KEYR;
			uint32_t OPTKEYR;
			uint32_t SR;
			uint32_t CR;
			uint32_t AR;
			uint32_t reserved;
			uint32_t OBR;
			uint32_t WRPR;
		} REGs;
		page reserved;
	} FLASH;
} volatile *const DEVMAP = (void *) 0x40000000;

#define ENA_IRQ(IRQ) {CTX->NVIC.REGs.ISER[((uint32_t)(IRQ) >> 5)] = (1 << ((uint32_t)(IRQ) & 0x1F));}
#define DIS_IRQ(IRQ) {CTX->NVIC.REGs.ICER[((uint32_t)(IRQ) >> 5)] = (1 << ((uint32_t)(IRQ) & 0x1F));}
#define CLR_IRQ(IRQ) {CTX->NVIC.REGs.ICPR[((uint32_t)(IRQ) >> 5)] = (1 << ((uint32_t)(IRQ) & 0x1F));}

struct {
	word_t reversed0[(0xe000e010-0xe0000000)/sizeof(word_t)];
	union {
		struct {
			uint32_t CSR;
			uint32_t RVR;
			uint32_t CVR;
			uint32_t CALIB;
		} REGs;
	} SYSTICK;
	word_t reversed1[(0xe000e100-0xe000e020)/sizeof(word_t)];
	union {
		struct {
			uint32_t ISER[8];
			uint32_t RES0[24];
			uint32_t ICER[8];
			uint32_t RES1[24];
			uint32_t ISPR[8];
			uint32_t RES2[24];
			uint32_t ICPR[8];
			uint32_t RES3[24];
			uint32_t IABR[8];
			uint32_t RES4[56];
			uint8_t  IPR[240];
			uint32_t RES5[644];
			uint32_t STIR;
		} REGs;
	} NVIC;
} volatile *const CTX = ((void *) 0xE0000000);

enum IRQs {
	IRQ_ADC1_2	  = 18,
	IRQ_USART1	  = 37,
};

int  main(void);
void handler_systick(void);
void handler_adc1_2(void);
void handler_usart1(void);

// Funciones agregadas para procesar la señal ECG y mostrar el resultado.
void actualizar_deteccion_bpm(uint32_t muestra_adc);
void mostrar_bpm_en_leds(uint32_t bpm);
void reiniciar_deteccion_bpm(void);
uint32_t paso_minimo(uint32_t diferencia, uint32_t shift);

// ============================================================
// Tabla de vectores de interrupción
// ============================================================
const interrupt_t vector_table[] __attribute__ ((section(".vtab"))) = {
	STACKINIT,												// 0x0000_0000 Stack Pointer
	(interrupt_t) main,										// 0x0000_0004 Reset
	0,														// 0x0000_0008
	0,														// 0x0000_000C
	0,														// 0x0000_0010
	0,														// 0x0000_0014
	0,														// 0x0000_0018
	0,														// 0x0000_001C
	0,														// 0x0000_0020
	0,														// 0x0000_0024
	0,														// 0x0000_0028
	0,														// 0x0000_002C
	0,														// 0x0000_0030
	0,														// 0x0000_0034
	0,														// 0x0000_0038
	handler_systick,										// 0x0000_003C SYSTICK
	0,														// 0x0000_0040
	0,														// 0x0000_0044
	0,														// 0x0000_0048
	0,														// 0x0000_004C
	0,														// 0x0000_0050
	0,														// 0x0000_0054
	0,														// 0x0000_0058
	0,														// 0x0000_005C
	0,														// 0x0000_0060
	0,														// 0x0000_0064
	0,														// 0x0000_0068
	0,														// 0x0000_006C
	0,														// 0x0000_0070
	0,														// 0x0000_0074
	0,														// 0x0000_0078
	0,														// 0x0000_007C
	0,														// 0x0000_0080
	0,														// 0x0000_0084
	handler_adc1_2,											// 0x0000_0088
	0,														// 0x0000_008C
	0,														// 0x0000_0090
	0,														// 0x0000_0094
	0,														// 0x0000_0098
	0,														// 0x0000_009C
	0,														// 0x0000_00A0
	0,														// 0x0000_00A4
	0,														// 0x0000_00A8
	0,														// 0x0000_00AC
	0,														// 0x0000_00B0 TIM2
	0,														// 0x0000_00B4
	0,														// 0x0000_00B8
	0,														// 0x0000_00BC
	0,														// 0x0000_00C0
	0,														// 0x0000_00C4
	0,														// 0x0000_00C8
	0,														// 0x0000_00CC
	0,														// 0x0000_00D0
	handler_usart1,											// 0x0000_00D4 USART1
	0,														// 0x0000_00D8
	0,														// 0x0000_00DC
};

// ============================================================
// Interrupción de SysTick
// Se usa como base de tiempo para tomar muestras periódicas.
// ============================================================
volatile uint32_t tick;
volatile uint32_t tock;
void handler_systick(void)
{
	tick = !tock;
}

// ============================================================
// Interrupción del ADC
// Se ejecuta cuando termina una conversión.
// ============================================================
volatile uint32_t adc1_2_rdy;
volatile uint32_t adc1_2_req;
void handler_adc1_2(void)
{
	adc1_2_rdy = adc1_2_req;
	DEVMAP->ADC[ADC1].REGs.SR &= ~(1 << 1);				// Limpia el bit EOC
	CLR_IRQ(IRQ_ADC1_2);
}

// ============================================================
// Interrupción de USART1
// Se usa para transmitir la trama por UART sin bloquear el programa.
// ============================================================
volatile uint32_t tx_rdy;
volatile uint32_t tx_req;
void handler_usart1(void)
{
	tx_rdy = tx_req;
	DEVMAP->USART1.REGs.CR1 &= ~(1 << 7);					// Deshabilita interrupción TXE
	CLR_IRQ(IRQ_USART1);
}

volatile uint32_t ecg_baseline_x16          = (2048u << 4); // valor inicial cercano al punto medio del ADC
volatile uint32_t ecg_envelope_x16          = (2048u << 4);
volatile uint32_t ecg_estado_sobre_umbral   = 0;             // indica si la señal ya cruzó el umbral de detección
volatile uint32_t ecg_muestras_refractario  = 0;             // evita detectar dos veces el mismo latido
volatile uint32_t ecg_muestras_desde_latido = 0;             // se usa para calcular el intervalo RR
volatile uint32_t ecg_bpm_actual            = 0;             // último valor de frecuencia cardíaca calculado
volatile uint32_t ecg_primer_latido_pendiente = 1;           // el primer pico solo se usa como referencia
volatile uint32_t ecg_muestra_anterior_x16 = (2048u << 4);    // muestra anterior para estimar la pendiente
volatile uint32_t ecg_pendiente_actual_x16 = 0;               // pendiente calculada entre dos muestras consecutivas

// Constantes principales del detector de picos R.
// La señal se muestrea a 250 Hz, por lo tanto cada muestra representa 4 ms.
#define MUESTRAS_REFRACTARIAS   80          // 80 muestras * 4 ms = 320 ms -> evita re-disparos demasiado seguidos
#define ECG_RANGO_MINIMO_X16    (60u << 4)  // amplitud mínima para aceptar detección
#define ECG_PENDIENTE_MIN_X16   (120u << 4) // pendiente mínima para aceptar un QRS real

#define BPM_MINIMO_PLAUSIBLE    30
#define BPM_MAXIMO_PLAUSIBLE   220

#define TX_OFFSET_BPM    4
#define TX_BUFFER_LEN    9


void mostrar_bpm_en_leds(uint32_t bpm)
{
	uint32_t leds_a_encender;

	if (bpm < 60)        leds_a_encender = 1;
	else if (bpm < 80)   leds_a_encender = 2;
	else if (bpm < 100)  leds_a_encender = 3;
	else if (bpm < 120)  leds_a_encender = 4;
	else                 leds_a_encender = 5;

	// Primero apago todos los LEDs de BPM y después enciendo la cantidad necesaria.
	DEVMAP->GPIOs[GPIOB].REGs.ODR &= ~((1<<8)|(1<<9)|(1<<10)|(1<<11)|(1<<12));
	if (leds_a_encender >= 1) DEVMAP->GPIOs[GPIOB].REGs.ODR |= (1 << 8);
	if (leds_a_encender >= 2) DEVMAP->GPIOs[GPIOB].REGs.ODR |= (1 << 9);
	if (leds_a_encender >= 3) DEVMAP->GPIOs[GPIOB].REGs.ODR |= (1 << 10);
	if (leds_a_encender >= 4) DEVMAP->GPIOs[GPIOB].REGs.ODR |= (1 << 11);
	if (leds_a_encender >= 5) DEVMAP->GPIOs[GPIOB].REGs.ODR |= (1 << 12);
}

// -----------------------------------------------------------
uint32_t paso_minimo(uint32_t diferencia, uint32_t shift)
{
	uint32_t paso = diferencia >> shift;

	// Como se trabaja con enteros, el desplazamiento puede dar cero para diferencias chicas.
	// Este mínimo evita que la línea de base o la envolvente queden congeladas.
	if ((paso == 0) && (diferencia > 0))
		paso = 1;

	return paso;
}

// -----------------------------------------------------------
void actualizar_deteccion_bpm(uint32_t muestra_adc)
{
	uint32_t muestra_x16 = muestra_adc << 4;
	uint32_t umbral_subida_x16;
	uint32_t umbral_bajada_x16;
	uint32_t margen_histeresis_x16;
	uint32_t rango_x16;
	uint32_t diff;
	uint32_t pendiente_x16;

	// Calculo la pendiente positiva de la señal.
	// El complejo QRS suele tener una subida rápida, a diferencia de variaciones lentas o ruido.
	if (muestra_x16 > ecg_muestra_anterior_x16) {
		pendiente_x16 = muestra_x16 - ecg_muestra_anterior_x16;
	} else {
		pendiente_x16 = 0;
	}
	ecg_muestra_anterior_x16 = muestra_x16;
	ecg_pendiente_actual_x16 = pendiente_x16;

	// La línea de base se actualiza lentamente para acompañar cambios de continua.
	if (muestra_x16 > ecg_baseline_x16) {
		diff = muestra_x16 - ecg_baseline_x16;
		ecg_baseline_x16 += paso_minimo(diff, 6);
	} else {
		diff = ecg_baseline_x16 - muestra_x16;
		ecg_baseline_x16 -= paso_minimo(diff, 6);
	}

	// La envolvente sigue los picos: sube rápido y vuelve lentamente hacia la línea de base.
	if (muestra_x16 > ecg_envelope_x16) {
		ecg_envelope_x16 = muestra_x16;
	} else {
		if (ecg_envelope_x16 > ecg_baseline_x16) {
			diff = ecg_envelope_x16 - ecg_baseline_x16;
			ecg_envelope_x16 -= paso_minimo(diff, 9);
		} else {
			ecg_envelope_x16 = ecg_baseline_x16;
		}
	}

	if (ecg_envelope_x16 > ecg_baseline_x16) {
		rango_x16 = ecg_envelope_x16 - ecg_baseline_x16;
	} else {
		rango_x16 = 0;
	}

	// Si la amplitud útil es muy baja, no intento calcular BPM.
	// Esto evita interpretar ruido como si fueran latidos.
	if (rango_x16 < ECG_RANGO_MINIMO_X16) {
		ecg_muestras_desde_latido++;
		return;
	}

	umbral_subida_x16     = ecg_baseline_x16 + (rango_x16 >> 1);
	margen_histeresis_x16 = rango_x16 >> 3;
	umbral_bajada_x16     = (umbral_subida_x16 > margen_histeresis_x16)
	                            ? (umbral_subida_x16 - margen_histeresis_x16)
	                            : 0;

	// Período refractario: durante este tiempo no se acepta otro pico.
	if (ecg_muestras_refractario > 0)
		ecg_muestras_refractario--;

	ecg_muestras_desde_latido++;

	if (!ecg_estado_sobre_umbral) {

		if ((muestra_x16 > umbral_subida_x16) &&
		    (pendiente_x16 > ECG_PENDIENTE_MIN_X16) &&
		    (ecg_muestras_refractario == 0)) {

			ecg_estado_sobre_umbral = 1;

			if (ecg_primer_latido_pendiente) {
				// El primer pico detectado solo fija la referencia temporal.
				ecg_primer_latido_pendiente = 0;
				ecg_muestras_desde_latido = 0;
				ecg_muestras_refractario  = MUESTRAS_REFRACTARIAS;
			} else {
				uint32_t intervalo_ms = ecg_muestras_desde_latido * 4;  // 4 ms por muestra a 250 Hz

				if (intervalo_ms > 0) {
					uint32_t bpm_candidato = 60000 / intervalo_ms;

					if ((bpm_candidato >= BPM_MINIMO_PLAUSIBLE) &&
					    (bpm_candidato <= BPM_MAXIMO_PLAUSIBLE)) {

						ecg_bpm_actual = bpm_candidato;
						mostrar_bpm_en_leds(ecg_bpm_actual);

						// Reinicio el contador solo si el BPM calculado entra en un rango razonable.
						ecg_muestras_desde_latido = 0;
						ecg_muestras_refractario  = MUESTRAS_REFRACTARIAS;
					}
				}
			}
		}
	} else {
		if (muestra_x16 < umbral_bajada_x16) {
			ecg_estado_sobre_umbral = 0;
		}
	}
}

void reiniciar_deteccion_bpm(void)
{
	ecg_baseline_x16             = (2048u << 4);
	ecg_envelope_x16             = (2048u << 4);
	ecg_estado_sobre_umbral      = 0;
	ecg_muestras_refractario     = 0;
	ecg_muestras_desde_latido    = 0;
	ecg_primer_latido_pendiente  = 1;
	ecg_bpm_actual               = 0;
	ecg_muestra_anterior_x16     = (2048u << 4);
	ecg_pendiente_actual_x16     = 0;

	// Apaga los 5 LEDs de BPM (PB8..PB12)
	DEVMAP->GPIOs[GPIOB].REGs.ODR &= ~((1<<8)|(1<<9)|(1<<10)|(1<<11)|(1<<12));
}

// ============================================================
// Función principal
// ============================================================
int main(void)
{
	// ----------------------------------------------------------
	// Configuración del reloj del sistema a 72 MHz
	// ----------------------------------------------------------
	DEVMAP->RCC.REGs.CR    |= (1 << 16);					// Habilita HSE
	while (!(DEVMAP->RCC.REGs.CR & (1 << 17)));				// Espera estabilización de HSE

	DEVMAP->RCC.REGs.CR    &= ~(1 << 24);					// Deshabilita PLL para configurarlo
	DEVMAP->RCC.REGs.CFGR  |= (0b0111 << 18);				// PLLMUL = x9 -> 8MHz*9 = 72MHz
	DEVMAP->RCC.REGs.CFGR  |=  (1 << 16);					// Fuente del PLL = HSE
	DEVMAP->RCC.REGs.CR    |=  (1 << 24);					// Habilita PLL
	while (!(DEVMAP->RCC.REGs.CR & (1 << 25)));				// Espera que el PLL se bloquee

	DEVMAP->FLASH.REGs.ACR |= (0b010 << 0);					// Flash: 2 wait states para 72MHz
	DEVMAP->RCC.REGs.CFGR  |= (0b0000 << 4);				// AHB sin divisor -> 72MHz
	DEVMAP->RCC.REGs.CFGR  |= (0b100 << 8);					// APB1 /2 -> 36MHz (máx. permitido)
	DEVMAP->RCC.REGs.CFGR  |= (0b10 << 14);					// ADC clock = PCLK2/6 = 12MHz

	DEVMAP->RCC.REGs.CFGR  |= (0b10 << 0);					// Selecciona PLL como reloj de sistema
	while (!(DEVMAP->RCC.REGs.CFGR & (0b10 << 2)));			// Espera la conmutación

	// ----------------------------------------------------------
	// SysTick: genera la frecuencia de muestreo del ECG, 250 Hz
	// ----------------------------------------------------------
	DEVMAP->RCC.REGs.APB2ENR |= (1 << 4);					// Habilita clock de GPIOC

	DEVMAP->GPIOs[GPIOC].REGs.CRH &= 0xFF0FFFFF;			// Limpia nibble de PC13
	DEVMAP->GPIOs[GPIOC].REGs.CRH |= 0x00300000;			// PC13: salida push-pull 50MHz
	DEVMAP->GPIOs[GPIOC].REGs.ODR |= (1 << 13);				// LED apagado al inicio (activo en bajo)

	CTX->SYSTICK.REGs.RVR = 36000 - 1;						// 9 MHz / 250 Hz = 36000 cuentas
	CTX->SYSTICK.REGs.CSR  = 0x00000;						// Limpia registro
	CTX->SYSTICK.REGs.CSR |= (1 << 1);						// Habilita interrupción
	CTX->SYSTICK.REGs.CSR |= (1 << 0);						// Habilita SysTick
	CTX->SYSTICK.REGs.CVR  = 0;								// Limpia contador

	// ----------------------------------------------------------
	// ADC: PA1 =  salida analógica del AD8232
	// ----------------------------------------------------------
	DEVMAP->RCC.REGs.CFGR    |= (0b10 << 14);				// ADC prescaler /6 -> 12 MHz
	DEVMAP->RCC.REGs.APB2ENR |= (1 << 9);					// Habilita clock de ADC1
	DEVMAP->RCC.REGs.APB2ENR |= (1 << 2);					// Habilita clock de GPIOA

	DEVMAP->GPIOs[GPIOA].REGs.CRL &= ~(0xFu << 4);			// PA1: entrada analógica

	DEVMAP->ADC[ADC1].REGs.CR2 |= (1 << 0);					// ADON: encendido del ADC

	// Calibración del ADC
	DEVMAP->ADC[ADC1].REGs.CR2 |= (1 << 3);					// RSTCAL
	while (DEVMAP->ADC[ADC1].REGs.CR2 & (1 << 3));
	DEVMAP->ADC[ADC1].REGs.CR2 |= (1 << 2);					// CAL
	while (DEVMAP->ADC[ADC1].REGs.CR2 & (1 << 2));

	DEVMAP->ADC[ADC1].REGs.CR1	&= ~(1 << 8);				// SCAN deshabilitado (1 solo canal)
	DEVMAP->ADC[ADC1].REGs.SQR3 &= ~0xFFFFFFFF;				// Limpia registro de secuencia
	DEVMAP->ADC[ADC1].REGs.SQR3 |= (0b00001 << 0);			// Canal 1 (PA1) primero en la secuencia
	DEVMAP->ADC[ADC1].REGs.CR2	&= ~(1 << 1);				// Conversión simple (no continua)
	DEVMAP->ADC[ADC1].REGs.CR2	&= ~(1 << 11);				// Alineación a la derecha
	DEVMAP->ADC[ADC1].REGs.CR1	|= (1 << 5);				// Habilita interrupción EOC
	ENA_IRQ(IRQ_ADC1_2);									// Habilita IRQ del ADC en el NVIC

	DEVMAP->RCC.REGs.APB2ENR |= (1 << 3);					// Habilita clock de GPIOB

	DEVMAP->GPIOs[GPIOB].REGs.CRL = 0x33300088;				// PB0 y PB1 como entradas; PB5, PB6 y PB7 como salidas

	DEVMAP->GPIOs[GPIOB].REGs.CRH &= 0xFFF00000;			// Limpia los 5 nibbles de PB8..PB12
	DEVMAP->GPIOs[GPIOB].REGs.CRH |= 0x00033333;			// PB8..PB12: salida push-pull 50MHz

	DEVMAP->GPIOs[GPIOB].REGs.ODR = 0;						// PB0/PB1 pull-down; todos los LEDs apagados

	// ----------------------------------------------------------
	// USART1: 115200 bps, PA9 (TX)  => RX
	// ----------------------------------------------------------
	DEVMAP->RCC.REGs.APB2ENR |= (1 << 14);					// Habilita clock de USART1

	DEVMAP->GPIOs[GPIOA].REGs.CRH &= 0xFFFFF00F;			// Limpia PA9 
	DEVMAP->GPIOs[GPIOA].REGs.CRH |= 0x000004B0;			// PA9: AF push-pull | PA10: entrada flotante

	DEVMAP->USART1.REGs.CR1  = 0;
	DEVMAP->USART1.REGs.CR1 &= ~(1 << 12);					// 8 bits de datos
	DEVMAP->USART1.REGs.CR2 |=  (0b00 << 12);				// 1 stop bit
	DEVMAP->USART1.REGs.BRR  = 72000000 / 115200;			// = 625 (0x271)
	DEVMAP->USART1.REGs.CR1 |= (1 << 3);					// Transmisor habilitado (TE)
	DEVMAP->USART1.REGs.CR1 |= (1 << 13);					// USART habilitado (UE)
	ENA_IRQ(IRQ_USART1);									// Habilita IRQ del USART en el NVIC

	/* ******* */
	/* PROCESO */ {
	/* ******* */
		enum states { S_SYSTICK, S_ADC, S_TXDATA };
		enum states state;
		uint8_t tx_data[TX_BUFFER_LEN];
		uint8_t *tx_ptr = tx_data;

		// Ventana para evaluar amplitud: 250 muestras equivalen a 1 segundo a 250 Hz
		uint32_t sample_count = 0;
		uint32_t win_min = 0xFFFFFFFF;
		uint32_t win_max = 0;

		uint32_t electrodos_desconectados_anterior = 0;

		// Dato UART: "BPM=YYY\r\n"
		tx_data[0] = 'B';
		tx_data[1] = 'P';
		tx_data[2] = 'M';
		tx_data[3] = '=';
		tx_data[7] = '\r';
		tx_data[8] = '\n';

		tock  = tick;
		state = S_SYSTICK;

		for (;;) {
			switch (state) {

			// ------------------------------------------------
			// Estado 1: espera el próximo tick del SysTick
			// ------------------------------------------------
			case S_SYSTICK:
				if (tick != tock) {
					DEVMAP->ADC[ADC1].REGs.CR2 |= (1 << 0);	// Dispara nueva conversión (ADON ya en 1)
					tock = tick;
					adc1_2_req = !adc1_2_rdy;
					state = S_ADC;
				}
				break;
			// ------------------------------------------------
			// Estado 2: esperar fin de conversión y procesar
			// ------------------------------------------------
			case S_ADC:
				if (adc1_2_req == adc1_2_rdy) {
					uint32_t dr;
					uint32_t lo_plus, lo_minus;

					dr = DEVMAP->ADC[ADC1].REGs.DR;
					// Lectura de los pines de detección de electrodos
					lo_plus  = (DEVMAP->GPIOs[GPIOB].REGs.IDR >> 0) & 1;
					lo_minus = (DEVMAP->GPIOs[GPIOB].REGs.IDR >> 1) & 1;

					if (lo_plus || lo_minus) {
						// Electrodo desconectado: se prende el LED rojo y se reinicia el cálculo
						dr = 9999;	
						DEVMAP->GPIOs[GPIOB].REGs.ODR |=  (1 << 5);	// Rojo ON
						DEVMAP->GPIOs[GPIOB].REGs.ODR &= ~(1 << 6);	// Amarillo OFF
						DEVMAP->GPIOs[GPIOB].REGs.ODR &= ~(1 << 7);	// Verde OFF
					
						if (!electrodos_desconectados_anterior) {
							reiniciar_deteccion_bpm();
							electrodos_desconectados_anterior = 1;
						}
					} else {
						// Electrodos conectados: actualizo mínimos y máximos para evaluar la amplitud
						if (dr < win_min) win_min = dr;
						if (dr > win_max) win_max = dr;
						DEVMAP->GPIOs[GPIOB].REGs.ODR &= ~(1 << 5);	// Rojo OFF

						electrodos_desconectados_anterior = 0;

						actualizar_deteccion_bpm(dr);
					}
					sample_count++;
					if (sample_count >= 250) {

						if (!(lo_plus || lo_minus)) {
							uint32_t amplitude = win_max - win_min;
							if (amplitude < 50) {
								DEVMAP->GPIOs[GPIOB].REGs.ODR |=  (1 << 6);	// Amarillo ON
								DEVMAP->GPIOs[GPIOB].REGs.ODR &= ~(1 << 7);	// Verde OFF
							} else {
								DEVMAP->GPIOs[GPIOB].REGs.ODR &= ~(1 << 6);	// Amarillo OFF
								DEVMAP->GPIOs[GPIOB].REGs.ODR |=  (1 << 7);	// Verde ON
							}
						}
						DEVMAP->GPIOs[GPIOC].REGs.ODR ^= (1 << 13);	// Parpadeo de actividad cada 1 segundo
						sample_count = 0;
						win_min = 0xFFFFFFFF;
						win_max = 0;
					}

					tx_data[TX_OFFSET_BPM + 0] = '0' + (ecg_bpm_actual / 100) % 10;
					tx_data[TX_OFFSET_BPM + 1] = '0' + (ecg_bpm_actual / 10)  % 10;
					tx_data[TX_OFFSET_BPM + 2] = '0' + (ecg_bpm_actual)       % 10;

					tx_req = tx_rdy;
					tx_ptr = tx_data;
					state  = S_TXDATA;
				}
				break;

			// ------------------------------------------------
			// Estado 3: transmitir los datos
			// ------------------------------------------------
			case S_TXDATA:
				if (tx_ptr < tx_data + sizeof(tx_data)) {
					if (tx_req == tx_rdy) {
						tx_req = !tx_rdy;
						DEVMAP->USART1.REGs.DR = *tx_ptr++;
						DEVMAP->USART1.REGs.CR1 |= (1 << 7);	
					}
				} else {
					state = S_SYSTICK;							//Espera una nueva muestra
				}
				break;
			}
		}
	}

	return 0;
}