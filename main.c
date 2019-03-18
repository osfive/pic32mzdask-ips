/*-
 * Copyright (c) 2018 Ruslan Bukin <br@bsdpad.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <sys/console.h>
#include <sys/systm.h>
#include <sys/thread.h>
#include <sys/malloc.h>

#include <machine/cpuregs.h>
#include <machine/cpufunc.h>
#include <machine/frame.h>

#include <mips/mips/timer.h>
#include <mips/mips/trap.h>

#include <dev/spi/spi.h>
#include <dev/st7789v/st7789v.h>
#include <mips/microchip/pic32mzda.h>

#include <libfont/libfont.h>

#define	CPU_FREQ	200000000

void app_init(void);

static struct global_data {
	uint8_t buffer[240*240*2];
	uint8_t *ptr;
	struct font_info font;
} g_data;

extern char MipsException[], MipsExceptionEnd[];
#define	MIPS_GEN_EXC_VEC	0x80000180

PIC32_DEVCFG (
	DEVCFG0_UNUSED |
	DEVCFG0_BOOTISA_MIPS32,
	DEVCFG1_UNUSED |
	DEVCFG1_FNOSC_SPLL |
	DEVCFG1_POSCMOD_HS_OSC |
	DEVCFG1_FCKSM_CKS_EN_M_EN |
	DEVCFG1_IESO,
	DEVCFG2_UNUSED |
	DEVCFG2_FPLLIDIV(3) |
	DEVCFG2_FPLLRNG_5_10 |
	DEVCFG2_FPLLMULT(50) |
	DEVCFG2_FPLLODIV_2 |
	DEVCFG2_UPLLFSEL_24MHZ,
	DEVCFG3_UNUSED |
	DEVCFG3_FETHIO |
	DEVCFG3_USERID(0xffff)
);

/* Software contexts */
static struct pic32_uart_softc uart_sc;
static struct pic32_port_softc port_sc;
static struct pic32_pps_softc pps_sc;
static struct pic32_syscfg_softc syscfg_sc;
static struct pic32_spi_softc spi_sc;
static struct pic32_intc_softc intc_sc;
static struct pic32_timer_softc timer_sc;
static struct mips_timer_softc mtimer_sc;

spi_device_t spi_dev;

extern uint32_t _sfont;
extern uint32_t _sbss;
extern uint32_t _ebss;

static void
softintr(void *arg, struct trapframe *frame, int i)
{
	uint32_t cause;

	printf("Soft interrupt %d\n", i);

	cause = mips_rd_cause();
	cause &= ~(1 << (8 + i));
	mips_wr_cause(cause);

};

static void
mips_timer_intr_wrap(void *arg, struct trapframe *frame,
    int mips_irq, int intc_irq)
{

	mips_timer_intr(arg, frame, mips_irq);
}

static const struct mips_intr_entry mips_intr_map[MIPS_N_INTR] = {
	[0] = { softintr, NULL },
	[1] = { softintr, NULL },
	[2] = { pic32_intc_intr, (void *)&intc_sc },
};

static const struct intc_intr_entry intc_intr_map[216] = {
	[_CORE_TIMER_VECTOR] = { mips_timer_intr_wrap, (void *)&mtimer_sc },
};

void
udelay(uint32_t usec)
{

	mips_timer_udelay(&mtimer_sc, usec);
}

static void
pic32_reset(struct pic32_port_softc *sc, uint8_t enable)
{

	pic32_port_tris(sc, PORT_K, 1, PORT_OUTPUT);
	pic32_port_lat(sc, PORT_K, 1, enable);
}

static void
ips_data_command(struct pic32_port_softc *sc, uint8_t enable)
{

	pic32_port_tris(sc, PORT_K, 2, PORT_OUTPUT);
	pic32_port_lat(sc, PORT_K, 2, enable);
}

#if 0
static void
pic32_bl(struct pic32_port_softc *sc, uint8_t enable)
{

	pic32_port_tris(sc, PORT_G, 9, PORT_OUTPUT);
	pic32_port_lat(sc, PORT_G, 9, enable);
}
#endif

static void
uart_putchar(int c, void *arg)
{
	struct pic32_uart_softc *sc;

	sc = arg;

	if (c == '\n')
		pic32_putc(sc, '\r');

	pic32_putc(sc, c);
}

static void
draw_pixel(void *arg, int x, int y, int pixel)
{
	uint16_t *addr;

	addr = (uint16_t *)&g_data.ptr[240 * y * 2 + x * 2];

	if (pixel)
		*addr = 0xffff;
	else
		*addr = 0;
}

void draw_text(uint8_t *s);
void draw_text_utf8(uint8_t *s);

void
draw_text(uint8_t *s)
{
	struct char_info ci;
	int i;

	g_data.ptr = (uint8_t *)&g_data.buffer[0];
	for (i = 0; i < strlen(s); i++) {
		get_char_info(&g_data.font, s[i], &ci);
		draw_char(&g_data.font, s[i]);
		g_data.ptr += ci.xsize * 2;
	}
}

void
draw_text_utf8(uint8_t *s)
{
	struct char_info ci;
	uint8_t *newptr;
	uint8_t *buf;
	int c;

	g_data.ptr = (uint8_t *)&g_data.buffer[0];

	buf = (uint8_t *)s;

	for (;;) {
		c = utf8_to_ucs2(buf, &newptr);
		if (c == -1)
			return;
		buf = newptr;
		get_char_info(&g_data.font, c, &ci);
		draw_char(&g_data.font, c);
		g_data.ptr += ci.xsize * 2;
	}
}

static void
display_clear(void)
{
	int i;

	for (i = 0; i < 240*240*2; i++)
		g_data.buffer[i] = 0;
}

static void
ips_cmd(spi_device_t *dev, uint8_t cmd, int len, ...)
{
	va_list args;
	uint8_t data;
	int i;

	ips_data_command(&port_sc, 0);
	dev->transfer(dev, &cmd, NULL, 1);
	ips_data_command(&port_sc, 1);

	va_start(args, len);
	for (i = 0; i < len; i++) {
		data = (uint8_t)va_arg(args, unsigned int);
		dev->transfer(dev, &data, NULL, 1);
	}
	va_end(args);
}

static void
ips_data(spi_device_t *dev, uint8_t data)
{

	dev->transfer(dev, &data, NULL, 1);
}

static void
ips_address(void)
{

	ips_cmd(&spi_dev, 0x2A, 4, 0x00, 0x00, 0x00, 0xEF);
	ips_cmd(&spi_dev, 0x2B, 4, 0x00, 0x00, 0x00, 0xEF);
	ips_cmd(&spi_dev, 0x2C, 0);
}

static void
ips_init(void)
{

	printf("Initializing display\n");

	/* Reset IPS */
	pic32_reset(&port_sc, 0);
	udelay(200000);
	pic32_reset(&port_sc, 1);

	ips_cmd(&spi_dev, ST7789V_SWRESET, 0);
	udelay(150000);
	ips_cmd(&spi_dev, ST7789V_SLPOUT, 0);
	udelay(500000);
	ips_cmd(&spi_dev, ST7789V_COLMOD, 1,
	    (COLMOD_CTRL_FMT_16BIT | COLMOD_RGB_FMT_65K));
	udelay(10000);

	ips_cmd(&spi_dev, ST7789V_MADCTL, 1, 0x00);
	ips_cmd(&spi_dev, ST7789V_GAMSET, 1, 0x02); //Gamma curve 2 (G1.8)
	ips_cmd(&spi_dev, ST7789V_DGMEN, 1, 0x04); // Enable gamma

	ips_cmd(&spi_dev, ST7789V_INVON, 0);
	udelay(10000);
	ips_cmd(&spi_dev, ST7789V_NORON, 0);
	udelay(10000);
	ips_cmd(&spi_dev, ST7789V_DISPON, 0);
	udelay(500000);
}

