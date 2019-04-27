APP =		pic32mzdask-ips
MACHINE =	mips

CC =		${CROSS_COMPILE}gcc
LD =		${CROSS_COMPILE}ld
OBJCOPY =	${CROSS_COMPILE}objcopy

OBJDIR =	${CURDIR}/obj
LDSCRIPT =	${OBJDIR}/ldscript
LDSCRIPT_TPL =	${CURDIR}/ldscript.tpl

FONT =		${OBJDIR}/ter-124n.ld
FONT_SRC =	${CURDIR}/fonts/ter-124n.pcf.gz

OBJECTS =	main.o						\
		osfive/sys/mips/microchip/pic32_intc.o		\
		osfive/sys/mips/microchip/pic32_port.o		\
		osfive/sys/mips/microchip/pic32_pps.o		\
		osfive/sys/mips/microchip/pic32_spi.o		\
		osfive/sys/mips/microchip/pic32_syscfg.o	\
		osfive/sys/mips/microchip/pic32_timer.o		\
		osfive/sys/mips/microchip/pic32_uart.o		\
		start.o

KERNEL =	malloc sched
LIBRARIES =	libc libc_quad libfont

CFLAGS =	-march=mips32r2 -EL -msoft-float -nostdlib	\
		-mno-abicalls -O -fno-pic -fno-builtin-printf	\
		-O -pipe -g -nostdinc -fno-omit-frame-pointer	\
		-fno-optimize-sibling-calls -ffreestanding	\
		-fwrapv	-fdiagnostics-show-option		\
		-fms-extensions -finline-limit=8000 -Wall	\
		-Wredundant-decls -Wnested-externs		\
		-Wstrict-prototypes -Wmissing-prototypes	\
		-Wpointer-arith -Winline -Wcast-qual -Wundef	\
		-Wno-pointer-sign -Wno-format			\
		-Wmissing-include-dirs -Wno-unknown-pragmas	\
		-Werror -DCONFIG_SCHED -D__mips_o32

all:	${FONT} ${OBJDIR}/${APP}.elf ${OBJDIR}/${APP}.srec

${LDSCRIPT}: ${LDSCRIPT_TPL}
	@sed s#FONT_PATH#${FONT}#g ${LDSCRIPT_TPL} > ${LDSCRIPT}

${FONT}: ${FONT_SRC}
	@gunzip -c ${FONT_SRC} | \
	    hexdump -v -e '"BYTE(0x" 1/1 "%02X" ")\n"' > ${FONT}

clean:
	@rm -f ${OBJECTS} ${FONT} ${LDSCRIPT} ${OBJDIR}/${APP}.*

include osfive/lib/libc/Makefile.inc
include osfive/lib/libfont/Makefile.inc
include osfive/mk/default.mk
