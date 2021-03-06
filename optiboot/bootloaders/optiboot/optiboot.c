/**********************************************************/
/* Optiboot bootloader for Arduino                        */
/*                                                        */
/* http://optiboot.googlecode.com                         */
/*                                                        */
/* Arduino-maintained version : See README.TXT            */
/* http://code.google.com/p/arduino/                      */
/*  It is the intent that changes not relevant to the     */
/*  Arduino production envionment get moved from the      */
/*  optiboot project to the arduino project in "lumps."   */
/*                                                        */
/* Heavily optimised bootloader that is faster and        */
/* smaller than the Arduino standard bootloader           */
/*                                                        */
/* Enhancements:                                          */
/*   Fits in 512 bytes, saving 1.5K of code space         */
/*   Background page erasing speeds up programming        */
/*   Higher baud rate speeds up programming               */
/*   Written almost entirely in C                         */
/*   Customisable timeout with accurate timeconstant      */
/*   Optional virtual UART. No hardware UART required.    */
/*                                                        */
/* What you lose:                                         */
/*   Implements a skeleton STK500 protocol which is       */
/*     missing several features including EEPROM          */
/*     programming and non-page-aligned writes            */
/*   High baud rate breaks compatibility with standard    */
/*     Arduino flash settings                             */
/*                                                        */
/* Fully supported:                                       */
/*   ATmega168 based devices  (Diecimila etc)             */
/*   ATmega328P based devices (Duemilanove etc)           */
/*                                                        */
/* Beta test (believed working.)                          */
/*   ATmega8 based devices (Arduino legacy)               */
/*   ATmega328 non-picopower devices                      */
/*   ATmega644P based devices (Sanguino)                  */
/*   ATmega1284P based devices                            */
/*   ATmega1280 based devices (Arduino Mega)              */
/*                                                        */
/* Alpha test                                             */
/*   ATmega32                                             */
/*                                                        */
/* Work in progress:                                      */
/*   ATtiny84 based devices (Luminet)                     */
/*                                                        */
/* Does not support:                                      */
/*   USB based devices (eg. Teensy, Leonardo)             */
/*                                                        */
/* Assumptions:                                           */
/*   The code makes several assumptions that reduce the   */
/*   code size. They are all true after a hardware reset, */
/*   but may not be true if the bootloader is called by   */
/*   other means or on other hardware.                    */
/*     No interrupts can occur                            */
/*     UART and Timer 1 are set to their reset state      */
/*     SP points to RAMEND                                */
/*                                                        */
/* Code builds on code, libraries and optimisations from: */
/*   stk500boot.c          by Jason P. Kyle               */
/*   Arduino bootloader    http://arduino.cc              */
/*   Spiff's 1K bootloader http://spiffie.org/know/arduino_1k_bootloader/bootloader.shtml */
/*   avr-libc project      http://nongnu.org/avr-libc     */
/*   Adaboot               http://www.ladyada.net/library/arduino/bootloader.html */
/*   AVR305                Atmel Application Note         */
/*                                                        */
/* This program is free software; you can redistribute it */
/* and/or modify it under the terms of the GNU General    */
/* Public License as published by the Free Software       */
/* Foundation; either version 2 of the License, or        */
/* (at your option) any later version.                    */
/*                                                        */
/* This program is distributed in the hope that it will   */
/* be useful, but WITHOUT ANY WARRANTY; without even the  */
/* implied warranty of MERCHANTABILITY or FITNESS FOR A   */
/* PARTICULAR PURPOSE.  See the GNU General Public        */
/* License for more details.                              */
/*                                                        */
/* You should have received a copy of the GNU General     */
/* Public License along with this program; if not, write  */
/* to the Free Software Foundation, Inc.,                 */
/* 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA */
/*                                                        */
/* Licence can be viewed at                               */
/* http://www.fsf.org/licenses/gpl.txt                    */
/*                                                        */
/**********************************************************/