static void
ips_main(void *arg)
{
	uint8_t text[32];
	int z;
	int i;

	ips_init();

	display_clear();

	ips_address();
	ips_data_command(&port_sc, 1);
	for (i = 0; i < 240 * 240 * 2; i++)
		ips_data(&spi_dev, g_data.buffer[i]);

	printf("Main loop\n");

	z = 0;
	while (1) {
		display_clear();
		sprintf(text, "hello %d", z);
		draw_text_utf8(text);

		ips_address();
		ips_data_command(&port_sc, 1);
		for (i = 0; i < 240 * 240 * 2; i++)
			ips_data(&spi_dev, g_data.buffer[i]);
		z += 1;

		raw_sleep(CPU_FREQ / 2);
	}
}

static void
app_ports_init(struct pic32_port_softc *sc)
{

	/* UART2 digital */
	pic32_port_ansel(&port_sc, PORT_B, 14, 1);
	pic32_port_ansel(&port_sc, PORT_B, 15, 1);

	pic32_pps_write(&pps_sc, PPS_U2RXR, IS2_RPB0);
	pic32_pps_write(&pps_sc, PPS_RPG9R, OS3_U2TX);

	/* SPI MOSI */
	pic32_pps_write(&pps_sc, PPS_RPG8R, OS0_SDO2);
}

void
app_init(void)
{
	uint32_t status;
	uint32_t *sbss;
	uint32_t *ebss;
	uint32_t reg;

	/* Clear BSS */
	sbss = (uint32_t *)&_sbss;
	ebss = (uint32_t *)&_ebss;
	while (sbss < ebss)
		*sbss++ = 0;

	md_init();
	malloc_init();
	malloc_add_region(0xa0040380, (256*1024 - 0x380));

	/* Install interrupt code */
	bcopy(MipsException, (void *)MIPS_GEN_EXC_VEC,
		MipsExceptionEnd - MipsException);

	pic32_port_init(&port_sc, PORTS_BASE);
	pic32_pps_init(&pps_sc, PPS_BASE);
	pic32_syscfg_init(&syscfg_sc, SYSCFG_BASE);
	pic32_timer_init(&timer_sc, TIMER_BASE(1));
	mips_timer_init(&mtimer_sc, CPU_FREQ / 2);
	app_ports_init(&port_sc);

	reg = SPICON_MSTEN | SPICON_CKE | SPICON_CKP;
	pic32_spi_init(&spi_dev, &spi_sc, SPI2_BASE,
	    CPU_FREQ, 100000000, reg);

	pic32_uart_init(&uart_sc, UART2_BASE, 115200, CPU_FREQ, 2);
	console_register(uart_putchar, (void *)&uart_sc);

	pic32_intc_init(&intc_sc, INTC_BASE);

	font_init(&g_data.font, (uint8_t *)&_sfont);
	g_data.font.draw_pixel = draw_pixel;

	printf("\n\n\n");
	printf("osfive initialized\n");

	mips_wr_ebase(0x80000000);

	status = mips_rd_status();
	status &= ~(MIPS_SR_IM_M);
	status |= MIPS_SR_IM_HARD(0);
	status |= MIPS_SR_IE;
	status &= ~MIPS_SR_BEV;
	status &= ~(MIPS_SR_EXL | MIPS_SR_ERL);
	mips_wr_status(status);

	mips_install_intr_map(mips_intr_map);
	pic32_intc_install_intr_map(&intc_sc, intc_intr_map);
	pic32_intc_enable(&intc_sc, _CORE_TIMER_VECTOR, 1);

	printf("status register: %x\n", mips_rd_status());
	printf("compare register: %x\n", mips_rd_compare());

	thread_create("ips", 50000000, ips_main, NULL);

	__asm __volatile("syscall");

	while (1)
		cpu_idle();
}
