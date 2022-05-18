/*
 *	Platform features
 *
 *	Z80 at 7.372MHz
 *	Zilog SIO/2 at 0x80-0x83
 *	Motorola 6850 repeats all over 0x40-0x7F (not recommended)
 *	IDE at 0x10-0x17 no high or control access
 *	16550A at 0xA0
 *
 *	Known bugs
 *	Not convinced we have all the INT clear cases right for SIO error
 *
 *	The SC121 just an initial sketch for playing with IM2 and the
 *	relevant peripherals. I'll align it properly with the real thing as more
 *	info appears.
 *
 */

#include <stdio.h>

//pico headers
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/multicore.h"

//iniparcer
#include "dictionary.h"
#include "iniparser.h"


//sd card reader
#include "f_util.h"
#include "ff.h"
#include "pico/stdlib.h"
#include "rtc.h"
//
#include "hw_config.h"

#include <unistd.h>


#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <tusb.h>
#include "system.h"
#include "libz80/z80.h"

#include "acia.h"
#include "ide.h"
//#include "ppide.h"
#include "z80dma.h"
#include "z80dis.h"

//2 pages of RAM/ROM for PICO
#define USERAM 1
#define USEROM 0

//spo256al2
//pico SDK includes
#include "hardware/pwm.h"

//program includes
#include "allophones.c"
#include "allophoneDefs.h"

//must be pins on the same slice
#define soundIO1 15
#define soundIO2 14

#define PWMrate 90

uint PWMslice;
uint8_t SPO256Port;
uint8_t SPO256DataOut;
uint8_t SPO256DataReady=0;

//Beep

#include "midiNotes.h"
uint8_t BeepPort;
uint8_t BeepDataOut;
uint8_t BeepDataReady=0;





//RAMROM
static uint8_t ram[0x10000]; //64k
static uint8_t rom[0x10000]; //64k

//rom setup
static uint8_t romdisable=0; //1 disables rom, making ram 0-64k
static uint16_t pagesize=0x8000; //32k rom
int rombank = 4; //default to CPM on default rom image

//max files for ls on SD card.
#define MaxBinFiles 100
char BinFiles[MaxBinFiles];

/* use stdio for errors via usb uart */
/* use 0=UART or 1=USB for serial comms */
/* 3=both for init ONLY */
int UseUsb=3; 

//IDE
static int ide =1; //set to 1 to init IDE
struct ide_controller *ide0;

/* Real UART setup*/
#define UART_ID uart0
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1

char usbcharbuf=0;
int hasusbcharwaiting=0;

int have_acia = 0;


struct acia *acia;
static uint8_t acia_narrow;


//serial in circular buffer
#define INBUFFERSIZE 10000
static char charbufferUART[INBUFFERSIZE];
static int charinUART=0;
static int charoutUART=0;

//usb serial buffer
static char charbufferUSB[INBUFFERSIZE];
static int charinUSB=0;
static int charoutUSB=0;

#define ENDSTDIN 0xFF //non char rx value


//PIO
int PIOA=0;

uint8_t PIOAp[]={16,17,18,19,20,21,26,27};

//PICO GPIO
// use regular LED
const uint LEDPIN = PICO_DEFAULT_LED_PIN;

/*
//ROM Address Switches before PCB layout
const uint ROMA13 = 18;
const uint ROMA14 = 19;
const uint ROMA15 = 20;

//
const uint SELSEL = 21;
*/

const uint HASSwitchesIO =22;
//
int HasSwitches=0;
//

//ROM Address Switches
const uint ROMA13 = 10;
const uint ROMA14 = 11;
const uint ROMA15 = 12;

const uint SELSEL = 13;


//buttons
const uint DUMPBUT =9;
const uint RESETBUT =7;


/* use stdio for errors via usb uart */

/* Real UART setup*/
#define UART_ID uart0
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_TX_PIN 0
#define UART_RX_PIN 1

static unsigned int bankreg[4];
static uint8_t bankenable;

static uint8_t switchrom = 1;
//static uint32_t romsize = 65536;

#define CPUBOARD_Z80		0
#define CPUBOARD_SC108		1
#define CPUBOARD_SC114		2
#define CPUBOARD_Z80SBC64	3
#define CPUBOARD_EASYZ80	4
#define CPUBOARD_SC121		5
#define CPUBOARD_MICRO80	6
#define CPUBOARD_ZRCC		7
#define CPUBOARD_TINYZ80	8
#define CPUBOARD_PDOG128	9
#define CPUBOARD_PDOG512	10

static uint8_t cpuboard = CPUBOARD_Z80;

static uint8_t have_ctc;
static uint8_t have_16x50;
static uint8_t fast = 0;
static uint8_t int_recalc = 0;

static uint16_t tstate_steps = 365;	/* RC2014 speed */

/* IRQ source that is live in IM2 */
static uint8_t live_irq;

#define IRQ_SIOA	1
#define IRQ_SIOB	2
#define IRQ_CTC		3	/* 3 4 5 6 */

static Z80Context cpu_z80;

volatile int emulator_done;

#define TRACE_MEM	0x000001
#define TRACE_IO	0x000002
#define TRACE_ROM	0x000004
#define TRACE_UNK	0x000008
#define TRACE_CPU	0x000010
#define TRACE_512	0x000020
#define TRACE_RTC	0x000040
#define TRACE_SIO	0x000080
#define TRACE_CTC	0x000100
#define TRACE_CPLD	0x000200
#define TRACE_IRQ	0x000400
#define TRACE_UART	0x000800
#define TRACE_Z84C15	0x001000
#define TRACE_IDE	0x002000
#define TRACE_SPI	0x004000
#define TRACE_SD	0x008000
#define TRACE_PPIDE	0x010000
#define TRACE_COPRO	0x020000
#define TRACE_COPRO_IO	0x040000
#define TRACE_TMS9918A  0x080000
#define TRACE_FDC	0x100000
#define TRACE_PS2	0x200000
#define TRACE_ACIA	0x400000

static int trace = 00000;

static void reti_event(void);

static uint8_t mem_read0(uint16_t addr)
{

      if(romdisable){
          if (trace & TRACE_MEM) printf( "R%04X[%02X]\n", addr, ram[addr]);
          return ram[addr];
      }else{
          if(addr<pagesize){
            if (trace & TRACE_MEM) printf( "RE%04X[%02X]\n", addr, rom[addr]);
            return rom[addr];
          }else{
              if (trace & TRACE_MEM)  printf( "R%04X[%02X]\n", addr, ram[addr]);
            return ram[addr];
          }
      }


	
}

static void mem_write0(uint16_t addr, uint8_t val)
{

      if(romdisable){
          //is ALL RAM
          if (trace & TRACE_MEM) printf( "W%04x[%02X]\n", (unsigned int) addr, (unsigned int) val);
          ram[addr]=val;
      }else{
          if(addr<pagesize){
            //Lower Mem is ROM - Write DFA
            if (trace & TRACE_MEM) printf( "ROM FAIL WRITE %04x[%02X]\n", (unsigned int) addr, (unsigned int) val);
          }else{
            if (trace & TRACE_MEM) printf( "W%04x[%02X]", (unsigned int) addr, (unsigned int) val);
            ram[addr]=val;
          }
      }



}


uint8_t do_mem_read(uint16_t addr, int quiet)
{
	return mem_read0(addr);
}

uint8_t mem_read(int unused, uint16_t addr)
{
	static uint8_t rstate = 0;
	uint8_t r = do_mem_read(addr, 0);

	if (cpu_z80.M1) {
		/* DD FD CB see the Z80 interrupt manual */
		if (r == 0xDD || r == 0xFD || r == 0xCB) {
			rstate = 2;
			return r;
		}
		/* Look for ED with M1, followed directly by 4D and if so trigger
		   the interrupt chain */
		if (r == 0xED && rstate == 0) {
			rstate = 1;
			return r;
		}
	}
	if (r == 0x4D && rstate == 1)
		reti_event();
	rstate = 0;
	return r;
}