/**********************************************************/
/*                                                        */
/* Optional defines:                                      */
/*                                                        */
/**********************************************************/
/*                                                        */
/* BIG_BOOT:                                              */
/* Build a 1k bootloader, not 512 bytes. This turns on    */
/* extra functionality.                                   */
/*                                                        */
/* BAUD_RATE:                                             */
/* Set bootloader baud rate.                              */
/*                                                        */
/* LUDICROUS_SPEED:                                       */
/* 230400 baud :-)                                        */
/*                                                        */
/* SOFT_UART:                                             */
/* Use AVR305 soft-UART instead of hardware UART.         */
/*                                                        */
/* LED_START_FLASHES:                                     */
/* Number of LED flashes on bootup.                       */
/*                                                        */
/* LED_DATA_FLASH:                                        */
/* Flash LED when transferring data. For boards without   */
/* TX or RX LEDs, or for people who like blinky lights.   */
/*                                                        */
/* SUPPORT_EEPROM:                                        */
/* Support reading and writing from EEPROM. This is not   */
/* used by Arduino, so off by default.                    */
/*                                                        */
/* TIMEOUT_MS:                                            */
/* Bootloader timeout period, in milliseconds.            */
/* 500,1000,2000,4000,8000 supported.                     */
/*                                                        */
/* UART:                                                  */
/* UART number (0..n) for devices with more than          */
/* one hardware uart (644P, 1284P, etc)                   */
/*                                                        */
/* RADIO_UART:                                            */
/* Try to initialise an NRF24L01 chip if one is connected */
/* to the SPI pins and treat received bytes like those    */
/* received over UART.  This uses the 1M, ShockBurst(tm)  */
/* mode for simplicity. Slave address will be read from   */
/* the EEPROM, needs to be set up first.                  */
/*                                                        */
/**********************************************************/

/**********************************************************/
/* Version Numbers!                                       */
/*                                                        */
/* Arduino Optiboot now includes this Version number in   */
/* the source and object code.                            */
/*                                                        */
/* Version 3 was released as zip from the optiboot        */
/*  repository and was distributed with Arduino 0022.     */
/* Version 4 starts with the arduino repository commit    */
/*  that brought the arduino repository up-to-date with   */
/*  the optiboot source tree changes since v3.            */
/* Version 5 was created at the time of the new Makefile  */
/*  structure (Mar, 2013), even though no binaries changed*/
/* It would be good if versions implemented outside the   */
/*  official repository used an out-of-seqeunce version   */
/*  number (like 104.6 if based on based on 4.5) to       */
/*  prevent collisions.                                   */
/*                                                        */
/**********************************************************/

/**********************************************************/
/* Edit History:					                                */
/*							                                          */
/* Jan 2019                                               */
/* 6.0 Connectage/mathieu:                                */
/*   Remove SOFT_UART, VIRTUALBOOT, RADIO_UART, SEQN      */
/* Mar 2013                                               */
/* 5.0 WestfW: Major Makefile restructuring.              */
/*             See Makefile and pin_defs.h                */
/*             (no binary changes)                        */
/*                                                        */
/* 4.6 WestfW/Pito: Add ATmega32 support                  */
/* 4.6 WestfW/radoni: Don't set LED_PIN as an output if   */
/*                    not used. (LED_START_FLASHES = 0)   */
/* Jan 2013						  */
/* 4.6 WestfW/dkinzer: use autoincrement lpm for read     */
/* 4.6 WestfW/dkinzer: pass reset cause to app in R2      */
/* Mar 2012                                               */
/* 4.5 WestfW: add infrastructure for non-zero UARTS.     */
/* 4.5 WestfW: fix SIGNATURE_2 for m644 (bad in avr-libc) */
/* Jan 2012:                                              */
/* 4.5 WestfW: fix NRWW value for m1284.                  */
/* 4.4 WestfW: use attribute OS_main instead of naked for */
/*             main().  This allows optimizations that we */
/*             count on, which are prohibited in naked    */
/*             functions due to PR42240.  (keeps us less  */
/*             than 512 bytes when compiler is gcc4.5     */
/*             (code from 4.3.2 remains the same.)        */
/* 4.4 WestfW and Maniacbug:  Add m1284 support.  This    */
/*             does not change the 328 binary, so the     */
/*             version number didn't change either. (?)   */
/* June 2011:                                             */
/* 4.4 WestfW: remove automatic soft_uart detect (didn't  */
/*             know what it was doing or why.)  Added a   */
/*             check of the calculated BRG value instead. */
/*             Version stays 4.4; existing binaries are   */
/*             not changed.                               */
/* 4.4 WestfW: add initialization of address to keep      */
/*             the compiler happy.  Change SC'ed targets. */
/*             Return the SW version via READ PARAM       */
/* 4.3 WestfW: catch framing errors in getch(), so that   */
/*             AVRISP works without HW kludges.           */
/*  http://code.google.com/p/arduino/issues/detail?id=368n*/
/* 4.2 WestfW: reduce code size, fix timeouts, change     */
/*             verifySpace to use WDT instead of appstart */
/* 4.1 WestfW: put version number in binary.		  */
/**********************************************************/

#define OPTIBOOT_MAJVER 5
#define OPTIBOOT_MINVER 0

#define MAKESTR(a) #a
#define MAKEVER(a, b) MAKESTR(a*256+b)

asm("  .section .version\n"
    "optiboot_version:  .word " MAKEVER(OPTIBOOT_MAJVER, OPTIBOOT_MINVER) "\n"
    "  .section .text\n");

#include <inttypes.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include "boot.h"
#include "pin_defs.h"
#include "stk500.h"

#ifndef LED_START_FLASHES
#define LED_START_FLASHES 0
#endif

#ifdef LUDICROUS_SPEED
#define BAUD_RATE 230400L
#endif

/* set the UART baud rate defaults */
#ifndef BAUD_RATE
#if F_CPU >= 8000000L
#define BAUD_RATE   115200L // Highest rate Avrdude win32 will support
#elsif F_CPU >= 1000000L
#define BAUD_RATE   9600L   // 19200 also supported, but with significant error
#elsif F_CPU >= 128000L
#define BAUD_RATE   4800L   // Good for 128kHz internal RC
#else
#define BAUD_RATE 1200L     // Good even at 32768Hz
#endif
#endif

#ifndef UART
#define UART 0
#endif

#define BAUD_SETTING (( (F_CPU + BAUD_RATE * 4L) / ((BAUD_RATE * 8L))) - 1 )
#define BAUD_ACTUAL (F_CPU/(8 * ((BAUD_SETTING)+1)))
#define BAUD_ERROR (( 100*(BAUD_RATE - BAUD_ACTUAL) ) / BAUD_RATE)

#if BAUD_ERROR >= 5
#error BAUD_RATE error greater than 5%
#elif BAUD_ERROR <= -5
#error BAUD_RATE error greater than -5%
#elif BAUD_ERROR >= 2
#warning BAUD_RATE error greater than 2%
#elif BAUD_ERROR <= -2
#warning BAUD_RATE error greater than -2%
#endif

#if (F_CPU + BAUD_RATE * 4L) / (BAUD_RATE * 8L) - 1 > 250
#error Unachievable baud rate (too slow) BAUD_RATE
#endif // baud rate slow check
#if (F_CPU + BAUD_RATE * 4L) / (BAUD_RATE * 8L) - 1 < 3
#error Unachievable baud rate (too fast) BAUD_RATE
#endif // baud rate fastn check

/* Watchdog settings */
#define WATCHDOG_OFF    (0)
#define WATCHDOG_16MS   (_BV(WDE))
#define WATCHDOG_32MS   (_BV(WDP0) | _BV(WDE))
#define WATCHDOG_64MS   (_BV(WDP1) | _BV(WDE))
#define WATCHDOG_125MS  (_BV(WDP1) | _BV(WDP0) | _BV(WDE))
#define WATCHDOG_250MS  (_BV(WDP2) | _BV(WDE))
#define WATCHDOG_500MS  (_BV(WDP2) | _BV(WDP0) | _BV(WDE))
#define WATCHDOG_1S     (_BV(WDP2) | _BV(WDP1) | _BV(WDE))
#define WATCHDOG_2S     (_BV(WDP2) | _BV(WDP1) | _BV(WDP0) | _BV(WDE))
#ifndef __AVR_ATmega8__
#define WATCHDOG_4S     (_BV(WDP3) | _BV(WDE))
#define WATCHDOG_8S     (_BV(WDP3) | _BV(WDP0) | _BV(WDE))
#endif

