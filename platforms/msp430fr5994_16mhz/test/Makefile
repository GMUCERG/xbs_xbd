all: interrupt.elf rx_i2c.elf
interrupt.elf: interrupt.c
	msp430-gcc -g3 -gdwarf-2 -mmcu=msp430xG4618 interrupt.c -o interrupt.elf
rx_i2c.elf: rx_i2c.c
	msp430-gcc -g3 -gdwarf-2 -mmcu=msp430xG4618 rx_i2c.c -o rx_i2c.elf