void mem_write(int unused, uint16_t addr, uint8_t val)
{
	 mem_write0(addr, val);
}


static unsigned int nbytes;



uint8_t z80dis_byte(uint16_t addr)
{
	uint8_t r = do_mem_read(addr, 1);
	printf( "%02X ", r);
	nbytes++;
	return r;
}

uint8_t z80dis_byte_quiet(uint16_t addr)
{
	return do_mem_read(addr, 1);
}

static void z80_trace(unsigned unused)
{
	static uint32_t lastpc = -1;
	char buf[256];

	if ((trace & TRACE_CPU) == 0)
		return;
	nbytes = 0;
	/* Spot XXXR repeating instructions and squash the trace */
	if (cpu_z80.M1PC == lastpc && z80dis_byte_quiet(lastpc) == 0xED &&
		(z80dis_byte_quiet(lastpc + 1) & 0xF4) == 0xB0) {
		return;
	}
	lastpc = cpu_z80.M1PC;
	printf( "%04X: ", lastpc);
	z80_disasm(buf, lastpc);
	while(nbytes++ < 6)
		printf( "   ");
	printf( "%-16s ", buf);
	printf( "[ %02X:%02X %04X %04X %04X %04X %04X %04X ]\n",
		cpu_z80.R1.br.A, cpu_z80.R1.br.F,
		cpu_z80.R1.wr.BC, cpu_z80.R1.wr.DE, cpu_z80.R1.wr.HL,
		cpu_z80.R1.wr.IX, cpu_z80.R1.wr.IY, cpu_z80.R1.wr.SP);
}



// experimental usb char in circular buffer
int intUSBcharwaiting(){
// no interrupt or waiting check so use unblocking getchar, adds to buff if available
    char c = getchar_timeout_us(0); 
    if(c!=ENDSTDIN){
        charbufferUSB[charinUSB]=c;
        charinUSB++;
        if (charinUSB==INBUFFERSIZE){
            charinUSB=0;
        }
    }
    return charinUSB!=charoutUSB;
}

int testUSBcharwaiting(){
   return charinUSB!=charoutUSB;
}

char getUSBcharwaiting(void){
    char c=0;
    if(charinUSB!=charoutUSB){
        c=charbufferUSB[charoutUSB];
        charoutUSB++;
        if (charoutUSB==INBUFFERSIZE){
            charoutUSB=0;
        }
    }else{
        printf("USB Buffer underrun");
    }
  return c;
}

// experimental UART char in circular buffer rx via USB interrupt
void intUARTcharwaiting(void){
//   char c = getchar_timeout_us(0); 
    while (uart_is_readable(UART_ID)) {
        char c =uart_getc(UART_ID);
        charbufferUART[charinUART]=c;
        charinUART++;
        if (charinUART==INBUFFERSIZE){
            charinUART=0;
        }
    }
}

//char waiting test is inbuff=outbuffer?
int testUARTcharwaiting(){
      return charinUART!=charoutUART;
}

/*
int testUARTcharwaiting(){
    char c = getchar_timeout_us(0); 
    if(c!=ENDSTDIN){
        //printf("%02X ",c);
        charbufferUART[charinUART]=c;
        charinUART++;
        if (charinUART==INBUFFERSIZE){
            charinUART=0;
        }
    }
  
    return charinUART!=charoutUART;

}
*/
char getUARTcharwaiting(void){
    char c=0;
    if(charinUART!=charoutUART){
        c=charbufferUART[charoutUART];
        charoutUART++;
        if (charoutUART==INBUFFERSIZE){
            charoutUART=0;
        }

    }else{
        printf("UART Buffer underrun");
    }
  return c;

}


unsigned int check_chario(void)
{
   unsigned int r = 0;

   if (UseUsb==0){	
	if(testUARTcharwaiting()){
	//bodge.. if currently in ACIA interrupt , lie that there is nothng waiting
	//this prevents overruns
	    if(have_acia==1){
	        if(!acia_in_interrupt(acia)){
                    r|=1;//receive ready
                }
            }else{
                r|=1;
            }     
        }
        if (uart_is_writable(UART_ID )>0)
	    r |= 2;//transmit ready
   }else{
        if(testUSBcharwaiting()){
         //bodge.. if currently in interrupt , lie that there is nothng waiting
         //this prevents overruns
            if(have_acia==1){
                if(!acia_in_interrupt(acia)){
                    r|=1;//receive ready
                }
            }else{
                 r|=1;
            }    
       }
       r |=2; //always ready to tx
   }
   return r;
}

unsigned int next_char(void)
{
    char c;
    if (UseUsb==0){

	//READ UART WITH waiting char
        //uart_read_blocking(UART_ID,&c,1);
        c=getUARTcharwaiting();
    }else{
        //READ Serial USB WITH waiting char
        c=getUSBcharwaiting();
    }

    if (c == 0x0A)
        c = '\r';
//    putchar(c);
    return c;
}


void out_char(char * val){

    if (UseUsb==0){
        //out via UART
        uart_write_blocking(UART_ID,val,1);
    }else{
       //out via USB serial
        putchar(*val);
    }  
}


void recalc_interrupts(void)
{
	int_recalc = 1;
}


static void acia_check_irq(struct acia *acia)
{
	if (acia_irq_pending(acia))
		Z80INT(&cpu_z80, 0xFF);	/* FIXME probably last data or bus noise */
}


/* UART: very mimimal for the moment */
struct uart16x50 {
    uint8_t ier;
    uint8_t iir;
    uint8_t fcr;
    uint8_t lcr;
    uint8_t mcr;
    uint8_t lsr;
    uint8_t msr;
    uint8_t scratch;
    uint8_t ls;
    uint8_t ms;
    uint8_t dlab;
    uint8_t irq;
#define RXDA	1
#define TEMT	2
#define MODEM	8
    uint8_t irqline;
    uint8_t input;
};

static struct uart16x50 uart[1];

static void uarta_init(struct uart16x50 *uptr, int in)
{
    uptr->dlab = 0;
    uptr->input = in;
}

static void uart_check_irq(struct uart16x50 *uptr)
{
    if (uptr->irqline)
	    Z80INT(&cpu_z80, 0xFF);	/* actually undefined */
}

/* Compute the interrupt indicator register from what is pending */
static void uart_recalc_iir(struct uart16x50 *uptr)
{
    if (uptr->irq & RXDA)
        uptr->iir = 0x04;
    else if (uptr->irq & TEMT)
        uptr->iir = 0x02;
    else if (uptr->irq & MODEM)
        uptr->iir = 0x00;
    else {
        uptr->iir = 0x01;	/* No interrupt */
        uptr->irqline = 0;
        return;
    }
    /* Ok so we have an event, do we need to waggle the line */
    if (uptr->irqline)
        return;
    uptr->irqline = uptr->irq;

}

/* Raise an interrupt source. Only has an effect if enabled in the ier */
static void uart_interrupt(struct uart16x50 *uptr, uint8_t n)
{
    if (uptr->irq & n)
        return;
    if (!(uptr->ier & n))
        return;
    uptr->irq |= n;
    uart_recalc_iir(uptr);
}

static void uart_clear_interrupt(struct uart16x50 *uptr, uint8_t n)
{
    if (!(uptr->irq & n))
        return;
    uptr->irq &= ~n;
    uart_recalc_iir(uptr);
}

static void uart_event(struct uart16x50 *uptr)
{
    uint8_t r = check_chario();
    uint8_t old = uptr->lsr;
    uint8_t dhigh;
    if (uptr->input && (r & 1))
        uptr->lsr |= 0x01;	/* RX not empty */
    if (r & 2)
        uptr->lsr |= 0x60;	/* TX empty */
    dhigh = (old ^ uptr->lsr);
    dhigh &= uptr->lsr;		/* Changed high bits */
    if (dhigh & 1)
        uart_interrupt(uptr, RXDA);
    if (dhigh & 0x2)
        uart_interrupt(uptr, TEMT);
}