/* Function Prototypes */
/* The main function is in init9, which removes the interrupt vector table */
/* we don't need. It is also 'naked', which means the compiler does not    */
/* generate any entry or exit code itself. */
int main(void) __attribute__ ((OS_main)) __attribute__ ((section (".init9"))) __attribute__ ((__noreturn__));
void putch(char);
uint8_t getch(void);
static inline void getNch(uint8_t); /* "static inline" is a compiler hint to reduce code size */
void verifySpace();
#if LED_START_FLASHES > 0
static inline void flash_led(uint8_t);
#endif
uint8_t getLen();
static inline void watchdogReset();
void watchdogConfig(uint8_t x);
void wait_timeout(void) __attribute__ ((__noreturn__));
void appStart(uint8_t rstFlags) __attribute__ ((naked))  __attribute__ ((__noreturn__));
static int radio_init(void);

static uint8_t radio_mode = 0;
static uint8_t radio_present = 0;
static uint8_t pkt_max_len = 32;

#define CE_DDR		DDRB
#define CE_PORT		PORTB
#define CSN_DDR		DDRB
#define CSN_PORT	PORTB
#define CE_PIN		(1 << 0)
#define CSN_PIN		(1 << 2)

#include "spi.h"
#include "nrf24.h"

/*
 * NRWW memory
 * Addresses below NRWW (Non-Read-While-Write) can be programmed while
 * continuing to run code from flash, slightly speeding up programming
 * time.  Beware that Atmel data sheets specify this as a WORD address,
 * while optiboot will be comparing against a 16-bit byte address.  This
 * means that on a part with 128kB of memory, the upper part of the lower
 * 64k will get NRWW processing as well, even though it doesn't need it.
 * That's OK.  In fact, you can disable the overlapping processing for
 * a part entirely by setting NRWWSTART to zero.  This reduces code
 * space a bit, at the expense of being slightly slower, overall.
 *
 * RAMSTART should be self-explanatory.  It's bigger on parts with a
 * lot of peripheral registers.
 */
#if defined(__AVR_ATmega168__)
#define RAMSTART (0x100)
#define NRWWSTART (0x3800)
#elif defined(__AVR_ATmega328P__) || defined(__AVR_ATmega32__)
#define RAMSTART (0x100)
#define NRWWSTART (0x7000)
#elif defined (__AVR_ATmega644P__)
#define RAMSTART (0x100)
#define NRWWSTART (0xE000)
// correct for a bug in avr-libc
#undef SIGNATURE_2
#define SIGNATURE_2 0x0A
#elif defined (__AVR_ATmega1284P__)
#define RAMSTART (0x100)
#define NRWWSTART (0xE000)
#elif defined(__AVR_ATtiny84__)
#define RAMSTART (0x100)
#define NRWWSTART (0x0000)
#elif defined(__AVR_ATmega1280__)
#define RAMSTART (0x200)
#define NRWWSTART (0xE000)
#elif defined(__AVR_ATmega8__) || defined(__AVR_ATmega88__)
#define RAMSTART (0x100)
#define NRWWSTART (0x1800)
#endif

// TODO: get actual .bss+.data size from GCC
#define BSS_SIZE	0x80

/* C zero initialises all global variables. However, that requires */
/* These definitions are NOT zero initialised, but that doesn't matter */
/* This allows us to drop the zero init code, saving us memory */
#define buff    ((uint8_t*)(RAMSTART+BSS_SIZE))

/*
 * Handle devices with up to 4 uarts (eg m1280.)  Rather inelegantly.
 * Note that mega8/m32 still needs special handling, because ubrr is handled
 * differently.
 */
