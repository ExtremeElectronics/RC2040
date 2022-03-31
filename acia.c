#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "system.h"
#include "acia.h"

//ACIA
//#include "pico/stdlib.h"
//#include "hardware/gpio.h"

//#include "hardware/uart.h"
//#include "hardware/irq.h"
//#define UART_ID uart0

struct acia {
    uint8_t status;
    uint8_t config;
    uint8_t rxchar;
    uint8_t input;
    uint8_t inint;
    uint8_t inreset;
    uint8_t trace;
};


static void acia_irq_compute(struct acia *acia)
{
	/* Recalculate the interrupt bit */
	acia->status &= 0x7F;
	if ((acia->status & 0x01) && (acia->config & 0x80))
		acia->status |= 0x80;
	if ((acia->status & 0x02) && (acia->config & 0x60) == 0x20)
		acia->status |= 0x80;
	/* Now see what should happen */
	if (!(acia->config & 0x80) || !(acia->status & 0x80)) {
		if (acia->inint && acia->trace)
			printf( "ACIA interrupt end.\n");
		acia->inint = 0;
		acia->status &= 0x7F;
		return;
	}
	if (acia->inint == 0 && (acia->trace))
		printf( "ACIA interrupt.\n");
	acia->inint = 1;
	recalc_interrupts();
}

//DJW ininit test
uint8_t acia_in_interrupt(struct acia *acia)
{
  return acia->inint;
}

static void acia_receive(struct acia *acia)
{
	if (acia->inreset)
		return;
	/* Already a character waiting so set OVRN */
	if (acia->status & 1)
		acia->status |= 0x20;
		
	acia->rxchar = next_char();
	if (acia->trace)
		printf( "ACIA rx.\n");
	acia->status |= 0x01;	/* IRQ, and rx data full */
}

static void acia_transmit(struct acia *acia)
{
	if (!(acia->status & 2)) {
		if (acia->trace)
			printf( "ACIA tx is clear.\n");
		acia->status |= 0x02;	/* IRQ, and tx data empty */
	}
}

void acia_timer(struct acia *acia)
{
	int s = check_chario();
	if ((s & 1) && acia->input)
		acia_receive(acia);
	if (s & 2)
		acia_transmit(acia);
	if (s)
		acia_irq_compute(acia);
}

uint8_t acia_read(struct acia *acia, uint16_t addr)
{
	if (acia->trace)
		printf( "acia_read %d ", addr);
	switch (addr) {
	case 0:
		if (acia->inreset) {
			if (acia->trace)
				printf( "= 0 (reset).\n");
			return 0;
		}
		/* Reading the ACIA status has no effect on the bits */
		if (acia->trace)
			printf( "acia->status %d\n", acia->status);
		return acia->status;
	case 1:
		/* Reading the ACIA character clears the receive ready
		   and also updates the error bits to match the new byte */
		/* Clear receive ready and rx overrun */
		acia->status &= ~0x21;
		acia_irq_compute(acia);
		if (acia->trace)
			printf( "acia_char %d\n", acia->rxchar);
		return acia->rxchar;
	default:
		printf( "acia: bad addr.\n");
		exit(1);
	}
}

void acia_write(struct acia *acia, uint16_t addr, uint8_t val)
{
	if (acia->trace)
		printf( "acia_write %d %d\n", addr, val);
	switch (addr) {
	case 0:
		/* bit 7 enables interrupts, bits 5-6 are tx control
		   bits 2-4 select the word size and 0-1 counter divider
		   except 11 in them means reset */
		acia->config = val;
		if ((acia->config & 3) == 3)
			acia->inreset = 1;
		else if (acia->inreset) {
			acia->inreset = 0;
			acia->status = 2;
		}
		if (acia->trace)
			printf( "ACIA config %02X\n", val);
		acia_irq_compute(acia);
		return;
	case 1:
		//write(1, &val, 1);
//		uart_write_blocking(UART_ID,&val,1);
	        out_char(&val);
		/* Clear TDRE - we now have a byte */
		acia->status &= ~0x02;
		acia_irq_compute(acia);
		break;
	}
}

void acia_set_input(struct acia *acia, int onoff)
{
	acia->input = onoff;
}

void acia_reset(struct acia *acia)
{
    memset(acia, 0, sizeof(struct acia));
    acia->status = 2;
    acia_irq_compute(acia);
}

uint8_t acia_irq_pending(struct acia *acia)
{
	return acia->inint;
}

struct acia *acia_create(void)
{
    struct acia *acia = malloc(sizeof(struct acia));
    if (acia == NULL) {
        printf( "Out of memory.\n");
        exit(1);
    }
    acia_reset(acia);
    return acia;
}

void acia_free(struct acia *acia)
{
    free(acia);
}

void acia_trace(struct acia *acia, int onoff)
{
    acia->trace = onoff;
}
