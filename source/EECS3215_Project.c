/*
 * Copyright 2016-2020 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
/**
 * @file    EECS3215_Project.c
 * @brief   Application entry point.
 */
#include "LPC802.h"
#include "segment.h"
#include "fsl_usart.h"
#include <stdlib.h>
#include <stdio.h>
#include "PIDController.h"

#define LED_A 12
#define LED_B 10
#define LED_C 8
#define LED_D 13
#define LED_E 0
#define LED_F 9
#define LED_G 4

#define SEGMENT_1 17
#define SEGMENT_2 16
#define SEGMENT_3 14
#define SEGMENT_4 7

#define BEAM_BREAK 1

#define CTIMER_FREQ (240000)

#define EXAMPLE_USART_CLK_SRC kCLOCK_MainClk
#define EXAMPLE_USART_CLK_FREQ CLOCK_GetFreq(EXAMPLE_USART_CLK_SRC)

const int ALL_DIGITS[10] = {DIGIT_0,
							DIGIT_1,
							DIGIT_2,
							DIGIT_3,
							DIGIT_4,
							DIGIT_5,
							DIGIT_6,
							DIGIT_7,
							DIGIT_8,
							DIGIT_9};

const int ALL_SEGMENTS[4] = {(1UL<<SEGMENT_4),
							 (1UL<<SEGMENT_3),
							 (1UL<<SEGMENT_2),
							 (1UL<<SEGMENT_1)};

int digits[4] = {0};
int current_pos = 0;
int counter = 0;
int prev_counter = 0;
int match_count = CTIMER_FREQ/2;
int rate = 0;
float duty_cycle = 0.5;
uint32_t volatile adc_result = 0;

char buffer[10];
uint32_t current_index = 0;
uint8_t temp = 0;

void SysTick_Cfg();
void setup_PWM(void);
void setup_PIN_IRQ(void);
void setup_ADC(void);
void setup_UART(void);
void set_display(int number);
void set_output(double percent);
void send_dataUART(int data);
double get_feedforward(double setpoint);
int get_setpoint(void);

PIDController pid;


int main(void) {
//	BOARD_InitBootPins();
//	BOARD_BootClockFRO24M();

	pid = pid_create(0.001, 0.0, 0.0);

	SYSCON->SYSAHBCLKCTRL0 |= (SYSCON_SYSAHBCLKCTRL0_GPIO0_MASK | SYSCON_SYSAHBCLKCTRL0_GPIO_INT_MASK);

	SYSCON->PRESETCTRL0 &= ~(SYSCON_PRESETCTRL0_GPIO0_RST_N_MASK | SYSCON_PRESETCTRL0_GPIOINT_RST_N_MASK);// reset GPIO and GPIO Interrupt
	SYSCON->PRESETCTRL0 |= (SYSCON_PRESETCTRL0_GPIO0_RST_N_MASK | SYSCON_PRESETCTRL0_GPIOINT_RST_N_MASK);// clear reset (bit=1)

	GPIO->CLR[0] = ALL_LEDS;

	GPIO->DIRSET[0] = ALL_LEDS;
	GPIO->DIRCLR[0] = (1UL<<BEAM_BREAK);

	GPIO->SET[0] = (1UL<<SEGMENT_1) |
				   (1UL<<SEGMENT_2) |
				   (1UL<<SEGMENT_3) |
				   (1UL<<SEGMENT_4);

	GPIO->DIRSET[0] = (1UL<<SEGMENT_1) |
				   	  (1UL<<SEGMENT_2) |
					  (1UL<<SEGMENT_3) |
					  (1UL<<SEGMENT_4);

	int n = 0000;
	set_display(n);

	__disable_irq();
	SysTick_Cfg();
	setup_PWM();
	setup_PIN_IRQ();
	__enable_irq();

	setup_ADC();

	//setup_UART();

    /* Enter an infinite loop, just incrementing a counter. */
    while(1) {
    	int setpoint = get_setpoint();
    	pid_calc(&pid, setpoint, rate);
    	set_output(get_feedforward(setpoint) + pid.output);

//    	send_dataUART(rate);
    }
    return 0 ;
}

double get_feedforward(double setpoint){
	return 0.0012*setpoint;
}