#if UART == 0
# define UART_SRA UCSR0A
# define UART_SRB UCSR0B
# define UART_SRC UCSR0C
# define UART_SRL UBRR0L
# define UART_UDR UDR0
#elif UART == 1
#if !defined(UDR1)
#error UART == 1, but no UART1 on device
#endif
# define UART_SRA UCSR1A
# define UART_SRB UCSR1B
# define UART_SRC UCSR1C
# define UART_SRL UBRR1L
# define UART_UDR UDR1
#elif UART == 2
#if !defined(UDR2)
#error UART == 2, but no UART2 on device
#endif
# define UART_SRA UCSR2A
# define UART_SRB UCSR2B
# define UART_SRC UCSR2C
# define UART_SRL UBRR2L
# define UART_UDR UDR2
#elif UART == 3
#if !defined(UDR1)
#error UART == 3, but no UART3 on device
#endif
# define UART_SRA UCSR3A
# define UART_SRB UCSR3B
# define UART_SRC UCSR3C
# define UART_SRL UBRR3L
# define UART_UDR UDR3
#endif

static void eeprom_write(uint16_t addr, uint8_t val) {
  while (!eeprom_is_ready());

  EEAR = addr;
  EEDR = val;
  EECR |= 1 << EEMPE;	/* Write logical one to EEMPE */
  EECR |= 1 << EEPE;	/* Start eeprom write by setting EEPE */
}

static uint8_t eeprom_read(uint16_t addr) {
  while (!eeprom_is_ready());

  EEAR = addr;
  EECR |= 1 << EERE;	/* Start eeprom read by writing EERE */

  return EEDR;
}

/* main program starts here */
int main(void) {
  uint8_t ch;

  /*
   * Making these local and in registers prevents the need for initializing
   * them, and also saves space because code no longer stores to memory.
   * (initializing address keeps the compiler happy, but isn't really
   *  necessary, and uses 4 bytes of flash.)
   */
  register uint16_t address = 0;
  register uint8_t  length;

  // After the zero init loop, this is the first code to run.
  //
  // This code makes the following assumptions:
  //  No interrupts will execute
  //  SP points to RAMEND
  //  r1 contains zero
  //
  // If not, uncomment the following instructions:
  // cli();
  asm volatile ("cli");
  asm volatile ("clr __zero_reg__");
#if defined(__AVR_ATmega8__) || defined (__AVR_ATmega32__)
  SP=RAMEND;  // This is done by hardware reset
#endif

  /*
   * With wireless flashing it's possible that this is a remote
   * board that's hard to reset manually.  In this case optiboot can
   * force the watchdog to run before jumping to userspace, so that if
   * a buggy program is uploaded, the board resets automatically.  We
   * still use the watchdog to reset the bootloader too.
   */
#ifdef FORCE_WATCHDOG
  SP = RAMEND - 32;
#define reset_cause (*(uint8_t *) (RAMEND - 16 - 4))
#define marker (*(uint32_t *) (RAMEND - 16 - 3))

  /* GCC does loads Y with SP at the beginning, repeat it with the new SP */
  asm volatile ("in r28, 0x3d");
  asm volatile ("in r29, 0x3e");

  ch = MCUSR;
  MCUSR = 0;
  if ((ch & _BV(WDRF)) && marker == 0xdeadbeef) {
    marker = 0;
    appStart(reset_cause);
  }
  /* Save the original reset reason to pass on to the applicatoin */
  reset_cause = ch;
  marker = 0xdeadbeef;
#else
  // Adaboot no-wait mod
  ch = MCUSR;
  MCUSR = 0;
  if (ch & (_BV(WDRF) | _BV(PORF) | _BV(BORF)))
    appStart(ch);
#endif

#if BSS_SIZE > 0
  // Prepare .data
  asm volatile (
	"	ldi	r17, hi8(__data_end)\n"
	"	ldi	r26, lo8(__data_start)\n"
	"	ldi	r27, hi8(__data_start)\n"
	"	ldi	r30, lo8(__data_load_start)\n"
	"	ldi	r31, hi8(__data_load_start)\n"
	"	rjmp	cpchk\n"
	"copy:	lpm	__tmp_reg__, Z+\n"
	"	st	X+, __tmp_reg__\n"
	"cpchk:	cpi	r26, lo8(__data_end)\n"
	"	cpc	r27, r17\n"
	"	brne	copy\n");
  // Prepare .bss
  asm volatile (
	"	ldi	r17, hi8(__bss_end)\n"
	"	ldi	r26, lo8(__bss_start)\n"
	"	ldi	r27, hi8(__bss_start)\n"
	"	rjmp	clchk\n"
	"clear:	st	X+, __zero_reg__\n"
	"clchk:	cpi	r26, lo8(__bss_end)\n"
	"	cpc	r27, r17\n"
	"	brne	clear\n");
#endif

#if LED_START_FLASHES > 0
  // Set up Timer 1 for timeout counter
  TCCR1B = _BV(CS12) | _BV(CS10); // div 1024
#endif
  /*
   * Disable pullups that may have been enabled by a user program.
   * Somehow a pullup on RXD screws up everything unless RXD is externally
   * driven high.
   */
  DDRD |= 3;
  PORTD &= ~3;
#if defined(__AVR_ATmega8__) || defined (__AVR_ATmega32__)
  UCSRA = _BV(U2X); //Double speed mode USART
  UCSRB = _BV(RXEN) | _BV(TXEN);  // enable Rx & Tx
  UCSRC = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);  // config USART; 8N1
  UBRRL = (uint8_t)( (F_CPU + BAUD_RATE * 4L) / (BAUD_RATE * 8L) - 1 );
