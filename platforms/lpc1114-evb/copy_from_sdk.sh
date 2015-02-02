files=" microbuilder_sdk/core/cpu/cpu.c \
	microbuilder_sdk/core/cpu/cpu.h \
	microbuilder_sdk/core/gpio/gpio.c \
	microbuilder_sdk/core/gpio/gpio.h \
	microbuilder_sdk/core/iap/iap.c \
	microbuilder_sdk/core/iap/iap.h \
	microbuilder_sdk/lpc111x.h \
	microbuilder_sdk/lpc1xxx/LPC11xx_handlers.c \
	microbuilder_sdk/lpc1xxx/LPC1xxx_startup.c \
	microbuilder_sdk/sysdefs.h \
	microbuilder_sdk/core/systick/systick.c \
	microbuilder_sdk/core/systick/systick.h \
	microbuilder_sdk/core/uart/uart_buf.c \
	microbuilder_sdk/core/uart/uart.c \
	microbuilder_sdk/core/uart/uart.h \
	microbuilder_sdk/core/wdt/wdt.c \
	microbuilder_sdk/core/wdt/wdt.h "
	
for f in $files
do
  cp "$f" hal/drivers
done
      
	
	