static void show_settings(struct uart16x50 *uptr)
{
    uint32_t baud;

    if (!(trace & TRACE_UART))
        return;

    baud = uptr->ls + (uptr->ms << 8);
    if (baud == 0)
        baud = 1843200;
    baud = 1843200 / baud;
    baud /= 16;
    printf( "[%d:%d",
            baud, (uptr->lcr &3) + 5);
    switch(uptr->lcr & 0x38) {
        case 0x00:
        case 0x10:
        case 0x20:
        case 0x30:
            printf( "N");
            break;
        case 0x08:
            printf( "O");
            break;
        case 0x18:
            printf( "E");
            break;
        case 0x28:
            printf( "M");
            break;
        case 0x38:
            printf( "S");
            break;
    }
    printf( "%d ",
            (uptr->lcr & 4) ? 2 : 1);

    if (uptr->lcr & 0x40)
        printf( "break ");
    if (uptr->lcr & 0x80)
        printf( "dlab ");
    if (uptr->mcr & 1)
        printf( "DTR ");
    if (uptr->mcr & 2)
        printf( "RTS ");
    if (uptr->mcr & 4)
        printf( "OUT1 ");
    if (uptr->mcr & 8)
        printf( "OUT2 ");
    if (uptr->mcr & 16)
        printf( "LOOP ");
    printf( "ier %02x]\n", uptr->ier);
}

static void uart_write(struct uart16x50 *uptr, uint8_t addr, uint8_t val)
{
    switch(addr) {
    case 0:	/* If dlab = 0, then write else LS*/
        if (uptr->dlab == 0) {
            if (uptr == &uart[0]) {
                //uart_putc(UART_ID,val);
                out_char(&val);
                //putchar(val);
                //fflush(stdout);
            }
            uart_clear_interrupt(uptr, TEMT);
            uart_interrupt(uptr, TEMT);
        } else {
            uptr->ls = val;
            show_settings(uptr);
        }
        break;
    case 1:	/* If dlab = 0, then IER */
        if (uptr->dlab) {
            uptr->ms= val;
            show_settings(uptr);
        }
        else
            uptr->ier = val;
        break;
    case 2:	/* FCR */
        uptr->fcr = val & 0x9F;
        break;
    case 3:	/* LCR */
        uptr->lcr = val;
        uptr->dlab = (uptr->lcr & 0x80);
        show_settings(uptr);
        break;
    case 4:	/* MCR */
        uptr->mcr = val & 0x3F;
        break;
    case 5:	/* LSR (r/o) */
        break;
    case 6:	/* MSR (r/o) */
        break;
    case 7:	/* Scratch */
        uptr->scratch = val;
        break;
    }
}

static uint8_t uart_read(struct uart16x50 *uptr, uint8_t addr)
{
    uint8_t r;

    switch(addr) {
    case 0:
        /* receive buffer */
        if (uptr == &uart[0] && uptr->dlab == 0) {
            uart_clear_interrupt(uptr, RXDA);
            if (check_chario() & 1)
                return next_char();
            return 0x00;
        } else
            return uptr->ls;
        break;
    case 1:
        /* IER */
        if (uptr->dlab == 0)
            return uptr->ier;
        return uptr->ms;
    case 2:
        /* IIR */
        return uptr->iir;
    case 3:
        /* LCR */
        return uptr->lcr;
    case 4:
        /* mcr */
        return uptr->mcr;
    case 5:
        /* lsr */
        r = check_chario();
        uptr->lsr &=0x90;
        if (r & 1)
             uptr->lsr |= 0x01;	/* Data ready */
        if (r & 2)
             uptr->lsr |= 0x60;	/* TX empty | holding empty */
        /* Reading the LSR causes these bits to clear */
        r = uptr->lsr;
        uptr->lsr &= 0xF0;
        return r;
    case 6:
        /* msr */
        uptr->msr &= 0x7F;
        r = uptr->msr;
        /* Reading clears the delta bits */
        uptr->msr &= 0xF0;
        uart_clear_interrupt(uptr, MODEM);
        return r;
    case 7:
        return uptr->scratch;
    }
    return 0xFF;
}


struct z80_sio_chan {
	uint8_t wr[8];
	uint8_t rr[3];
	uint8_t data[3];
	uint8_t dptr;
	uint8_t irq;
	uint8_t rxint;
	uint8_t txint;
	uint8_t intbits;
#define INT_TX	1
#define INT_RX	2
#define INT_ERR	4
	uint8_t pending;	/* Interrupt bits pending as an IRQ cause */
	uint8_t vector;		/* Vector pending to deliver */
};

static int sio2;
static int sio2_input;
static struct z80_sio_chan sio[2];



/*
 *	Interrupts. We don't handle IM2 yet.
 */

static void sio2_clear_int(struct z80_sio_chan *chan, uint8_t m)
{
	if (trace & TRACE_IRQ) {
		printf( "Clear intbits %d %x\n",
			(int)(chan - sio), m);
	}
	chan->intbits &= ~m;
	chan->pending &= ~m;
	/* Check me - does it auto clear down or do you have to reti it ? */
	if (!(sio->intbits | sio[1].intbits)) {
		sio->rr[1] &= ~0x02;
		chan->irq = 0;
	}
	recalc_interrupts();
}

static void sio2_raise_int(struct z80_sio_chan *chan, uint8_t m)
{
	uint8_t new = (chan->intbits ^ m) & m;
	chan->intbits |= m;
	if ((trace & TRACE_SIO) && new)
		printf( "SIO raise int %x new = %x\n", m, new);
	if (new) {
		if (!sio->irq) {
			chan->irq = 1;
			sio->rr[1] |= 0x02;
			recalc_interrupts();
		}
	}
}

static void sio2_reti(struct z80_sio_chan *chan)
{
	/* Recalculate the pending state and vectors */
	/* FIXME: what really goes here */
	sio->irq = 0;
	recalc_interrupts();
}

static int sio2_check_im2(struct z80_sio_chan *chan)
{
	uint8_t vector = sio[1].wr[2];
	/* See if we have an IRQ pending and if so deliver it and return 1 */
	if (chan->irq) {
		/* Do the vector calculation in the right place */
		/* FIXME: move this to other platforms */
		if (sio[1].wr[1] & 0x04) {
			/* This is a subset of the real options. FIXME: add
			   external status change */
			if (sio[1].wr[1] & 0x04) {
				vector &= 0xF1;
				if (chan == sio)
					vector |= 1 << 3;
				if (chan->intbits & INT_RX)
					vector |= 4;
				else if (chan->intbits & INT_ERR)
					vector |= 2;
			}
			if (trace & TRACE_SIO)
				printf( "SIO2 interrupt %02X\n", vector);
			chan->vector = vector;
		} else {
			chan->vector = vector;
		}
		if (trace & (TRACE_IRQ|TRACE_SIO))
			printf( "New live interrupt pending is SIO (%d:%02X).\n",
				(int)(chan - sio), chan->vector);
		if (chan == sio)
			live_irq = IRQ_SIOA;
		else
			live_irq = IRQ_SIOB;
		Z80INT(&cpu_z80, chan->vector);
		return 1;
	}
	return 0;
}

/*
 *	The SIO replaces the last character in the FIFO on an
 *	overrun.
 */