#else
  UART_SRA = _BV(U2X0); //Double speed mode USART0
  UART_SRB = _BV(RXEN0) | _BV(TXEN0);
  UART_SRC = _BV(UCSZ00) | _BV(UCSZ01);
  UART_SRL = (uint8_t)( (F_CPU + BAUD_RATE * 4L) / (BAUD_RATE * 8L) - 1 );
#endif

  // Set up watchdog to trigger after 2s
  watchdogConfig(WATCHDOG_2S);

#if (LED_START_FLASHES > 0) || defined(LED_DATA_FLASH)
  /* Set LED pin as output */
  LED_DDR |= _BV(LED);
#endif

  flash_led(2);
  if (!radio_init()) {
    while (1);
  }


#if LED_START_FLASHES > 0
  /* Flash onboard LED to signal entering of bootloader */
  flash_led(LED_START_FLASHES * 2);
#endif

  /* Forever loop */
  for (;;) {

    /* get character from UART */
    ch = getch();
    marker = 0;

    if(ch == STK_GET_PARAMETER) {
      unsigned char which = getch();
      verifySpace();
      if (which == 0x82) {
        /*
        * Send optiboot version as "minor SW version"
        */
	      putch(OPTIBOOT_MINVER);
      } else if (which == 0x81) {
	       putch(OPTIBOOT_MAJVER);
      } else {
        /*
        * GET PARAMETER returns a generic 0x03 reply for
              * other parameters - enough to keep Avrdude happy
        */
      	putch(0x03);
      }
    }
    else if(ch == STK_SET_DEVICE) {
      // SET DEVICE is ignored
      getNch(20);
    }
    else if(ch == STK_SET_DEVICE_EXT) {
      // SET DEVICE EXT is ignored
      getNch(5);
    }
    else if(ch == STK_LOAD_ADDRESS) {
      // LOAD ADDRESS
      uint16_t newAddress;
      newAddress = getch();
      newAddress |= getch() << 8;
#ifdef RAMPZ
      // Transfer top bit to RAMPZ
      RAMPZ = (newAddress & 0x8000) ? 1 : 0;
#endif
      newAddress <<= 1; // Convert from word address to byte address
      address = newAddress;
      verifySpace();
    }
    else if(ch == STK_UNIVERSAL) {
      // UNIVERSAL command is ignored
      getNch(4);
      putch(0x00);
    }
    /* Write memory, length is big endian and is in bytes */
    else if(ch == STK_PROG_PAGE) {
      // PROGRAM PAGE - we support flash and EEPROM programming
      uint8_t *bufPtr;
      uint16_t addrPtr;
      uint8_t type;

      getch();			/* getlen() */
      length = getch();
      type = getch();

#ifdef SUPPORT_EEPROM
      if (type == 'F')		/* Flash */
#endif
        // If we are in RWW section, immediately start page erase
        if (address < NRWWSTART) __boot_page_erase_short((uint16_t)(void*)address);

      // While that is going on, read in page contents
      bufPtr = buff;
      do *bufPtr++ = getch();
      while (--length);

#ifdef SUPPORT_EEPROM
      if (type == 'F') {	/* Flash */
#endif
        // If we are in NRWW section, page erase has to be delayed until now.
        // Todo: Take RAMPZ into account (not doing so just means that we will
        //  treat the top of both "pages" of flash as NRWW, for a slight speed
        //  decrease, so fixing this is not urgent.)
        if (address >= NRWWSTART) __boot_page_erase_short((uint16_t)(void*)address);

        // Read command terminator, start reply
        verifySpace();

        // If only a partial page is to be programmed, the erase might not be complete.
        // So check that here
        boot_spm_busy_wait();

        // Copy buffer into programming buffer
        bufPtr = buff;
        addrPtr = (uint16_t)(void*)address;
        ch = SPM_PAGESIZE / 2;
        do {
          uint16_t a;
          a = *bufPtr++;
          a |= (*bufPtr++) << 8;
          __boot_page_fill_short((uint16_t)(void*)addrPtr,a);
          addrPtr += 2;
        } while (--ch);

        // Write from programming buffer
        __boot_page_write_short((uint16_t)(void*)address);
        boot_spm_busy_wait();

#if defined(RWWSRE)
        // Reenable read access to flash
        boot_rww_enable();
#endif
#ifdef SUPPORT_EEPROM
      } else if (type == 'E') {	/* EEPROM */
        // Read command terminator, start reply
        verifySpace();

        length = bufPtr - buff;
        addrPtr = address;
        bufPtr = buff;
        while (length--) {
          watchdogReset();
          eeprom_write(addrPtr++, *bufPtr++);
        }
      }
#endif
    }
    /* Read memory block mode, length is big endian.  */
    else if(ch == STK_READ_PAGE) {
      // READ PAGE - we only read flash and EEPROM
      uint8_t type;

      getch();			/* getlen() */
      length = getch();
      type = getch();

      verifySpace();
      /* TODO: putNch */
#ifdef SUPPORT_EEPROM
      if (type == 'F')
#endif
        do {
#if defined(RAMPZ)
          // Since RAMPZ should already be set, we need to use EPLM directly.
          // Also, we can use the autoincrement version of lpm to update "address"
          //      do putch(pgm_read_byte_near(address++));
          //      while (--length);
          // read a Flash and increment the address (may increment RAMPZ)
          __asm__ ("elpm %0,Z+\n" : "=r" (ch), "=z" (address): "1" (address));
#else
          // read a Flash byte and increment the address
          __asm__ ("lpm %0,Z+\n" : "=r" (ch), "=z" (address): "1" (address));
#endif
          putch(ch);
        } while (--length);
#ifdef SUPPORT_EEPROM
      else if (type == 'E')
        while (length--)
          putch(eeprom_read(address++));
#endif
    }

    /* Get device signature bytes  */
    else if(ch == STK_READ_SIGN) {
      // READ SIGN - return what Avrdude wants to hear
      verifySpace();
      putch(SIGNATURE_0);
      putch(SIGNATURE_1);
      putch(SIGNATURE_2);
    }
    else if (ch == STK_LEAVE_PROGMODE) { /* 'Q' */
      // Adaboot no-wait mod
      marker = 0xdeadbeef;
      watchdogConfig(WATCHDOG_16MS);
      verifySpace();
    }
    else {
      // This covers the response to commands like STK_ENTER_PROGMODE
      verifySpace();
    }
    putch(STK_OK);
  }
}