int get_setpoint(){
	ADC0->SEQ_CTRL[0] &= ~(1UL<<ADC_SEQ_CTRL_START_SHIFT); // start bit to 0
	ADC0->SEQ_CTRL[0] |= (1UL<<ADC_SEQ_CTRL_START_SHIFT); // start bit to 1
	// Read the captured value on ADC ch 8. Assign it to a variable.
	adc_result = ((ADC0->DAT[8])&(ADC_DAT_RESULT_MASK));// isolate bits 15:4 (data)
	adc_result = (adc_result>>ADC_DAT_RESULT_SHIFT);// shift right;get true numeric value.

	// Remove deadband at the extremes of the potentiometer
	if(adc_result < 50){
		adc_result = 0;
	}
	else if(adc_result > 4000){
		adc_result = 4000;
	}

	return adc_result/10; // Speed will be from 0 RPM to 400 RPM
}

void send_dataUART(int data){
	if(((USART0->STAT) & USART_STAT_TXRDY_MASK)) {
		temp = buffer[current_index];
		if(temp != '\0'){
			USART0->TXDAT = temp;
			current_index++;
		} else {
			sprintf(buffer, "%d\n", data);
			current_index = 0;
		}
	}
}

// Takes an input from -1.0 to 1.0
void set_output(double percent){
	if(percent > 1.0){
		percent = 1.0;
	}
	else if(percent < -1.0){
		percent = -1.0;
	}

	duty_cycle = percent*0.032 + 0.075;
	CTIMER0->MR[0] = CTIMER_FREQ*(1-duty_cycle);
}

void set_display(int number) {
	int i = 0;

	for(i = 0; i < 4; i++){
		if(number > 0){
			digits[i] = number % 10;

			number /= 10;
		} else {
			digits[i] = 0;
		}
	}
}

void setup_UART(){
	CLOCK_Select(kUART0_Clk_From_Fro);

	usart_config_t config;
	/* Default config by using USART_GetDefaultConfig():
	 * config.baudRate_Bps = 9600U;
	 * config.parityMode = kUSART_ParityDisabled;
	 * config.stopBitCount = kUSART_OneStopBit;
	 * config.bitCountPerChar = kUSART_8BitsPerChar;
	 * config.loopback = false;
	 * config.enableRx = false;
	 * config.enableTx = false;
	 * config.syncMode = kUSART_SyncModeDisabled;
	 */
	USART_GetDefaultConfig(&config);
	config.enableRx     = false;
	config.enableTx     = true;
	config.baudRate_Bps = 9600;

	/* Initialize the USART with configuration. */
	USART_Init(USART0, &config, EXAMPLE_USART_CLK_FREQ);
}

void setup_PIN_IRQ(){
	NVIC_DisableIRQ(PIN_INT0_IRQn);

	// Set up GPIO IRQ: interrupt channel 0 (PINTSEL0) to GPIO 8
	SYSCON->PINTSEL[0] = 0x1; // PINTSEL0 is P0_8
	// Configure the Pin interrupt mode register (a.k.a ISEL) for edge-sensitive
	// on PINTSEL0. 0 is edge sensitive. 1 is level sensitive.
	PINT->ISEL = 0x00;// channel 0 bit is 0: is edge sensitive (so are the other channels)
	// Use IENR or IENF (or S/CIENF or S/CIENR) to set edge type
	// Configure Chan 0 for only falling edge detection (no rising edge detection)
	PINT->CIENR = 0b00000001; // bit 0 is 1: disable channel 0 IRQ for rising edge
	PINT->SIENF = 0b00000001; // bit 0 is 1: enable channel 0 IRQ for falling edge
	// Remove any pending or left-over interrupt flags
	PINT->IST = 0xFF;

	NVIC_EnableIRQ(PIN_INT0_IRQn);
}