static void sio2_queue(struct z80_sio_chan *chan, uint8_t c)
{
	if (trace & TRACE_SIO)
		printf( "SIO %d queue %d: ", (int) (chan - sio), c);
	/* Receive disabled */
	if (!(chan->wr[3] & 1)) {
		printf( "RX disabled.\n");
		return;
	}
	/* Overrun */
	if (chan->dptr == 2) {
		if (trace & TRACE_SIO)
			printf( "Overrun.\n");
		chan->data[2] = c;
		chan->rr[1] |= 0x20;	/* Overrun flagged */
		/* What are the rules for overrun delivery FIXME */
		sio2_raise_int(chan, INT_ERR);
	} else {
		/* FIFO add */
		if (trace & TRACE_SIO)
			printf( "Queued %d (mode %d)\n", chan->dptr, chan->wr[1] & 0x18);
		chan->data[chan->dptr++] = c;
		chan->rr[0] |= 1;
		switch (chan->wr[1] & 0x18) {
		case 0x00:
			break;
		case 0x08:
			if (chan->dptr == 1)
				sio2_raise_int(chan, INT_RX);
			break;
		case 0x10:
		case 0x18:
			sio2_raise_int(chan, INT_RX);
			break;
		}
	}
	/* Need to deal with interrupt results */
}

static void sio2_channel_timer(struct z80_sio_chan *chan, uint8_t ab)
{
	if (ab == 0) {
		int c = check_chario();
//		printf("Check chario %i %i \n\r",c,sio2_input);
		if (sio2_input) {
			if (c & 1){
			        if(chan->dptr<1){ //prevent overrun
				  sio2_queue(chan, next_char());
//				  printf("Added to q");
				}
			}
		}
		if (c & 2) {
			if (!(chan->rr[0] & 0x04)) {
				chan->rr[0] |= 0x04;
				if (chan->wr[1] & 0x02)
					sio2_raise_int(chan, INT_TX);
			}
		}
	} else {
		if (!(chan->rr[0] & 0x04)) {
			chan->rr[0] |= 0x04;
			if (chan->wr[1] & 0x02)
				sio2_raise_int(chan, INT_TX);
		}
	}
}

static void sio2_timer(void)
{
	sio2_channel_timer(sio, 0);
	sio2_channel_timer(sio + 1, 1);
}

static void sio2_channel_reset(struct z80_sio_chan *chan)
{
	chan->rr[0] = 0x2C;
	chan->rr[1] = 0x01;
	chan->rr[2] = 0;
	sio2_clear_int(chan, INT_RX | INT_TX | INT_ERR);
}

static void sio_reset(void)
{
	sio2_channel_reset(sio);
	sio2_channel_reset(sio + 1);
}

static uint8_t sio2_read(uint16_t addr)
{
	struct z80_sio_chan *chan = (addr & 2) ? sio + 1 : sio;
	if (!(addr & 1)) {
		/* Control */
		uint8_t r = chan->wr[0] & 007;
		chan->wr[0] &= ~007;

		chan->rr[0] &= ~2;
		if (chan == sio && (sio[0].intbits | sio[1].intbits))
			chan->rr[0] |= 2;
		if (trace & TRACE_SIO)
			printf( "sio%c read reg %d = ", (addr & 2) ? 'b' : 'a', r);
		switch (r) {
		case 0:
		case 1:
			if (trace & TRACE_SIO)
				printf( "%02X\n", chan->rr[r]);
			return chan->rr[r];
		case 2:
			if (chan != sio) {
				if (trace & TRACE_SIO)
					printf( "%02X\n", chan->rr[2]);
				return chan->rr[2];
			}
		case 3:
			/* What does the hw report ?? */
			printf( "INVALID(0xFF)\n");
			return 0xFF;
		}
	} else {
		/* FIXME: irq handling */
		uint8_t c = chan->data[0];
		chan->data[0] = chan->data[1];
		chan->data[1] = chan->data[2];
		if (chan->dptr)
			chan->dptr--;
		if (chan->dptr == 0)
			chan->rr[0] &= 0xFE;	/* Clear RX pending */
		sio2_clear_int(chan, INT_RX);
		chan->rr[0] &= 0x3F;
		chan->rr[1] &= 0x3F;
		if (trace & TRACE_SIO)
			printf( "sio%c read data %d\n", (addr & 2) ? 'b' : 'a', c);
		if (chan->dptr && (chan->wr[1] & 0x10))
			sio2_raise_int(chan, INT_RX);
		return c;
	}
	return 0xFF;
}

static void sio2_write(uint16_t addr, uint8_t val)
{
	struct z80_sio_chan *chan = (addr & 2) ? sio + 1 : sio;
	uint8_t r;
	if (!(addr & 1)) {
		if (trace & TRACE_SIO)
			printf( "sio%c write reg %d with %02X\n", (addr & 2) ? 'b' : 'a', chan->wr[0] & 7, val);
		switch (chan->wr[0] & 007) {
		case 0:
			chan->wr[0] = val;
			/* FIXME: CRC reset bits ? */
			switch (val & 070) {
			case 000:	/* NULL */
				break;
			case 010:	/* Send Abort SDLC */
				/* SDLC specific no-op for async */
				break;
			case 020:	/* Reset external/status interrupts */
				sio2_clear_int(chan, INT_ERR);
				chan->rr[1] &= 0xCF;	/* Clear status bits on rr0 */
				break;
			case 030:	/* Channel reset */
				if (trace & TRACE_SIO)
					printf( "[channel reset]\n");
				sio2_channel_reset(chan);
				break;
			case 040:	/* Enable interrupt on next rx */
				chan->rxint = 1;
				break;
			case 050:	/* Reset transmitter interrupt pending */
				chan->txint = 0;
				sio2_clear_int(chan, INT_TX);
				break;
			case 060:	/* Reset the error latches */
				chan->rr[1] &= 0x8F;
				break;
			case 070:	/* Return from interrupt (channel A) */
				if (chan == sio) {
					sio->irq = 0;
					sio->rr[1] &= ~0x02;
					sio2_clear_int(sio, INT_RX | INT_TX | INT_ERR);
					sio2_clear_int(sio + 1, INT_RX | INT_TX | INT_ERR);
				}
				break;
			}
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			r = chan->wr[0] & 7;
			if (trace & TRACE_SIO)
				printf( "sio%c: wrote r%d to %02X\n",
					(addr & 2) ? 'b' : 'a', r, val);
			chan->wr[r] = val;
			if (chan != sio && r == 2)
				chan->rr[2] = val;
			chan->wr[0] &= ~007;
			break;
		}
		/* Control */
	} else {
		/* Strictly we should emulate this as two bytes, one going out and
		   the visible queue - FIXME */
		/* FIXME: irq handling */
		chan->rr[0] &= ~(1 << 2);	/* Transmit buffer no longer empty */
		chan->txint = 1;
		/* Should check chan->wr[5] & 8 */
		sio2_clear_int(chan, INT_TX);
		if (trace & TRACE_SIO)
			printf( "sio%c write data %d\n", (addr & 2) ? 'b' : 'a', val);
		if (chan == sio)
//			write(1, &val, 1);
			out_char(&val);
		else {
//			write(1, "\033[1m;", 5);
			//write(1, &val,1);
			out_char(&val);
//			write(1, "\033[0m;", 5);
		}
	}
}


static uint8_t my_ide_read(uint16_t addr)
{
	uint8_t r =  ide_read8(ide0, addr);
	if (trace & TRACE_IDE)
		printf( "ide read %d = %02X\n", addr, r);
	return r;
}

static void my_ide_write(uint16_t addr, uint8_t val)
{
	if (trace & TRACE_IDE)
		printf( "ide write %d = %02X\n", addr, val);
	ide_write8(ide0, addr, val);
}

struct rtc *rtc;

/*
 *	Z80 CTC
 */

struct z80_ctc {
	uint16_t count;
	uint16_t reload;
	uint8_t vector;
	uint8_t ctrl;
#define CTC_IRQ		0x80
#define CTC_COUNTER	0x40
#define CTC_PRESCALER	0x20
#define CTC_RISING	0x10
#define CTC_PULSE	0x08
#define CTC_TCONST	0x04
#define CTC_RESET	0x02
#define CTC_CONTROL	0x01
	uint8_t irq;		/* Only valid for channel 0, so we know
				   if we must wait for a RETI before doing
				   a further interrupt */
};