/*
 * Radio mode gets set the moment we receive any command over the radio chip.
 * From that point our responses will also be sent through the radio instead
 * of through the UART.  Otherwise all communication goes through the UART
 * as normal.
 *
 * TODO: require a challenge-response negotiation at least to start the
 * radio mode, for security -- the keys need to be stored in EEPROM.  Ideally
 * we'd encrypt all communication but that would be an overkill for the
 * bootloader.
 */

static int radio_init(void) {
  uint8_t addr[5];

  spi_init();

  radio_present = 0;
  if (!nrf24_init()) {
    radio_present = 1;
  }

  if (!radio_present)
    return 0;

  addr[0] = 0x02;
  addr[1] = 0x02;
  addr[2] = 0x02;
  addr[3] = 0x02;
  addr[4] = 0x02;
  nrf24_set_rx_addr(addr);

  addr[0] = 0x01;
  addr[1] = 0x01;
  addr[2] = 0x01;
  addr[3] = 0x01;
  addr[4] = 0x01;
  nrf24_set_tx_addr(addr);

  nrf24_rx_mode();
  return 1;
}

void putch(char ch) {
  static uint8_t pkt_len = 0;
  static uint8_t pkt_buf[32];

  pkt_buf[pkt_len++] = ch;

  if (ch == STK_OK || pkt_len == pkt_max_len) {
    uint8_t cnt = 128;

    while (--cnt) {
      /* Wait 4ms to allow the remote end to switch to Rx mode */
      my_delay(4);

      nrf24_tx(pkt_buf, pkt_len);
      if (!nrf24_tx_result_wait())
        break;

      /*
      * TODO: also check if there's anything in the Rx FIFO - that
      * would indicate that the other side has actually received our
      * packet but the ACK may have been lost instead.  In any case
      * the other side is not listening for what we're re-sending,
      * maybe has given up and is resending the full command which
      * is ok.
      */
    }

    pkt_len = 1;
    pkt_buf[0] ++;
  }

}

