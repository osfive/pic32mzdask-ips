APP =		pic32mzdask-ips
ARCH =		mips

CC =		${CROSS_COMPILE}gcc
LD =		${CROSS_COMPILE}ld
OBJCOPY =	${CROSS_COMPILE}objcopy

LDSCRIPT =	${.OBJDIR}/ldscript
LDSCRIPT_TPL =	${.CURDIR}/ldscript.tpl

FONT =		${.OBJDIR}/ter-124n.ld
FONT_SRC =	${.CURDIR}/fonts/ter-124n.pcf.gz

OBJECTS =	main.o						\
		osfive/sys/mips/mips/exception.o		\
		osfive/sys/mips/mips/trap.o			\
		osfive/sys/mips/mips/timer.o			\
		osfive/sys/mips/microchip/pic32_uart.o		\
		osfive/sys/mips/microchip/pic32_port.o		\
		osfive/sys/mips/microchip/pic32_pps.o		\
		osfive/sys/mips/microchip/pic32_spi.o		\
		osfive/sys/mips/microchip/pic32_syscfg.o	\
		osfive/sys/mips/microchip/pic32_intc.o		\
		osfive/sys/mips/microchip/pic32_timer.o		\
		osfive/sys/kern/subr_prf.o			\
		osfive/sys/kern/subr_console.o			\
		osfive/lib/libfont/libfont.o			\
		osfive/lib/libc/stdio/printf.o			\
		osfive/lib/libc/string/strlen.o			\
		osfive/lib/libc/string/bcopy.o			\
		start.o

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
		-Werror

all: ${FONT} compile link srec

${LDSCRIPT}: ${LDSCRIPT_TPL}
	sed s#FONT_PATH#${FONT}#g ${LDSCRIPT_TPL} > ${LDSCRIPT}

${FONT}: ${FONT_SRC}
	gunzip -c ${FONT_SRC} | hexdump -v -e '"BYTE(0x" 1/1 "%02X" ")\n"' > ${FONT}

clean:
	rm -f ${OBJECTS:M*} ${FONT} ${LDSCRIPT} ${APP}.elf ${APP}.srec

.include "osfive/mk/user.mk"
.include "osfive/mk/compile.mk"
.include "osfive/mk/link.mk"
.include "osfive/mk/readelf.mk"
.include "osfive/mk/objdump.mk"