#define CTC_STOPPED(c)	(((c)->ctrl & (CTC_TCONST|CTC_RESET)) == (CTC_TCONST|CTC_RESET))

struct z80_ctc ctc[4];
uint8_t ctc_irqmask;

/*
   Emulate pages rom card. 
 */
static void toggle_rom(void)
{
        if (romdisable == 0) {
                if (trace & TRACE_ROM) printf( "[ROM out(disabled)]\n");
                romdisable =1;
        } else {
                if (trace & TRACE_ROM) printf( "[ROM in(enabled) ATTEMPTED  ]\n");
//              romdisable =0;
        }

}

static void PIOA_init(void){
//init gpio ports
    int a;
    for (a=0;a<8;a++){
       gpio_init(PIOAp[a]);
    }

}

static uint8_t PIOA_read(void){
// set as inputs and read
    int a;
    uint8_t v=1;
    uint8_t r=0;
    //set pullups make input
    for (a=0;a<8;a++){
       gpio_set_dir(PIOAp[a],GPIO_IN);
       gpio_pull_up(PIOAp[a]);
    }
    sleep_ms(1);
    //get bits disable pullups
    for (a=0;a<8;a++){   
       if(gpio_get(PIOAp[a]))r=r+v;
       gpio_disable_pulls(PIOAp[a]);
       v=v << 1;
    }
    return r;

}

static void PIOA_write(uint8_t val){
//set as outputs and write
  int a;
  uint8_t v=1;
  for (a=0;a<8;a++){
    gpio_set_dir(PIOAp[a],GPIO_OUT);
    gpio_put(PIOAp[a],v & val); 
    v=v << 1;
  }

}


static uint8_t io_read_2014(uint16_t addr)
{
	if (trace & TRACE_IO)
		printf( "read %02x\n", addr);
	if ((addr & 0xFF) == 0xBA) {
		return 0xCC;
	}

	addr &= 0xFF;

	if ((addr >= 0xA0 && addr <= 0xA7) && acia && acia_narrow == 1)
		return acia_read(acia, addr & 1);
	if ((addr >= 0x80 && addr <= 0x87) && acia && acia_narrow == 2)
		return acia_read(acia, addr & 1);
	if ((addr >= 0x80 && addr <= 0xBF) && acia && !acia_narrow)
		return acia_read(acia, addr & 1);
	if ((addr >= 0x80 && addr <= 0x87) && sio2 )
		return sio2_read(addr & 3);
	if ((addr >= 0x10 && addr <= 0x17) && ide == 1)
		return my_ide_read(addr & 7);
	if (addr >= 0xA0 && addr <= 0xA7 && have_16x50)
		return uart_read(&uart[0], addr & 7);
	else if (addr==PIOA) return PIOA_read();	
	else if (addr==SPO256Port) return SPO256DataReady;
	else if (addr==BeepPort) return BeepDataReady;

	if (trace & TRACE_UNK)
		printf( "Unknown read from port %04X\n", addr);
	return 0x78;	/* 78 is what my actual board floats at */
}

static void io_write_2014(uint16_t addr, uint8_t val, uint8_t known)
{
	if (trace & TRACE_IO)
		printf( "write %02x <- %02x\n", addr, val);

	if ((addr & 0xFF) == 0xBA) {
		/* Quart */
		return;
	}
	addr &= 0xFF;
	if ((addr >= 0xA0  && addr <= 0xA7) && acia && acia_narrow == 1)
		acia_write(acia, addr & 1, val);
	else if ((addr >= 0x80 && addr <= 0x87) && acia && acia_narrow == 2)
		acia_write(acia, addr & 1, val);
	else if ((addr >= 0x80 && addr <= 0xBF) && acia && !acia_narrow)
		acia_write(acia, addr & 1, val);
	else if ((addr >= 0x80 && addr <= 0x87) && sio2 )
		sio2_write(addr & 3, val);
	else if ((addr >= 0x10 && addr <= 0x17) && ide == 1)
		my_ide_write(addr & 7, val);
	else if (addr >= 0xA0 && addr <= 0xA7 && have_16x50)
		uart_write(&uart[0], addr & 7, val);
	/* The switchable/pageable ROM is not very well decoded */
	else if (switchrom && (addr & 0x7F) >= 0x38 && (addr & 0x7F) <= 0x3F)
		toggle_rom();
	else if (addr==PIOA)PIOA_write(val);	
	else if (addr==SPO256Port){SPO256DataOut=val;SPO256DataReady=1;}
	else if (addr==BeepPort){BeepDataOut=val;BeepDataReady=1;}
	else if (addr == 0xFD) {
		trace &= 0xFF00;
		trace |= val;
		printf( "trace set to %04X\n", trace);
	} else if (addr == 0xFE) {
		trace &= 0xFF;
		trace |= val << 8;
		printf("trace set to %d\n", trace);
	} else if (!known && (trace & TRACE_UNK))
		printf( "Unknown write to port %04X of %02X\n", addr, val);
}



void io_write(int unused, uint16_t addr, uint8_t val)
{
		io_write_2014(addr, val, 0);
}

uint8_t io_read(int unused, uint16_t addr)
{
		return io_read_2014(addr);
}

static void poll_irq_event(void)
{
		if (acia)
			acia_check_irq(acia);
		uart_check_irq(&uart[0]);
		if (!sio2_check_im2(sio))
		      sio2_check_im2(sio + 1);
}

static void reti_event(void)
{
		if (sio2) {
			sio2_reti(sio);
			sio2_reti(sio + 1);
		}
	live_irq = 0;
	poll_irq_event();
}


void dumpPC(Z80Context* z80ctx){
    printf("PC %04x\n",z80ctx->PC);
}

void init_pico_uart(void){
    // Set up our UART with a basic baud rate.
    uart_init(UART_ID, 2400);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Actually, we want a different speed
    // The call will return the actual baud rate selected, which will be as close as
    // possible to that requested
    int __unused actual = uart_set_baudrate(UART_ID, BAUD_RATE);

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);
    
    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART0_IRQ,intUARTcharwaiting );
    irq_set_enabled(UART0_IRQ, true);
    
    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, false);

//    uart_puts(UART_ID, "\nUart INIT OK\n\r");
}


void setup_led(void){
  gpio_init(LEDPIN);
  gpio_set_dir(LEDPIN, GPIO_OUT);
}


void flash_led(int t){
  //flash LED
  gpio_put(LEDPIN, 1);
  sleep_ms(t);
  gpio_put(LEDPIN, 0);
  sleep_ms(t);

}


void PrintToSelected(char * string, int all){
    int uu=UseUsb;
    if (all) uu=3;
    if(uu==0 || uu==3){
         uart_puts(UART_ID, string);
    }     
    if(uu==1 || uu==3){
         printf("%s",string);
    }
}