uint8_t getch(void) {
  uint8_t ch;
  static uint8_t pkt_len = 0, pkt_start = 0;
  static uint8_t pkt_buf[32];

  while(1) {
    if (pkt_len || nrf24_rx_fifo_data()) {

      if (!pkt_len) {
        static uint8_t seqn = 0xff;
        watchdogReset();
        nrf24_rx_read(pkt_buf, &pkt_len);
        pkt_start = 1;

        if (!pkt_len)
          continue;

        if (pkt_buf[0] == seqn) {
          pkt_len = 0;
          continue;
        }

        seqn = pkt_buf[0];
        pkt_len--;
      }

      ch = pkt_buf[pkt_start ++];
      pkt_len --;
      break;
    }
  }

  return ch;
}

void getNch(uint8_t count) {
  do getch(); while (--count);
  verifySpace();
}

void wait_timeout(void) {
  nrf24_idle_mode(0);		      // power the radio off
  watchdogConfig(WATCHDOG_16MS);      // shorten WD timeout
  while (1)			      // and busy-loop so that WD causes
    ;				      //  a reset and app start.
}

void verifySpace(void) {
  if (getch() != CRC_EOP)
    wait_timeout();
  putch(STK_INSYNC);
}

#if LED_START_FLASHES > 0
void flash_led(uint8_t count) {
  do {
    TCNT1 = -(F_CPU/(1024*16));
    TIFR1 = _BV(TOV1);
    while(!(TIFR1 & _BV(TOV1)));
#if defined(__AVR_ATmega8__)  || defined (__AVR_ATmega32__)
    LED_PORT ^= _BV(LED);
#else
    LED_PIN |= _BV(LED);
#endif
    watchdogReset();
  } while (--count);
}

#endif

// Watchdog functions. These are only safe with interrupts turned off.
void watchdogReset() {
  __asm__ __volatile__ (
    "wdr\n"
  );
}

void watchdogConfig(uint8_t x) {
  WDTCSR = _BV(WDCE) | _BV(WDE);
  WDTCSR = x;
}

void appStart(uint8_t rstFlags) {
  watchdogConfig(WATCHDOG_OFF);

  // save the reset flags in the designated register
  //  This can be saved in a main program by putting code in .init0 (which
  //  executes before normal c init code) to save R2 to a global variable.
  __asm__ __volatile__ ("mov r2, %0\n" :: "r" (rstFlags));

  __asm__ __volatile__ (
    // Jump to RST vector
    "clr r30\n"
    "clr r31\n"
    "ijmp\n"
  );
}