// The Potentiometer on the LPC board is connected to PIO0_15 (ADC_8)
void setup_ADC(void)
{
	// Power Config, Bit 4: 0 is powered on, 1 is powered down.
	SYSCON->PDRUNCFG &= ~(SYSCON_PDRUNCFG_ADC_PD_MASK);

	// Here, turn on ADC and SWM
	SYSCON->SYSAHBCLKCTRL0 |= ( SYSCON_SYSAHBCLKCTRL0_ADC_MASK | SYSCON_SYSAHBCLKCTRL0_SWM_MASK);// Reset the ADC module.
	// Table 64 in User Manual.
	// Bit 24 in Peripheral Reset Control Register 0: go 0, then 1 to reset and clear.
	SYSCON->PRESETCTRL0 &= ~(SYSCON_PRESETCTRL0_ADC_RST_N_MASK);// Assert reset (0)
	SYSCON->PRESETCTRL0 |= (SYSCON_PRESETCTRL0_ADC_RST_N_MASK);// Remove reset(1)

	SYSCON->ADCCLKSEL &= ~(SYSCON_ADCCLKSEL_SEL_MASK); // Use fro_clk as source for ADC async clock
	// Divide the FRO clock into the ADC. If 0 it shuts down the ADC clock?
	SYSCON->ADCCLKDIV = 1;// divide by 1 (values: 0 to 255)

	// Make bit 18 a 0 (active low) to turn on this ADC channel
	SWM0->PINENABLE0 &= ~(SWM_PINENABLE0_ADC_8_MASK); //

	ADC0->SEQ_CTRL[0] |= (1UL<<8);// turn on ADC channel 8.

	// Step 2. Set TRIGPOL to 1 and SEQ_ENA to 1 in SEQA_CTRL register
	ADC0->SEQ_CTRL[0] |= (1UL<<ADC_SEQ_CTRL_TRIGPOL_SHIFT); // trig pol set to 1.
	ADC0->SEQ_CTRL[0] |= (1UL<<ADC_SEQ_CTRL_SEQ_ENA_SHIFT); // Sequence A turned ON. // Step 3. set START bit to 1 in SEQA_CTRL register
	ADC0->SEQ_CTRL[0] |= (1UL<<ADC_SEQ_CTRL_START_SHIFT); // start bit to 1
}

void setup_PWM() {
	// enable switch matrix
	SYSCON->SYSAHBCLKCTRL0 |= (SYSCON_SYSAHBCLKCTRL0_SWM_MASK);

	SWM0->PINASSIGN.PINASSIGN4 &= ~(0xff); // clear the bottom 8 bits
	SWM0->PINASSIGN.PINASSIGN4 |= (0xB);// put 0xB is the bottom 8

	// Enable CTIMER clock
	SYSCON->SYSAHBCLKCTRL0 |= (SYSCON_SYSAHBCLKCTRL0_CTIMER0_MASK);
	SYSCON->PRESETCTRL0 &= ~(SYSCON_PRESETCTRL0_CTIMER0_RST_N_MASK); // Reset
	SYSCON->PRESETCTRL0|=(SYSCON_PRESETCTRL0_CTIMER0_RST_N_MASK);

	CTIMER0->EMR |= CTIMER_EMR_EM0_MASK; // (rule complicated)
	CTIMER0->PWMC |= CTIMER_PWMC_PWMEN0_MASK; // 0 is match mode; 1 is PWM mode.
	// Clear all the Channel 0 bits in the MCR
	CTIMER0->MCR &= ~(CTIMER_MCR_MR0R_MASK | CTIMER_MCR_MR0S_MASK | CTIMER_MCR_MR0I_MASK);

	CTIMER0->MCR |= CTIMER_MCR_MR3R_MASK;
	CTIMER0->MR[3] = CTIMER_FREQ;
	CTIMER0->MR[0] = match_count; // DUTY FACTOR is 0.75

	CTIMER0->TCR |= CTIMER_TCR_CEN_MASK;
}

void SysTick_Cfg() {
	NVIC_DisableIRQ(SysTick_IRQn);

	SYSCON->MAINCLKSEL = (0x0<<SYSCON_MAINCLKSEL_SEL_SHIFT);

	SYSCON->MAINCLKUEN &= ~(0x1);
	SYSCON->MAINCLKUEN |= 0x1;

	BOARD_BootClockFRO24M();

	SysTick_Config(12000); // 1000Hz

	NVIC_EnableIRQ(SysTick_IRQn);
}

void SysTick_Handler(void){
	GPIO->CLR[0] = (1UL<<SEGMENT_1) |
				   (1UL<<SEGMENT_2) |
				   (1UL<<SEGMENT_3) |
				   (1UL<<SEGMENT_4);

	GPIO->SET[0] = ALL_SEGMENTS[current_pos];
	GPIO->SET[0] = ALL_LEDS;
	GPIO->CLR[0] = ALL_DIGITS[digits[current_pos]];

	current_pos += 1;
	if(current_pos > 3){
		current_pos = 0;
	}

	counter++;
}

void PIN_INT0_IRQHandler(void) {
	if (PINT->IST & (1<<0)){
		PINT->IST = (1<<0);

		int difference = abs(counter - prev_counter);
		rate = (1/(difference/1000.0))*60.0/4; // Rate per minute
		set_display(rate);
		prev_counter = counter;

//		GPIO->NOT[0] = (1UL<<LED_G);
	}
	else {

	}
}