void WriteRamromToSd(FRESULT fr,char * filename,int writesize,int readfromram){
    if (readfromram){
      printf("\n##### Writing %s to SD from RAM for %04x bytes #####\n\r",filename,writesize);
    }else{
      printf("\n##### Writing %s to SD from ROM for %04x bytes #####\n\r",filename,writesize);
    }
    FIL fil;
    fr = f_open(&fil, filename, FA_WRITE | FA_OPEN_APPEND);
    if (FR_OK != fr && FR_EXIST != fr){
        panic("\nf_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
    }else{
      int a;
      char c;
      UINT bw;
      for(a=0;a<writesize;a++){
        printf("%02x ",ram[a]);
        if (readfromram){
          c=ram[a];
        }else{
          c=rom[a];
        }
        fr= f_write(&fil, &c, sizeof c, &bw);
      }
    }
    fr = f_close(&fil);
}

void ReadSdToRamrom(FRESULT fr,const char * filename,int readsize,int SDoffset,int writetoram ){
/*    if(writetoram){
      printf("\n##### Reading %s from SD %04x to RAM  for %04x bytes #####\n\r",filename,SDoffset,readsize);
    }else{
      printf("\n##### Reading %s from SD %04x to ROM  for %04x bytes #####\n\r",filename,SDoffset,readsize);
    }
*/
    FIL fil;
    fr = f_open(&fil, filename, FA_READ); //FA_WRITE
    if (FR_OK != fr && FR_EXIST != fr){
        panic("\nf_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
    }else{
      fr = f_lseek(&fil, SDoffset);

      int a;
      char c;
      UINT br;
      for(a=0;a<readsize;a++){
        fr = f_read(&fil, &c, sizeof c, &br);
        if (br==0){printf("Read Fail");}
        if(writetoram){
          ram[a]=c;
        }else{
          rom[a]=c;
        }
      }

    }
    fr = f_close(&fil);

}


void WriteRamToSD(FRESULT fr,const char * filename,int readsize ){
    char temp[128];
    sprintf(temp,"\n###### Writing %s to SD from RAM  for %04x bytes#####\n\r",filename,readsize);
    PrintToSelected(temp,1);
    FIL fil;
    fr = f_open(&fil, filename, FA_WRITE | FA_CREATE_ALWAYS); //FA_WRITE
    if (FR_OK != fr && FR_EXIST != fr){
        panic("\nf_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
    }else{

      int a;
      char c;
      UINT bw;
      for(a=0;a<readsize;a++){
        c=ram[a];
        fr = f_write(&fil, &c, sizeof c, &bw);
        if (bw==0){printf("Write Fail");}
      }

    }
    fr = f_close(&fil);

}


void CopyRamRom2Ram(int FromAddr, int ToAddr, int copysize, int fromram, int toram){
    int a;
    for(a=0;a<copysize;a++){
      if(fromram){
        if(toram){
          ram[a+ToAddr]=ram[a+FromAddr];
        }else{
          ram[a+ToAddr]=rom[a+FromAddr];
        }
      }else{
        if(toram){
          rom[a+ToAddr]=ram[a+FromAddr];
        }else{
          rom[a+ToAddr]=rom[a+FromAddr];
        }
      }
    }
}

void DumpRamRom(int FromAddr, int dumpsize,int dram){
  int rc=0;
  int a;
  if(dram){
    printf("RAM %04X ",FromAddr);
  }else{
    printf("ROM %04X ",FromAddr);
  }
  for(a=0;a<dumpsize;a++){
    if (dram){
      printf("%02x ",ram[a+FromAddr]);
    }else{
      printf("%02x ",rom[a+FromAddr]);
    }
      rc++;

      if(rc % 32==0){
        printf("\nR-M %04x ",rc+FromAddr);
      }else{
        if (rc % 8==0){
          printf(" ");
        }
      }
  }

}



void DumpMemory(int FromAddr, int dumpsize,FRESULT fr){
  int rc=0;
  int a;
  char temp[128];
  if(romdisable){
        sprintf(temp,"\n\rROM Disabled\n\r");
        PrintToSelected(temp,0);
  }else{
        sprintf(temp,"\n\rROM Enabled\n\r");
        PrintToSelected(temp,0);
  }
  sprintf(temp,"MEM %04X ",FromAddr);
  PrintToSelected(temp,0);
  WriteRamToSD(fr,"DUMP.BIN",0x10000 );
  for(a=0;a<dumpsize;a++){
      sprintf(temp,"%02x ",mem_read0(a+FromAddr));
      PrintToSelected(temp,0);
      rc++;

      if(rc % 32==0){
        sprintf(temp,"\r\nMEM %04x ",rc+FromAddr);
        PrintToSelected(temp,0);
      }else{
        if (rc % 8==0){
          sprintf(temp," ");
          PrintToSelected(temp,0);
        }
      }
  }

}

int ls(const char *dir,const char * search) {
    int filecnt=0;
    char cwdbuf[FF_LFN_BUF] = {0};
    FRESULT fr; /* Return value */
    char const *p_dir;
    if (dir[0]) {
        p_dir = dir;
    } else {
        fr = f_getcwd(cwdbuf, sizeof cwdbuf);
        if (FR_OK != fr) {
            printf("f_getcwd error: %s (%d)\n", FRESULT_str(fr), fr);
            return 0;
        }
        p_dir = cwdbuf;
    }
    printf("%s files %s\n",search, p_dir);
    DIR dj;      /* Directory object */
    FILINFO fno; /* File information */
    memset(&dj, 0, sizeof dj);
    memset(&fno, 0, sizeof fno);
    fr = f_findfirst(&dj, &fno, p_dir, search);
    if (FR_OK != fr) {
        printf("f_findfirst error: %s (%d)\n", FRESULT_str(fr), fr);
        return 0;
    }
    while (fr == FR_OK && fno.fname[0]) { /* Repeat while an item is found */
        /* Create a string that includes the file name, the file size and the
         attributes string. */
        const char *pcWritableFile = "writable file",
                   *pcReadOnlyFile = "read only file",
                   *pcDirectory = "directory";
        const char *pcAttrib;
        /* Point pcAttrib to a string that describes the file. */
        if (fno.fattrib & AM_DIR) {
            pcAttrib = pcDirectory;
        } else if (fno.fattrib & AM_RDO) {
            pcAttrib = pcReadOnlyFile;
        } else {
            pcAttrib = pcWritableFile;
        }
        /* Create a string that includes the file name, the file size and the
         attributes string. */
        printf("%i) %s [%s] [size=%llu]\n", filecnt+1, fno.fname, pcAttrib, fno.fsize);
        if (filecnt<MaxBinFiles){
          sprintf(&BinFiles[filecnt],"%s",fno.fname);
          filecnt++;
        }
        fr = f_findnext(&dj, &fno); /* Search for next item */
    }
    f_closedir(&dj);
    return filecnt++;
}


int GetRomSwitches(){
  gpio_init(HASSwitchesIO); 
  gpio_set_dir(HASSwitchesIO,GPIO_IN);
  gpio_pull_up(HASSwitchesIO);

  gpio_init(ROMA13);
  gpio_set_dir(ROMA13,GPIO_IN);
  gpio_pull_up(ROMA13);
  
  gpio_init(ROMA14);
  gpio_set_dir(ROMA14,GPIO_IN);
  gpio_pull_up(ROMA14);
  
  gpio_init(ROMA15);
  gpio_set_dir(ROMA15,GPIO_IN);
  gpio_pull_up(ROMA15);

//serial port selection swithch
  gpio_init(SELSEL);
  gpio_set_dir(SELSEL,GPIO_IN);
  gpio_pull_up(SELSEL);

  sleep_ms(1); //wait for io to settle.

  int v=0;
  if (gpio_get(HASSwitchesIO)==1){
    PrintToSelected("\r\nNo Switches, no settings changed \n\r",1);
    HasSwitches=0;
  }else{
    //switches present, use values
    HasSwitches=1;
    rombank=0;
    if (gpio_get(ROMA13))rombank+=1;
    if (gpio_get(ROMA14))rombank+=2;
    if (gpio_get(ROMA15))rombank+=4;
    PrintToSelected("\r\nOverriding INI from ROM/port Switches  \n\r",1);

    if (gpio_get(SELSEL)==1){
        UseUsb=1;
        PrintToSelected("Console Via USB  \n\r",1);
    }else{
        UseUsb=0;
        PrintToSelected("Console Via UART \n\r",1);
    }


//if has switches, then has buttons too.

//setup DUMP gpio
    gpio_init(DUMPBUT);
    gpio_set_dir(DUMPBUT,GPIO_IN);
    gpio_pull_up(DUMPBUT);

//setup RESETBUT gpio
    gpio_init(RESETBUT);
    gpio_set_dir(RESETBUT,GPIO_IN);
    gpio_pull_up(RESETBUT);



  }
  return rombank;

}


int SDFileExists(char * filename){
    FRESULT fr;
    FILINFO fno;

    fr = f_stat(filename, &fno);
    return fr==FR_OK;
}


//############################################################################################################
//################################################# Sound ####################################################
//############################################################################################################

void PlayAllophone(int al){
    int b,s;
    uint8_t v;
    //reset pwm settings (play notes may change them)
    pwm_set_clkdiv(PWMslice,16);
    pwm_set_wrap (PWMslice, 256);
    
    //get length of allophone sound bite
    s=allophonesizeCorrected[al];
    //and play
    for(b=0;b<s;b++){
        v=allophoneindex[al][b]; //get delta value
        sleep_us(PWMrate);
        pwm_set_both_levels(PWMslice,v,v);

    }

}

void PlayAllophones(uint8_t *alist,int listlength){
   int a;
   for(a=0;a<listlength;a++){
     PlayAllophone(alist[a]);
   }
}

void SetPWM(void){
    gpio_init(soundIO1);
    gpio_set_dir(soundIO1,GPIO_OUT);
    gpio_set_function(soundIO1, GPIO_FUNC_PWM);

    gpio_init(soundIO2);
    gpio_set_dir(soundIO2,GPIO_OUT);
    gpio_set_function(soundIO2, GPIO_FUNC_PWM);

    PWMslice=pwm_gpio_to_slice_num (soundIO1);
    pwm_set_clkdiv(PWMslice,16);
    pwm_set_both_levels(PWMslice,0x80,0x80);

    pwm_set_output_polarity(PWMslice,true,false);

    pwm_set_wrap (PWMslice, 256);
    pwm_set_enabled(PWMslice,true);

}

void Beep(uint8_t note){
    int w;     
    //set frequency    
    pwm_set_clkdiv(PWMslice,256);
    if (note>0 && note<128){
      //get divisor from Midi note table.
      w=MidiNoteWrap[note];  
      pwm_set_both_levels(PWMslice,w>>1,w>>1);
      //set frequency from midi note table.
      pwm_set_wrap(PWMslice,w);
    }else{
      pwm_set_both_levels(PWMslice,0x0,0x0);  
    }
}








//############################################################################################################
//################################################# Core 1 ####################################################
//############################################################################################################

void Core2Main(void){
  SetPWM();
  printf("\n#Core 1 Starting#\n");

  //RC2040
  uint8_t alist[] ={AR1,PA3,SS1,SS1,IY1,PA3,TT2,WH1,EH1,EH1,NN1,PA2,PA3,TT2,IY1,PA5,FF1,OR1,PA3,TT2,IY1,PA5};
  PlayAllophones(alist,sizeof(alist));
  
  while(1){
    if(SPO256DataReady>0){
      PlayAllophone(SPO256DataOut);
      SPO256DataReady=0;
    }  
    if(BeepDataReady>0){
      Beep(BeepDataOut);
      BeepDataReady=0;
    }  
    sleep_ms(1);
    tight_loop_contents();
  }
}





















//#######################################################################################################
//#                                      MAIN                                                           #
//#######################################################################################################



int main(int argc, char *argv[])
{
	static struct timespec tc;
		
	int opt;
	int fd;
	int romen = 1;
	int ramonly=0; //disble rom copy 64K romfile directly to ram
	
	//char *idepath = NULL; 
	const char * idepathi ="";
	const char * idepath ="";
	
	int indev;
	char *patha = NULL, *pathb = NULL;
	const char * romfile ="R0001009.BIN"; //default ROM image
        uint16_t romsize =0x2000;
        char temp[250];
	
	int SerialType =0; //ACIA=0 SIO=1

        char RomTitle[200];
        
//over clock done in ini parcer now
        set_sys_clock_khz(250000, true);


	
#define INDEV_ACIA	1
#define INDEV_SIO	2
#define INDEV_CPLD	3
#define INDEV_16C550A	4
#define INDEV_KIO	5

// led gpio
        setup_led();
        flash_led(250);
        stdio_usb_init();
        flash_led(250);
        printf("\n %c[2J\n\n\rUSB INIT OK \n\r",27);
                
//init uart
        init_pico_uart();
        sprintf(RomTitle,"\n %c[2J \n\n\rUART INIT OK \n\r",27);
        uart_puts(UART_ID,RomTitle);


// mount SD Card
        sd_card_t *pSD = sd_get_by_num(0);
        FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
        if (FR_OK != fr){
        // panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
          PrintToSelected("SD INIT FAIL  \n\r",1);
          while(1); //halt
        }

        PrintToSelected("SD INIT OK \n\r",1);

//banner

        sprintf(RomTitle, "\n\n\r     ________________________________");PrintToSelected(RomTitle,1);
        sprintf(RomTitle,   "\n\r    /                                |");PrintToSelected(RomTitle,1);
        sprintf(RomTitle,   "\n\r   /          PICO RC2040            |");PrintToSelected(RomTitle,1);
        sprintf(RomTitle,   "\n\r  /                                  |");PrintToSelected(RomTitle,1);
        sprintf(RomTitle,   "\n\r |  O                                |");PrintToSelected(RomTitle,1);
        sprintf(RomTitle,   "\n\r |          Derek Woodroffe          |");PrintToSelected(RomTitle,1);
        sprintf(RomTitle,   "\n\r |               2022                |");PrintToSelected(RomTitle,1);
        //sprintf(RomTitle,   "\n\r |                                   |");PrintToSelected(RomTitle,1);
        sprintf(RomTitle,   "\n\r |___________________________________|");PrintToSelected(RomTitle,1);
        sprintf(RomTitle,   "\n\r   | | | | | | | | | | | | | | | | |  \n\n\r");PrintToSelected(RomTitle,1);




// inifile parse
	dictionary * ini ;
	char       * ini_name ;
        const char  *   s ;
        const char * inidesc;
                
        int overclock;
        int jpc;
        int iscf=0;

       	ini_name = "rc2040.ini";

	
	if (SDFileExists(ini_name)){
	  sprintf(temp,"Ini file %s Exists Loading ... \n\r",ini_name);
	  PrintToSelected(temp,1);

//########################################### INI Parser ######################################		
	  ini = iniparser_load(fr, ini_name);
	  //iniparser_dump(ini, stdout);

	  // ROM select from ini
	  rombank=0;
	  if (iniparser_getint(ini, "ROM:a13", 0)) rombank+=1;
	  if (iniparser_getint(ini, "ROM:a14", 0)) rombank+=2;
	  if (iniparser_getint(ini, "ROM:a15", 0)) rombank+=4;

	  // ROMfile from ini
	  romfile = iniparser_getstring(ini, "ROM:romfile", romfile);
	  romsize = iniparser_getint(ini, "ROM:romsize", 0x2000);

	  // RAMonly load
	  ramonly =iniparser_getint(ini, "ROM:ramonly", 0);

	  // start at
	  jpc = iniparser_getint(ini, "ROM:jumpto", 0);

          // Use Ide
	  ide=iniparser_getint(ini, "IDE:ide",1);
	  
	  // IDE cf file
	  iscf=iniparser_getint(ini, "IDE:iscf", iscf);
	  
	  idepathi =iniparser_getstring(ini, "IDE:idefilei", "");
	  idepath =iniparser_getstring(ini, "IDE:idefile", idepath);
	  
	  // USB or UART
	  UseUsb=iniparser_getint(ini, "CONSOLE:port", 1);

	  // ACIA /SERIAL
          SerialType=iniparser_getint(ini, "EMULATION:serialtype",0 );
          
          // ININAME
          inidesc=iniparser_getstring(ini, "EMULATION:inidesc","Default ini" );
          sprintf(temp,"INI Description: %s \n\r",inidesc,1);
          PrintToSelected(temp,1);

          // Trace enable from inifile
	  trace=iniparser_getint(ini, "DEBUG:trace",0 );

	  // PORT
	  PIOA=iniparser_getint(ini, "[PORT]:pioa",0 );

	  SPO256Port=iniparser_getint(ini, "[PORT]:spo256",0x30 );
	  BeepPort=iniparser_getint(ini, "[PORT]:beep",0x31 );

          // Overclock
	  overclock=iniparser_getint(ini, "SPEED:overclock",0 );
	  if (overclock>0){
	        sprintf(temp,"Overclock to %i000\n\r",overclock,1);
	        PrintToSelected(temp,1);
	  	set_sys_clock_khz(overclock*1000, true);
          }

	  //iniparser_freedict(ini); // cant free, settings are pointed to dictionary.

	  PrintToSelected("Loaded INI\n\r",1);	

//########################################### End of INI Parser ###########################


//IF switches link present, get switches and select rom bank and UART from switches
          rombank=GetRomSwitches();
                  
        }else{
          uart_puts(UART_ID,"No  \n\r");
          printf("SD INIT OK \n\r",1);
        }

        flash_led(200);
        if (UseUsb){
        PrintToSelected("\rWaiting for USB to connect\n\r",1);
          //if usb wait for usb to connect.
          while (!tud_cdc_connected()) { sleep_ms(100);  }
        }

// "command line" settings"

//init PIO
        if(PIOA<256) PIOA_init();





//init Emulation
        cpuboard=0;  //remove shouldnt be needed any more

	//Fast -f // probably doesnt mean anything anymore no nano delay.
	fast = 1;

	if (SerialType==0){
  	  //ACIA enable 
  	  
  	  //SIO
	  sio2 = 0;
	  sio2_input = 0;
	  //ACIA
	  have_acia = 1;
	  indev = INDEV_ACIA;
          acia_narrow = 0;
          PrintToSelected("\rACIA selected\n\r",1);
        }else{
          //SIO enable 
          
          //SIO
          sio2 = 1;
          sio2_input = 1;
          indev = INDEV_SIO;
          //ACIA
          have_acia = 0;
          acia_narrow = 0;
          PrintToSelected("\rSIO selected\n\r",1);
        }
        
        //ram only system, hey we are emulating this, we can do ANYTHING!! 
        if (ramonly==1){
          // Read RAM from SD
          ReadSdToRamrom(fr,romfile,0x10000,0x0000,USERAM);   //load 64K image to ram
          sprintf(RomTitle,"Loading: '%s' 64K RAM only image - CPM CF File:'%s %s' \n\r",romfile,idepathi,idepath);
          romdisable =1; //disable romswitching
        }else{
          // Read Rom from SD
          ReadSdToRamrom(fr,romfile,romsize,0x2000*rombank,USEROM);   //load directly to rom
          sprintf(RomTitle,"Loading: '%s'[rombank:%i] for 0x%X bytes \n\r",romfile,rombank,romsize);
          PrintToSelected(RomTitle,1);
          sprintf(RomTitle,"CPM/IDE File:'%s %s' \n\r",idepath,idepathi);
        }
        PrintToSelected(RomTitle,1);

        have_ctc = 0;
        have_16x50 = 0;
	tstate_steps = 500;

  	if (ide == 1 ) {
		FIL fili;
		FIL fild;
		ide0 = ide_allocate("cf");
		if (ide0) {
			if (iscf==0){
                          FRESULT ide_fri=f_open(&fili, idepathi, FA_READ | FA_WRITE);
                          if (ide_fri != FR_OK) {
                              printf("Error IDE ident file Open Fail %s ",idepathi);
                              ide = 0;
                              if (trace & TRACE_IDE) printf( "IDE0 ident file Open fail");
                          }    
                        }
                        FRESULT ide_frd=f_open(&fild, idepath, FA_READ | FA_WRITE);
                        if ( ide_frd != FR_OK) {
				//perror(idepath);
				printf("Error IDE Data Open Fail %s ",idepath);
				ide = 0;
				if (trace & TRACE_IDE) printf( "IDE0 Data Open fail");
			}
			if (ide_attach(ide0, 0, fili,fild,iscf) == 0) {
				ide = 1;
				ide_reset_begin(ide0);
				if (trace & TRACE_IDE) printf( "IDE0 Open OK");
			}
		} else
			ide = 0;
	}

	if (have_acia) {
		acia = acia_create();
		if (trace & TRACE_ACIA)
			acia_trace(acia, 1);
	}
	if (sio2)
		sio_reset();
	if (have_16x50)
		uarta_init(&uart[0], indev == INDEV_16C550A ? 1: 0);
	switch(indev) {
	case INDEV_ACIA:
		acia_set_input(acia, 1);
		break;
	case INDEV_SIO:
		sio2_input = 1;
		break;
	case INDEV_CPLD:
		break;
	case INDEV_16C550A:
		break;
	default:
		printf( "Invalid input device %d.\n", indev);
	}

//Start Core2
        multicore_launch_core1(Core2Main);


	tc.tv_sec = 0;
	tc.tv_nsec = 20000000L;

	Z80RESET(&cpu_z80);
	//nonstandard start vector
	if(jpc){
	  cpu_z80.PC=jpc;
          sprintf(temp,"Starting at 0x%04X \n\r",jpc);
          PrintToSelected(temp,1);

	}
	
	cpu_z80.ioRead = io_read;
	cpu_z80.ioWrite = io_write;
	cpu_z80.memRead = mem_read;
	cpu_z80.memWrite = mem_write;
	cpu_z80.trace = z80_trace;

	PrintToSelected("\r\n #####################################\n\r",0);
	PrintToSelected(" ########## RC2040 STARTING ##########\n\r",0);
	PrintToSelected(" #####################################\n\n\r",0);

	/* This is the wrong way to do it but it's easier for the moment. We
	   should track how much real time has occurred and try to keep cycle
	   matched with that. The scheme here works fine except when the host
	   is loaded though */

	/* We run 7372000 t-states per second */
	/* We run 365 cycles per I/O check, do that 50 times then poll the
	   slow stuff and nap for 20ms to get 50Hz on the TMS99xx */
	while (!emulator_done) {
		int i;
		/* 36400 T states for base RC2014 - varies for others */
                if(HasSwitches){       
	            if(gpio_get(DUMPBUT)==0){
                        DumpMemory(0,0x10000,fr);
                        while(gpio_get(DUMPBUT)==0);
                    }

                    if(gpio_get(RESETBUT)==0) {
                        PrintToSelected("\r\n ########################### \n\r",0);
                        PrintToSelected(" ####### Z80 RESET ######### \n\r",0);
                        PrintToSelected(" ########################### \n\n\r",0);
                        Z80RESET(&cpu_z80);
                        while(gpio_get(RESETBUT)==0);
                    }
                }

		for (i = 0; i < 40; i++) {  //origional
//              for (i = 0; i < 100; i++) {
			int j;
//			for (j = 0; j < 100; j++) {//origional
			for (j = 0; j < 50; j++) {
		   	    Z80ExecuteTStates(&cpu_z80, (tstate_steps + 5)/ 10);
			}
			if (acia)
				acia_timer(acia);
			if (sio2)
				sio2_timer();
			if (have_16x50)
				uart_event(&uart[0]);
			/* We want to run UI events regularly it seems */
//			ui_event();
		}
		
		//fake USB char in interrupts
		if (UseUsb==1) intUSBcharwaiting();
		
		/* Do 20ms of I/O and delays */
//		if (!fast) sleep_ms(20);

		if (int_recalc) {
			/* If there is no pending Z80 vector IRQ but we think
			   there now might be one we use the same logic as for
			   reti */
			if (!live_irq )
				poll_irq_event();
			/* Clear this after because reti_event may set the
			   flags to indicate there is more happening. We will
			   pick up the next state changes on the reti if so */
			if (!(cpu_z80.IFF1|cpu_z80.IFF2))
				int_recalc = 0;
		}
	}
}


