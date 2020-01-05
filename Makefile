AVRDUDE_FLAGS=-p atmega328p -C ./avrdude_gpio.conf -c pi_1
FUSES=-U lfuse:w:0xe2:m -U hfuse:w:0xd9:m -U efuse:w:0x07:m
MCU=atmega328p

CC=avr-gcc -Wall -Wextra
CFLAGS=-Os -mmcu=${MCU} -Isrc

wallclock.hex: wallclock.elf
	avr-objcopy -j .text -j .data -O ihex $< $@

wallclock.elf: src/main.o src/twimaster.o src/ds1307.o
	$(CC) -mmcu=${MCU} -o $@ $^

test-connection:
	sudo avrdude ${AVRDUDE_FLAGS}

clean:
	rm -f *.elf src/*.o

upload: wallclock.hex
	sudo avrdude ${AVRDUDE_FLAGS} -U flash:w:$<:i

fuse:
	sudo avrdude ${AVRDUDE_FLAGS} ${FUSES}

# vim: set ts=8 sts=8 sw=8 noexpandtab:
